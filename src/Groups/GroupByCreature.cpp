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

#include "GroupByCreature.h"

#include "df/utils.h"
#include "DwarfFortressData.h"
#include "Unit.h"

using namespace Groups;

GroupByCreature::GroupByCreature(const DwarfFortressData &df):
	_df(df)
{
}

GroupByCreature::~GroupByCreature()
{
}

quint64 GroupByCreature::unitGroup(const Unit &unit) const
{
	return unit->race;
}

QString GroupByCreature::groupName(quint64 race_id) const
{
	Q_ASSERT(race_id < _df.raws->creatures.all.size());
	return df::fromCP437(_df.raws->creatures.all[race_id]->name[0]);
}
