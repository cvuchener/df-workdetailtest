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

#include "WorkDetailColumn.h"

#include "DwarfFortressData.h"
#include "WorkDetail.h"
#include "ObjectList.h"
#include "Unit.h"
#include "DataRole.h"
#include "Application.h"
#include "IconProvider.h"
#include "WorkDetailEditor.h"

#include <QIcon>
#include <QBrush>
#include <QMenu>
#include <QAction>


using namespace Columns;

WorkDetailColumn::WorkDetailColumn(DwarfFortressData &df, QObject *parent):
	AbstractColumn(parent),
	_df(df),
	_sort{*this, SortBy::Skill, {
			{SortBy::Skill, tr("skill")},
			{SortBy::Assigned, tr("assigned")}}}
{
	auto list = _df.work_details.get();
	connect(list, &QAbstractItemModel::rowsAboutToBeInserted,
		this, [this](const QModelIndex &, int first, int last) {
			columnsAboutToBeInserted(first, last);
		});
	connect(list, &QAbstractItemModel::rowsInserted,
		this, [this](const QModelIndex &, int first, int last) {
			columnsInserted(first, last);
		});
	connect(list, &QAbstractItemModel::rowsAboutToBeRemoved,
		this, [this](const QModelIndex &, int first, int last) {
			columnsAboutToBeRemoved(first, last);
		});
	connect(list, &QAbstractItemModel::rowsRemoved,
		this, [this](const QModelIndex &, int first, int last) {
			columnsRemoved(first, last);
		});
	connect(list, &QAbstractItemModel::dataChanged,
		this, [this](const QModelIndex &first, const QModelIndex &last, const QList<int> &) {
			columnDataChanged(first.row(), last.row());
		});
	connect(list, &ObjectListBase::unitDataChanged,
		this, [this](int row, int unit_id) {
			unitDataChanged(row, row, unit_id);
		});
}

WorkDetailColumn::~WorkDetailColumn()
{
}

int WorkDetailColumn::count() const
{
	return _df.work_details->rowCount();
}

QVariant WorkDetailColumn::headerData(int section, int role) const
{
	auto wd = _df.work_details->get(section);
	Q_ASSERT(wd);
	switch (role) {
	case Qt::DisplayRole:
		return wd->displayName();
	case Qt::DecorationRole:
		if ((*wd)->icon != df::work_detail_icon::ICON_NONE)
			return Application::instance()->icons().workdetail((*wd)->icon);
		else
			return {};
	default:
		return {};
	}
}

QVariant WorkDetailColumn::unitData(int section, const Unit &unit, int role) const
{
	static const QBrush Working = QColor(0, 255, 0, 64);
	static const QBrush NotWorking = QColor(255, 0, 0, 64);

	auto wd = _df.work_details->get(section);
	Q_ASSERT(wd);

	std::vector<const df::unit_skill *> skills;
	if (auto soul = unit->current_soul.get())
		for (const auto &skill: soul->skills)
			if (labor(skill->id) != df::unit_labor::NONE && (*wd)->allowed_labors.at(labor(skill->id)))
				skills.push_back(skill.get());
	switch (role) {
	case Qt::DisplayRole: {
		auto best_skill = std::ranges::max_element(skills, std::less{}, [](auto skill){return skill->rating;});
		if (best_skill == skills.end())
			return {};
		else
			return static_cast<int>((*best_skill)->rating);
	}
	case Qt::CheckStateRole:
		return wd->isAssigned(unit->id)
			? Qt::Checked : Qt::Unchecked;
	case Qt::BackgroundRole:
		if (!unit.canAssignWork())
			return {};
		switch ((*wd)->flags.bits.mode) {
		case df::work_detail_mode::EverybodyDoesThis:
			if (unit->flags4.bits.only_do_assigned_jobs && !wd->isAssigned(unit->id))
				return {};
			else
				return Working;
		case df::work_detail_mode::OnlySelectedDoesThis:
			if (wd->isAssigned(unit->id))
				return Working;
			else
				return {};
		case df::work_detail_mode::NobodyDoesThis:
			return NotWorking;
		}
	case Qt::ToolTipRole: {
		auto tooltip = tr("<h3>%1 - %2</h3>")
			.arg(unit.displayName())
			.arg(wd->displayName());
		if (!skills.empty()) {
			tooltip.append("<ul>");
			for (auto skill: skills)
				tooltip.append(tr("<li>%1 %2 (%3)</li>")
						.arg(QString::fromLocal8Bit(caption(skill->rating)))
						.arg(QString::fromLocal8Bit(caption_noun(skill->id)))
						.arg(int(skill->rating)));
			tooltip.append("</ul>");
		}
		return tooltip;
	}
	case DataRole::BorderRole: {
		switch (wd->status(unit->id)) {
		case WorkDetail::Pending:
			return QColor(Qt::gray);
		case WorkDetail::Failed:
			return QColor(Qt::red);
		default:
			return {};
		}
	}
	case DataRole::SortRole:
		switch (_sort.option) {
		case SortBy::Skill: {
			auto best_skill = std::ranges::max_element(skills, std::less{}, [](auto skill){return skill->rating;});
			if (best_skill == skills.end())
				return -1;
			else
				return static_cast<int>((*best_skill)->rating);
		}
		case SortBy::Assigned:
			return wd->isAssigned(unit->id);
		}
	default:
		return {};
	}
}

