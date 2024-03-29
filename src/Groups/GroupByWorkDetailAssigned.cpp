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

#include "GroupByWorkDetailAssigned.h"

#include <QCoreApplication>
#include "DwarfFortressData.h"
#include "Unit.h"
#include "WorkDetail.h"
#include "WorkDetailModel.h"

using namespace Groups;

GroupByWorkDetailAssigned::GroupByWorkDetailAssigned(const DwarfFortressData &df):
	_df(df)
{
}

GroupByWorkDetailAssigned::~GroupByWorkDetailAssigned()
{
}

quint64 GroupByWorkDetailAssigned::unitGroup(const Unit &unit) const
{
	quint64 count = 0;
	for (int i = 0; i < _df.work_details->rowCount(); ++i)
		if (_df.work_details->get(i)->isAssigned(unit->id))
			++count;
	return count;
}

QString GroupByWorkDetailAssigned::groupName(quint64 count) const
{
	return QCoreApplication::translate("GroupByWorkDetailAssigned", "assigned to %1 work details").arg(count);
}

QVariant GroupByWorkDetailAssigned::sortValue(quint64 count) const
{
	return count;
}
