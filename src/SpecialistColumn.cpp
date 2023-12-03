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

#include "SpecialistColumn.h"

#include "Unit.h"
#include "DwarfFortress.h"
#include "DataRole.h"

#include <QVariant>

SpecialistColumn::SpecialistColumn(DwarfFortress &df, QObject *parent):
	AbstractColumn(parent),
	_df(df)
{
}

SpecialistColumn::~SpecialistColumn()
{
}

QVariant SpecialistColumn::headerData(int section, int role) const
{
	if (role != Qt::DisplayRole)
		return {};
	return tr("Specialist");
}

QVariant SpecialistColumn::unitData(int section, const Unit &unit, int role) const
{
	switch (role) {
	case Qt::CheckStateRole:
	case DataRole::SortRole:
		return unit->flags4.bits.only_do_assigned_jobs
			? Qt::Checked
			: Qt::Unchecked;
	default:
		return {};
	}
}

QVariant SpecialistColumn::groupData(int section, GroupBy::Group group, std::span<const Unit *> units, int role) const
{
	auto is_specialist = [](const Unit *unit) -> bool {
		return (*unit)->flags4.bits.only_do_assigned_jobs;
	};

	switch (role) {
	case Qt::DisplayRole:
		return int(std::ranges::count_if(units, is_specialist));
	case Qt::CheckStateRole:
		return std::ranges::all_of(units, is_specialist)
			? Qt::Checked
			: std::ranges::none_of(units, is_specialist)
				? Qt::Unchecked
				: Qt::PartiallyChecked;
	case Qt::ToolTipRole: {
		auto tooltip = tr("<h2>%1</h2>").arg(group.name());
		auto count = std::ranges::count_if(units, is_specialist);
		if (count > 0) {
			tooltip.append(tr("<p>%1 specialists</p>").arg(count));
			tooltip.append("<ul>");
			for (auto unit: units)
				if (is_specialist(unit))
					tooltip.append(tr("<li>%1</li>")
							.arg(unit->displayName()));
			tooltip.append("</ul>");
		}
		else
			tooltip.append(tr("<p>There is no specialist</p>"));
		return tooltip;
	}
	case DataRole::SortRole:
		return int(std::ranges::count_if(units, is_specialist));
	default:
		return {};
	}
}

bool SpecialistColumn::setUnitData(int section, Unit &unit, const QVariant &value, int role)
{
	if (role != Qt::CheckStateRole)
		return false;
	if (!unit.canAssignWork())
		return false;
	unit.edit({ .only_do_assigned_jobs = value.value<Qt::CheckState>() == Qt::Checked });
	return true;
}

bool SpecialistColumn::setGroupData(int section, std::span<Unit *> units, const QVariant &value, int role)
{
	if (role != Qt::CheckStateRole)
		return false;
	for (auto unit: units) {
		if (!unit->canAssignWork())
			continue;
		unit->edit({ .only_do_assigned_jobs = value.value<Qt::CheckState>() == Qt::Checked });
	}
	return true;
}

Qt::ItemFlags SpecialistColumn::unitFlags(int section, const Unit &unit) const
{
	if (unit.canAssignWork())
		return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
	else
		return {};
}

Qt::ItemFlags SpecialistColumn::groupFlags(int section, std::span<const Unit *> units) const
{
	if (std::ranges::any_of(units, [](const Unit *unit) { return unit->canAssignWork(); }))
		return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
	else
		return {};
}

