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

#include "UnitFilterProxyModel.h"

#include "Unit.h"
#include "ObjectList.h"

UnitFilterProxyModel::UnitFilterProxyModel(QObject *parent)
	: QSortFilterProxyModel(parent)
	, _units(nullptr)
{
}

UnitFilterProxyModel::~UnitFilterProxyModel()
{
}

void UnitFilterProxyModel::setSourceModel(QAbstractItemModel *source_model)
{
	_units = dynamic_cast<ObjectList<Unit> *>(source_model);
	QSortFilterProxyModel::setSourceModel(_units);
}

const Unit *UnitFilterProxyModel::get(int row) const
{
	auto source = mapToSource(index(row, 0));
	return _units->get(source.row());
}

Unit *UnitFilterProxyModel::get(int row)
{
	auto source = mapToSource(index(row, 0));
	return _units->get(source.row());
}

QModelIndex UnitFilterProxyModel::find(int unit_id) const
{
	return mapFromSource(_units->find(unit_id));
}

bool UnitFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	if (source_parent.isValid())
		return true;
	if (auto unit = _units->get(source_row)) {
		return _unit_filter(*unit);
	}
	else
		return true;
}
