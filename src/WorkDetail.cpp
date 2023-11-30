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

#include "CP437.h"
#include "DwarfFortress.h"
#include "ObjectList.h"
#include <QCoroFuture>

#include <dfhack-client-qt/Function.h>

static const DFHack::Function<
	dfproto::workdetailtest::WorkDetailChanges,
	dfproto::workdetailtest::WorkDetailResults
> EditWorkDetails = {"workdetailtest", "EditWorkDetails"};
static const DFHack::Function<
	dfproto::workdetailtest::WorkDetailProperties,
	dfproto::workdetailtest::WorkDetailResult
> EditWorkDetail = {"workdetailtest", "EditWorkDetail"};

WorkDetail::WorkDetail(std::unique_ptr<df::work_detail> &&work_detail, DwarfFortress &df, QObject *parent):
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
	aboutToBeUpdated();
	_wd = std::move(work_detail);
	refresh();
	updated();
}

void WorkDetail::refresh()
{
	_display_name = fromCP437(_wd->name);
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

template<typename F>
QCoro::Task<> WorkDetail::changeAssignments(std::vector<int> units, F get_assign)
{
	auto thisptr = shared_from_this(); // make sure the object live for the whole coroutine
	Q_ASSERT(thisptr);
	auto index = _df.workDetails().find(*this);
	if (!index.isValid()) {
		qWarning() << "invalid work detail index";
		co_return;
	}
	std::vector<int8_t> old_assignment(units.size());
	std::ranges::transform(units, old_assignment.begin(), [this](int id) { return isAssigned(id); });
	dfproto::workdetailtest::WorkDetailProperties args;
	args.set_work_detail_index(index.row());
	args.set_work_detail_name(_wd->name);
	for (std::size_t i = 0; i < units.size(); ++i) {
		auto assignment = args.mutable_assignment_changes()->Add();
		assignment->set_unit_id(units[i]);
		auto assign = get_assign(old_assignment[i]);
		assignment->set_enable(assign);
		setAssignment(units[i], assign, WorkDetail::Pending);
	}
	auto r = co_await EditWorkDetail(_df.dfhack(), args).first;
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

QCoro::Task<> WorkDetail::edit(Properties changes)
{
	auto thisptr = shared_from_this(); // make sure the object live for the whole coroutine
	Q_ASSERT(thisptr);
	using namespace dfproto::workdetailtest;
	auto index = _df.workDetails().find(*this);
	if (!index.isValid()) {
		qWarning() << "invalid work detail index";
		co_return;
	}
	WorkDetailProperties args;
	args.set_work_detail_index(index.row());
	args.set_work_detail_name(_wd->name);
	if (!changes.name.isEmpty())
		args.set_new_name(toCP437(changes.name));
	if (changes.mode) {
		switch (*changes.mode) {
		case df::work_detail_mode::EverybodyDoesThis:
			args.set_new_mode(EverybodyDoesThis);
			break;
		case df::work_detail_mode::NobodyDoesThis:
			args.set_new_mode(NobodyDoesThis);
			break;
		case df::work_detail_mode::OnlySelectedDoesThis:
			args.set_new_mode(OnlySelectedDoesThis);
			break;
		default:
			break;
		}
	}
	if (changes.icon)
		args.set_new_icon(static_cast<int>(*changes.icon));
	auto r = co_await EditWorkDetail(_df.dfhack(), args).first;
	if (!r) {
		qCWarning(DFHackLog) << "editWorkDetail failed" << make_error_code(r.cr).message();
		co_return;
	}
	const auto &wd_result = r->work_detail();
	if (!wd_result.success()) {
		qCWarning(DFHackLog) << "editWorkDetail failed" << wd_result.error();
		co_return;
	}
	aboutToBeUpdated();
	if (!changes.name.isEmpty()) {
		_wd->name = toCP437(changes.name);
		_display_name = changes.name;
	}
	if (changes.mode) {
		const auto &res = r->mode();
		if (res.success()) {
			_wd->flags.bits.mode = *changes.mode;
		}
		else {
			qCWarning(DFHackLog) << "editWorkDetail failed" << res.error();
		}
	}
	if (changes.icon) {
		const auto &res = r->icon();
		if (res.success()) {
			_wd->icon = *changes.icon;
		}
		else {
			qCWarning(DFHackLog) << "editWorkDetail failed" << res.error();
		}
	}
	updated();
}
