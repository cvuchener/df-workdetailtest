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
#include "ObjectList.h"
#include <QCoroFuture>

#include "workdetailtest.pb.h"
#include <dfhack-client-qt/Function.h>

static const DFHack::Function<
	dfproto::workdetailtest::EditWorkDetail,
	dfproto::workdetailtest::WorkDetailResult
> EditWorkDetail = {"workdetailtest", "EditWorkDetail"};
static const DFHack::Function<
	dfproto::workdetailtest::AddWorkDetail,
	dfproto::workdetailtest::WorkDetailResult
> AddWorkDetail = {"workdetailtest", "AddWorkDetail"};

WorkDetail::WorkDetail(std::unique_ptr<df::work_detail> &&work_detail, DwarfFortressData &df, DFHack::Client &dfhack, QObject *parent):
	QObject(parent),
	_wd(std::move(work_detail)),
	_df(df),
	_dfhack(&dfhack)
{
	refresh();
}

WorkDetail::~WorkDetail()
{
}

void WorkDetail::update(std::unique_ptr<df::work_detail> &&work_detail)
{
	aboutToBeUpdated();
	_wd = std::move(work_detail);
	refresh();
	updated();
}

void WorkDetail::refresh()
{
	_display_name = df::fromCP437(_wd->name);
	_statuses.clear();
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
	bool status_changed;
	if (status == NoChange)
		status_changed = 0 != _statuses.erase(unit_id);
	else {
		auto [sit, inserted] = _statuses.try_emplace(unit_id, NoChange);
		status_changed = sit->second != status;
		sit->second = status;
	}
	if (assign != assigned || status_changed)
		unitDataChanged(unit_id);
}

std::vector<std::pair<df::unit_labor_t, bool>> WorkDetail::Properties::fromLabors(std::span<const bool, df::unit_labor::Count> labors)
{
	std::vector<std::pair<df::unit_labor_t, bool>> l;
	l.reserve(df::unit_labor::Count);
	for (int i = 0; i < df::unit_labor::Count; ++i)
		l.emplace_back(static_cast<df::unit_labor_t>(i), labors[i]);
	return l;
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
	auto index = _df.work_details->find(*this);
	if (!index.isValid()) {
		qWarning() << "invalid work detail index";
		co_return;
	}
	std::vector<int8_t> old_assignment(units.size());
	std::ranges::transform(units, old_assignment.begin(), [this](int id) { return isAssigned(id); });
	dfproto::workdetailtest::EditWorkDetail args;
	args.mutable_id()->set_index(index.row());
	args.mutable_id()->set_name(_wd->name);
	for (std::size_t i = 0; i < units.size(); ++i) {
		auto assignment = args.mutable_changes()->mutable_assignments()->Add();
		assignment->set_unit_id(units[i]);
		auto assign = get_assign(old_assignment[i]);
		assignment->set_enable(assign);
		setAssignment(units[i], assign, WorkDetail::Pending);
	}
	// Call
	if (!_dfhack) {
		qCWarning(DFHackLog) << "DFHack client was deleted";
		co_return;
	}
	auto r = co_await EditWorkDetail(*_dfhack, args).first;
	// Check results
	if (!r) {
		qCWarning(DFHackLog) << "editWorkDetail failed" << make_error_code(r.cr).message();
		for (std::size_t i = 0; i < units.size(); ++i)
			setAssignment(units[i], old_assignment[i], WorkDetail::Failed);
		co_return;
	}
	const auto &wd_result = r->work_detail();
	if (!wd_result.success()) {
		qCWarning(DFHackLog) << "editWorkDetail failed" << wd_result.error();
		for (std::size_t i = 0; i < units.size(); ++i)
			setAssignment(units[i], old_assignment[i], WorkDetail::Failed);
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
}

static void initPropertiesArgs(dfproto::workdetailtest::WorkDetailProperties &args, const WorkDetail::Properties &props)
{
	if (!props.name.isEmpty())
		args.set_name(df::toCP437(props.name));
	if (props.mode) {
		switch (*props.mode) {
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
	if (props.icon)
		args.set_icon(static_cast<int>(*props.icon));
	for (const auto &[labor, enable]: props.labors) {
		auto labor_change = args.mutable_labors()->Add();
		labor_change->set_labor(labor);
		labor_change->set_enable(enable);
	}
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
	auto index = _df.work_details->find(*this);
	if (!index.isValid()) {
		qWarning() << "invalid work detail index";
		co_return;
	}
	dfproto::workdetailtest::EditWorkDetail args;
	args.mutable_id()->set_index(index.row());
	args.mutable_id()->set_name(_wd->name);
	initPropertiesArgs(*args.mutable_changes(), changes);
	// Call
	if (!_dfhack) {
		qCWarning(DFHackLog) << "DFHack client was deleted";
		co_return;
	}
	auto r = co_await EditWorkDetail(*_dfhack, args).first;
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
	aboutToBeUpdated();
	setProperties(changes, *r);
	updated();
}

QCoro::Task<> WorkDetail::makeNewWorkDetail(std::shared_ptr<DwarfFortressData> df, QPointer<DFHack::Client> dfhack, Properties properties)
{
	// Prepare arguments
	dfproto::workdetailtest::AddWorkDetail args;
	initPropertiesArgs(*args.mutable_properties(), properties);
	// Call
	if (!dfhack) {
		qCWarning(DFHackLog) << "DFHack client was deleted";
		co_return;
	}
	auto r = co_await AddWorkDetail(*dfhack, args).first;
	// Check results
	if (!r) {
		qCWarning(DFHackLog) << "AddWorkDetail failed" << make_error_code(r.cr).message();
		co_return;
	}
	const auto &wd_result = r->work_detail();
	if (!wd_result.success()) {
		qCWarning(DFHackLog) << "AddWorkDetail failed" << wd_result.error();
		co_return;
	}
	// Apply changes
	auto wd = std::make_shared<WorkDetail>(std::make_unique<df::work_detail>(), *df, *dfhack);
	wd->setProperties(properties, *r);
	df->work_details->push(std::move(wd));
}
