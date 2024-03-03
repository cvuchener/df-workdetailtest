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

#include "InventoryModel.h"

#include "Unit.h"
#include "df/items.h"
#include "df/utils.h"
#include "DwarfFortressData.h"
#include "DataRole.h"
#include "Material.h"
#include "Item.h"

using namespace UnitDetails;

InventoryModel::InventoryModel(const DwarfFortressData &df, QObject *parent):
	UnitDataModel(df, parent)
{
}

InventoryModel::~InventoryModel()
{
}

int InventoryModel::rowCount(const QModelIndex &parent) const
{
	if (_u)
		return (*_u)->inventory.size();
	else
		return 0;
}

int InventoryModel::columnCount(const QModelIndex &parent) const
{
	return static_cast<int>(Column::Count);
}

QVariant InventoryModel::data(const QModelIndex &index, int role) const
{
	auto item = (*_u)->inventory.at(index.row()).get();
	switch (static_cast<Column>(index.column())) {
	case Column::Item:
		switch (role) {
		case Qt::DisplayRole:
		case DataRole::SortRole:
			return Item::toString(_df, *item->item);
		default:
			return {};
		}
	case Column::Mode:
		switch (role) {
		case Qt::DisplayRole:
		case DataRole::SortRole:
			switch (item->mode) {
			case df::unit_inventory_item_mode::Hauled: return tr("Hauled");
			case df::unit_inventory_item_mode::Weapon: return tr("Weapon");
			case df::unit_inventory_item_mode::Worn: return tr("Worn");
			case df::unit_inventory_item_mode::Piercing: return tr("Piercing");
			case df::unit_inventory_item_mode::Flask: return tr("Flask");
			case df::unit_inventory_item_mode::WrappedAround: return tr("Wrapped around");
			case df::unit_inventory_item_mode::StuckIn: return tr("Stuck in");
			case df::unit_inventory_item_mode::InMouth: return tr("In mouth");
			case df::unit_inventory_item_mode::Pet: return tr("Pet");
			case df::unit_inventory_item_mode::SewnInto: return tr("Sewn into");
			case df::unit_inventory_item_mode::Strapped: return tr("Strapped");
			default: return tr("Unknown");
			}
		default:
			return {};
		}
	default:
		return {};
	}
}

QVariant InventoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal)
		return {};
	if (role != Qt::DisplayRole)
		return {};
	switch (static_cast<Column>(section)) {
	case Column::Item:
		return tr("Item");
	case Column::Mode:
		return tr("Mode");
	default:
		return {};
	}
}
