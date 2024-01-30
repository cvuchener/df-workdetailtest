/*
 * Copyright 2024 Clement Vuchener
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

#include "UnitDataModel.h"

#include "ObjectList.h"
#include "Unit.h"
#include "DwarfFortressData.h"

using namespace UnitDetails;

UnitDataModel::UnitDataModel(const DwarfFortressData &df, QObject *parent):
	QAbstractTableModel(parent),
	_df(df),
	_u(nullptr)
{
}

UnitDataModel::~UnitDataModel()
{
}

void UnitDataModel::setUnit(const Unit *unit)
{
	beginResetModel();
	if (_u) {
		_u->disconnect(this);
		_u = nullptr;
	}
	_u = unit;
	if (_u) {
		connect(_df.units.get(), &QAbstractItemModel::dataChanged,
			this, [this](const QModelIndex &top_left, const QModelIndex &bottom_right, const QList<int> &roles) {
				auto current = _df.units->find(*_u);
				if (QItemSelectionRange(top_left, bottom_right).contains(current)) {
					beginResetModel();
					endResetModel();
				}
			});
	}
	endResetModel();
}

