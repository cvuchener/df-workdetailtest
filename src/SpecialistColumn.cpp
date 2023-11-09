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
	if (role != Qt::CheckStateRole)
		return {};
	return unit->flags4.bits.only_do_assigned_jobs
		? Qt::Checked
		: Qt::Unchecked;
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

Qt::ItemFlags SpecialistColumn::flags(int section, const Unit &unit) const
{
	if (unit.canAssignWork())
		return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
	else
		return {};
}

