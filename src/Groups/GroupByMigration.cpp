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

#include "GroupByMigration.h"

#include "DwarfFortress.h"
#include "Unit.h"

#include <QCoreApplication>
#include <QVariant>

using namespace Groups;

GroupByMigration::GroupByMigration(const DwarfFortress &df):
	_df(df)
{
}

GroupByMigration::~GroupByMigration()
{
}

quint64 GroupByMigration::unitGroup(const Unit &unit) const
{
	auto birth = df::time(unit->birth_year) + unit->birth_tick;
	auto arrival = _df.currentTime() - unit->time_on_site;
	return duration_cast<df::season>(arrival).count() * 2 + (birth == arrival ? 1 : 0);

}

QString GroupByMigration::groupName(quint64 id) const
{
	bool born_on_site = id % 2;
	auto date = df::date<df::year, df::season>(df::season(id/2));
	if (born_on_site)
		return QCoreApplication::translate("GroupByMigration", "Born in the %1 of the year %2")
			.arg(df::Seasons[get<df::season>(date).count()])
			.arg(get<df::year>(date).count());
	else
		return QCoreApplication::translate("GroupByMigration", "Arrived in the %1 of the year %2")
			.arg(df::Seasons[get<df::season>(date).count()])
			.arg(get<df::year>(date).count());
}

QVariant GroupByMigration::sortValue(quint64 group_id) const
{
	return group_id;
}
