/*
 * Copyright 2023 Clement Vuchener
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "WorkDetail.h"

#include "df/utils.h"
#include "DwarfFortressData.h"
#include "Unit.h"
#include "WorkDetailModel.h"
#include <QCoroFuture>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

Q_LOGGING_CATEGORY(WorkDetailLog, "workdetail");

#include "workdetailtest.pb.h"
#include <dfhack-client-qt/Function.h>

static const DFHack::Function<
	dfproto::workdetailtest::EditWorkDetail,
	dfproto::workdetailtest::WorkDetailResult
> EditWorkDetail = {"workdetailtest", "EditWorkDetail"};

WorkDetail::WorkDetail(std::unique_ptr<df::work_detail> &&work_detail, DwarfFortressData &df, QObject *parent):
	QObject(parent),
	_wd(std::move(work_detail)),
	_df(df)
{
	refresh();
}

WorkDetail::~WorkDetail()
{
}

void WorkDetail::update(std::unique_ptr<df::work_detail> &&work_detail)
{
	_wd = std::move(work_detail);
	refresh();
}

void WorkDetail::refresh()
{
	_display_name = df::fromCP437(_wd->name);
	_statuses.clear();
}

QString WorkDetail::makeToolTip() const
{
	QString tip = "<h3>" + _display_name + "</h3><ul>";
	tip += "<p>";
	switch (_wd->flags.bits.mode) {
	case df::work_detail_mode::EverybodyDoesThis:
		tip += tr("Everybody does this");
		break;
	case df::work_detail_mode::OnlySelectedDoesThis:
		tip += tr("Only selected does this");
		break;
	case df::work_detail_mode::NobodyDoesThis:
		tip += tr("Nobody does this");
		break;
	default:
		break;
	}
	tip += "</p>";
	for (std::size_t i = 0; i < df::unit_labor::Count; ++i) {
		auto labor = static_cast<df::unit_labor_t>(i);
		if (_wd->allowed_labors[i])
			tip += "<li>" + QString::fromLocal8Bit(caption(labor)) + "</li>";
	}
	tip += "</ul>";
	return tip;
}

bool WorkDetail::isAssigned(int unit_id) const
{
	return std::ranges::binary_search(_wd->assigned_units, unit_id);
}

void WorkDetail::setAssignment(int unit_id, bool assign, ChangeStatus status)
{
	auto it = std::ranges::lower_bound(_wd->assigned_units, unit_id);
	bool assigned = it != _wd->assigned_units.end() && *it == unit_id;
	if (assign && !assigned)
		_wd->assigned_units.insert(it, unit_id);
	if (!assign && assigned)
		_wd->assigned_units.erase(it);
	if (status == NoChange)
		_statuses.erase(unit_id);
	else
		_statuses[unit_id] = status;
}

std::vector<std::pair<df::unit_labor_t, bool>> WorkDetail::Properties::allLabors(std::span<const bool, df::unit_labor::Count> labors)
{
	std::vector<std::pair<df::unit_labor_t, bool>> l;
	l.reserve(df::unit_labor::Count);
	for (int i = 0; i < df::unit_labor::Count; ++i)
		l.emplace_back(static_cast<df::unit_labor_t>(i), labors[i]);
	return l;
}

void WorkDetail::Properties::setArgs(dfproto::workdetailtest::WorkDetailProperties &args) const
{
	if (!name.isEmpty())
		args.set_name(df::toCP437(name));
	if (mode) {
		switch (*mode) {
		case df::work_detail_mode::EverybodyDoesThis:
			args.set_mode(dfproto::workdetailtest::EverybodyDoesThis);
			break;
		case df::work_detail_mode::NobodyDoesThis:
			args.set_mode(dfproto::workdetailtest::NobodyDoesThis);
			break;
		case df::work_detail_mode::OnlySelectedDoesThis:
			args.set_mode(dfproto::workdetailtest::OnlySelectedDoesThis);
			break;
		default:
			break;
		}
	}
	if (icon)
		args.set_icon(static_cast<int>(*icon));
	if (no_modify)
		args.set_no_modify(*no_modify);
	if (cannot_be_everybody)
		args.set_cannot_be_everybody(*cannot_be_everybody);
	for (const auto &[labor, enable]: labors) {
		auto labor_change = args.mutable_labors()->Add();
		labor_change->set_labor(labor);
		labor_change->set_enable(enable);
	}
}

QJsonObject WorkDetail::Properties::toJson() const
{
	QJsonObject object;
	if (!name.isEmpty())
		object.insert("name", name);
	if (mode)
		object.insert("mode", QString::fromLocal8Bit(to_string(*mode)));
	if (icon)
		object.insert("icon", QString::fromLocal8Bit(to_string(*icon)));
	if (cannot_be_everybody)
		object.insert("cannot_be_everybody", *cannot_be_everybody);
	QJsonArray labor_array;
	for (const auto &[labor, enabled]: labors)
		if (enabled)
			labor_array.append(QString::fromLocal8Bit(to_string(labor)));
	if (!labor_array.isEmpty())
		object.insert("labors", std::move(labor_array));
	return object;
}

WorkDetail::Properties WorkDetail::Properties::fromJson(const QJsonObject &json)
{
	Properties props;
	if (json.contains("name"))
		props.name = json.value("name").toString();
	if (json.contains("mode")) {
		auto bytes = json.value("mode").toString().toLocal8Bit();
		props.mode = df::work_detail_mode::from_string({bytes.data(), std::size_t(bytes.size())});
		if (!props.mode)
			qCCritical(WorkDetailLog) << "Invalid mode value" << bytes;
	}
	if (json.contains("icon")) {
		auto bytes = json.value("icon").toString().toLocal8Bit();
		props.icon = df::work_detail_icon::from_string({bytes.data(), std::size_t(bytes.size())});
		if (!props.icon)
			qCCritical(WorkDetailLog) << "Invalid icon value" << bytes;
	}
	if (json.contains("cannot_be_everybody"))
		props.cannot_be_everybody = json.value("cannot_be_everybody").toBool();
	std::array<bool, df::unit_labor::Count> labors = {false};
	if (json.contains("labors")) {
		if (!json.value("labors").isArray())
			qCCritical(WorkDetailLog) << "Work detail json \"labors\" must be an array";
		for (auto item: json.value("labors").toArray()) {
			auto bytes = item.toString().toLocal8Bit();
			auto labor = df::unit_labor::from_string({bytes.data(), std::size_t(bytes.size())});
			if (labor)
				labors[*labor] = true;
			else
				qCCritical(WorkDetailLog) << "Invalid labor vlaue" << bytes;
		}
		props.labors = allLabors(labors);
	}
	return props;
}

WorkDetail::Properties WorkDetail::Properties::fromWorkDetail(const df::work_detail &wd)
{
	Properties props;
	props.name = df::fromCP437(wd.name);
	props.mode = df::work_detail_mode_t(wd.flags.bits.mode);
	props.icon = wd.icon;
	props.no_modify = wd.flags.bits.no_modify;
	props.cannot_be_everybody = wd.flags.bits.cannot_be_everybody;
	props.labors.reserve(df::unit_labor::Count);
	for (std::size_t i = 0; i < df::unit_labor::Count; ++i)
		props.labors.emplace_back(df::unit_labor_t(i), wd.allowed_labors[i]);
	return props;
}

QCoro::Task<> WorkDetail::assign(int unit_id, bool assign)
{
	return changeAssignments({unit_id}, [assign](bool) { return assign; });
}

QCoro::Task<> WorkDetail::assign(std::vector<int> units, bool assign)
{
	return changeAssignments(std::move(units), [assign](bool) { return assign; });
}

QCoro::Task<> WorkDetail::toggle(std::vector<int> units)
{
	return changeAssignments(std::move(units), [](bool assign) { return !assign; });
}

Q_DECLARE_LOGGING_CATEGORY(DFHackLog);

template<typename F>
QCoro::Task<> WorkDetail::changeAssignments(std::vector<int> units, F get_assign)
{
	auto thisptr = shared_from_this(); // make sure the object live for the whole coroutine
	Q_ASSERT(thisptr);
	// Prepare arguments
	dfproto::workdetailtest::EditWorkDetail args;
	if (!setId(*args.mutable_id())) {
		qWarning() << "invalid work detail index";
		co_return;
	}
	std::vector<int8_t> old_assignment(units.size());
	std::ranges::transform(units, old_assignment.begin(), [this](int id) { return isAssigned(id); });
	for (std::size_t i = 0; i < units.size(); ++i) {
		auto assignment = args.mutable_changes()->mutable_assignments()->Add();
		assignment->set_unit_id(units[i]);
		auto assign = get_assign(old_assignment[i]);
		assignment->set_enable(assign);
		setAssignment(units[i], assign, WorkDetail::Pending);
	}
	unitDataChanged(_df.units->makeSelection(units));
	// Call
	if (!_df.dfhack) {
		qCWarning(DFHackLog) << "DFHack client was deleted";
		co_return;
	}
	auto r = co_await EditWorkDetail(*_df.dfhack, args).first;
	// Check results
	if (!r) {
		qCWarning(DFHackLog) << "editWorkDetail failed" << make_error_code(r.cr).message();
		for (std::size_t i = 0; i < units.size(); ++i)
			setAssignment(units[i], old_assignment[i], WorkDetail::Failed);
		unitDataChanged(_df.units->makeSelection(units));
		co_return;
	}
	const auto &wd_result = r->work_detail();
	if (!wd_result.success()) {
		qCWarning(DFHackLog) << "editWorkDetail failed" << wd_result.error();
		for (std::size_t i = 0; i < units.size(); ++i)
			setAssignment(units[i], old_assignment[i], WorkDetail::Failed);
		unitDataChanged(_df.units->makeSelection(units));
		co_return;
	}
	// Apply changes
	for (std::size_t i = 0; i < units.size(); ++i) {
		const auto &assign_result = r->assignments(i);
		if (!assign_result.success()) {
			qCWarning(DFHackLog) << "editWorkDetail failed" << assign_result.error();
			setAssignment(units[i], old_assignment[i], WorkDetail::Failed);
		}
		else
			setAssignment(units[i], get_assign(old_assignment[i]), WorkDetail::NoChange);
	}
	unitDataChanged(_df.units->makeSelection(units));
}

bool WorkDetail::setId(dfproto::workdetailtest::WorkDetailId &id) const
{
	auto index = _df.work_details->find(*this);
	if (!index.isValid())
		return false;
	id.set_index(index.row());
	id.set_name(_wd->name);
	return true;
}

void WorkDetail::setProperties(const Properties &properties, const dfproto::workdetailtest::WorkDetailResult &r)
{
	if (!properties.name.isEmpty()) {
		_wd->name = df::toCP437(properties.name);
		_display_name = properties.name;
	}
	if (properties.mode) {
		const auto &res = r.mode();
		if (res.success()) {
			_wd->flags.bits.mode = *properties.mode;
		}
		else {
			qCWarning(DFHackLog) << "editWorkDetail failed" << res.error();
		}
	}
	if (properties.icon) {
		const auto &res = r.icon();
		if (res.success()) {
			_wd->icon = *properties.icon;
		}
		else {
			qCWarning(DFHackLog) << "editWorkDetail failed" << res.error();
		}
	}
	if (properties.no_modify)
		_wd->flags.bits.no_modify = *properties.no_modify;
	if (properties.cannot_be_everybody)
		_wd->flags.bits.cannot_be_everybody = *properties.cannot_be_everybody;
	for (std::size_t i = 0; i < properties.labors.size(); ++i) {
		const auto &labor_result = r.labors(i);
		const auto &[labor, enable] = properties.labors[i];
		if (labor_result.success()) {
			_wd->allowed_labors[labor] = enable;
		}
		else {
			qCWarning(DFHackLog) << "editWorkDetail failed" << labor_result.error();
		}
	}
}

QCoro::Task<> WorkDetail::edit(Properties changes)
{
	auto thisptr = shared_from_this(); // make sure the object live for the whole coroutine
	Q_ASSERT(thisptr);
	// Prepare arguments
	dfproto::workdetailtest::EditWorkDetail args;
	if (!setId(*args.mutable_id())) {
		qWarning() << "invalid work detail index";
		co_return;
	}
	changes.setArgs(*args.mutable_changes());
	// Call
	if (!_df.dfhack) {
		qCWarning(DFHackLog) << "DFHack client was deleted";
		co_return;
	}
	auto r = co_await EditWorkDetail(*_df.dfhack, args).first;
	// Check results
	if (!r) {
		qCWarning(DFHackLog) << "EditWorkDetail failed" << make_error_code(r.cr).message();
		co_return;
	}
	const auto &wd_result = r->work_detail();
	if (!wd_result.success()) {
		qCWarning(DFHackLog) << "EditWorkDetail failed" << wd_result.error();
		co_return;
	}
	// Apply changes
	setProperties(changes, *r);
	_df.work_details->updated(_df.work_details->find(*this));
}
