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

#include "AbstractColumn.h"

#include <QVariant>

AbstractColumn::AbstractColumn(QObject *parent):
	QObject(parent)
{
}

AbstractColumn::~AbstractColumn()
{
}

int AbstractColumn::count() const
{
	return 1;
}

QVariant AbstractColumn::groupData(int, GroupBy::Group, std::span<const Unit *>, int) const
{
	return {};
}

bool AbstractColumn::setUnitData(int, Unit &, const QVariant &, int)
{
	return false;
}

bool AbstractColumn::setGroupData(int, std::span<Unit *>, const QVariant &, int)
{
	return false;
}

void AbstractColumn::toggleUnits(int section, std::span<Unit *> units)
{
	for (auto u: units) {
		auto state = unitData(section, *u, Qt::CheckStateRole).value<Qt::CheckState>();
		setUnitData(section, *u, state == Qt::Checked ? Qt::Unchecked : Qt::Checked, Qt::CheckStateRole);
	}
}

Qt::ItemFlags AbstractColumn::unitFlags(int, const Unit &) const
{
	return Qt::ItemIsEnabled;
}

Qt::ItemFlags AbstractColumn::groupFlags(int, std::span<const Unit *>) const
{
	return Qt::ItemIsEnabled;
}

void AbstractColumn::makeUnitMenu(int, Unit &, QMenu *, QWidget *)
{
}

void AbstractColumn::makeHeaderMenu(int, QMenu *, QWidget *)
{
}