QVariant WorkDetailColumn::groupData(int section, GroupBy::Group group, std::span<const Unit *> units, int role) const
{
	auto wd = _df.work_details->get(section);
	Q_ASSERT(wd);

	auto is_assigned = [wd](const Unit *unit) { return wd->isAssigned((*unit)->id); };

	int count = std::ranges::count_if(units, is_assigned);

	switch (role) {
	case Qt::DisplayRole: {
		return count;
	}
	case Qt::CheckStateRole:
		return std::ranges::all_of(units, is_assigned)
			? Qt::Checked
			: std::ranges::none_of(units, is_assigned)
				? Qt::Unchecked
				: Qt::PartiallyChecked;
	case Qt::ToolTipRole: {
		auto tooltip = tr("<h3>%1 - %2</h3>")
			.arg(group.name())
			.arg(wd->displayName());
		if (count > 0) {
			tooltip.append(tr("<p>%1 assigned</p>").arg(count));
			tooltip.append("<ul>");
			for (auto unit: units)
				if (is_assigned(unit))
					tooltip.append(tr("<li>%1</li>")
							.arg(unit->displayName()));
			tooltip.append("</ul>");
		}
		else
			tooltip.append(tr("<p>No one is assigned</p>"));
		return tooltip;
	}
	case DataRole::SortRole:
		switch (_sort.option) {
		case SortBy::Skill: {
			auto best_rating = std::numeric_limits<int>::min();
			for (auto unit: units) {
				if (auto soul = (*unit)->current_soul.get())
					for (const auto &skill: soul->skills)
						if (labor(skill->id) != df::unit_labor::NONE
								&& (*wd)->allowed_labors.at(labor(skill->id))
								&& skill->rating > best_rating)
							best_rating = skill->rating;
			}
			return best_rating;
		}
		case SortBy::Assigned:
			return count;
		}
	default:
		return {};
	}
}

bool WorkDetailColumn::setUnitData(int section, Unit &unit, const QVariant &value, int role)
{
	if (role != Qt::CheckStateRole)
		return false;
	if (!unit.canAssignWork())
		return false;
	auto work_detail = _df.work_details->get(section);
	work_detail->assign(unit->id, value.toBool());
	return true;
}

bool WorkDetailColumn::setGroupData(int section, std::span<Unit *> units, const QVariant &value, int role)
{
	if (role != Qt::CheckStateRole)
		return false;
	auto work_detail = _df.work_details->get(section);
	std::vector<int> unit_ids;
	for (auto unit: units) {
		if (unit->canAssignWork())
			unit_ids.push_back((*unit)->id);
	}
	work_detail->assign(std::move(unit_ids), value.toBool());
	return true;
}

void WorkDetailColumn::toggleUnits(int section, std::span<Unit *> units)
{
	auto work_detail = _df.work_details->get(section);
	std::vector<int> unit_ids;
	for (auto unit: units) {
		if (unit->canAssignWork())
			unit_ids.push_back((*unit)->id);
	}
	work_detail->toggle(std::move(unit_ids));
}

Qt::ItemFlags WorkDetailColumn::unitFlags(int section, const Unit &unit) const
{
	if (unit.canAssignWork())
		return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
	else
		return {};
}

Qt::ItemFlags WorkDetailColumn::groupFlags(int section, std::span<const Unit *> units) const
{
	if (std::ranges::any_of(units, [](const Unit *unit) { return unit->canAssignWork(); }))
		return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
	else
		return {};
}

void WorkDetailColumn::makeHeaderMenu(int section, QMenu *menu, QWidget *parent)
{
	using namespace df::work_detail_mode;
	auto wd = _df.work_details->get(section);

	menu->addSeparator();
	auto edit_action = new QAction(tr("Edit %1...").arg(wd->displayName()), menu);
	menu->addAction(edit_action);

	connect(edit_action, &QAction::triggered, [wd, parent]() {
		WorkDetailEditor editor(parent);
		editor.setName(wd->displayName());
		editor.setMode(static_cast<work_detail_mode>((*wd)->flags.bits.mode));
		editor.setIcon((*wd)->icon);
		if (QDialog::Accepted == editor.exec()) {
			wd->edit({
				.name = editor.name(),
				.mode = editor.mode(),
				.icon = editor.icon(),
			});
		}
	});

	_sort.makeSortMenu(menu);

	auto make_mode_action = [menu, wd]<work_detail_mode Mode>(const QString &name) {
		auto action = new QAction(name, menu);
		action->setCheckable(true);
		action->setChecked(static_cast<work_detail_mode>((*wd)->flags.bits.mode) == Mode);
		connect(action, &QAction::triggered, [wd]() {
			wd->edit({ .mode = Mode });
		});
		return action;
	};

	menu->addSection(tr("Mode"));
	menu->addActions({
		make_mode_action.operator()<EverybodyDoesThis>(tr("Everybody does this")),
		make_mode_action.operator()<NobodyDoesThis>(tr("Nobody does this")),
		make_mode_action.operator()<OnlySelectedDoesThis>(tr("Only selected does this")),
	});
}

Factory WorkDetailColumn::makeFactory(const QJsonObject &)
{
	return [](DwarfFortressData &df, QPointer<DFHack::Client>) {
		return std::make_unique<WorkDetailColumn>(df);
	};
}
