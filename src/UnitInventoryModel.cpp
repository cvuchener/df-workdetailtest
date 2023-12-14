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

#include "UnitInventoryModel.h"

#include "Unit.h"
#include "df/items.h"
#include "df/utils.h"
#include "DwarfFortressData.h"

UnitInventoryModel::UnitInventoryModel(const DwarfFortressData &df, QObject *parent):
	QAbstractTableModel(parent),
	_df(df),
	_u(nullptr)
{
}

UnitInventoryModel::~UnitInventoryModel()
{
}

void UnitInventoryModel::setUnit(const Unit *unit)
{
	beginResetModel();
	if (_u) {
		_u->disconnect(this);
		_u = nullptr;
	}
	_u = unit;
	if (_u) {
		connect(_u, &Unit::aboutToBeUpdated,
			this, [this]() { beginResetModel(); });
		connect(_u, &Unit::updated, this,
			[this]() { endResetModel(); });
	}
	endResetModel();
}

int UnitInventoryModel::rowCount(const QModelIndex &parent) const
{
	if (_u)
		return (*_u)->inventory.size();
	else
		return 0;
}

int UnitInventoryModel::columnCount(const QModelIndex &parent) const
{
	return static_cast<int>(Column::Count);
}

struct ItemDescriptionGenerator
{
	const DwarfFortressData &df;

	template <std::derived_from<df::item> T>
	QString operator()(const T &item) const
	{
		using df::fromCP437;
		QStringList out;

		std::size_t stack_size = 1;
		if constexpr (df::ItemHasStackSize<T>)
			stack_size = item.stack_size;

		df::matter_state_t state = df::matter_state::Solid;
		if constexpr (std::derived_from<T, df::item_liquid>)
			state = df::matter_state::Liquid;
		if constexpr (std::derived_from<T, df::item_powder>)
			state = df::matter_state::Powder;
		if constexpr (std::derived_from<T, df::item_liquipowder>) {
			if (item.mat_state.bits.pressed)
				state = df::matter_state::Pressed;
			else if (item.mat_state.bits.paste)
				state = df::matter_state::Paste;
		}

		if constexpr (df::ItemHasItemDef<T>)
			if constexpr (df::ItemDefHasPrePlural<df::item_itemdef_t<T>>)
				if (stack_size > 1 && !item.subtype->name_preplural.empty()) {
					out.append(fromCP437(item.subtype->name_preplural));
				}
		if constexpr (df::ItemHasItemDef<T>)
			if constexpr (df::ItemDefHasAdjective<df::item_itemdef_t<T>>)
				if (!item.subtype->adjective.empty()) {
					out.append(fromCP437(item.subtype->adjective));
				}
		if constexpr (df::ItemHasMaterial<T>) {
			auto [material, mat_origin] = df.findMaterial(item.mat_type, item.mat_index);
			if (material) {
				if (auto hf = get_if<const df::historical_figure *>(&mat_origin)) {
					out.append(df.raws->language.translate_name((*hf)->name) + "'s");
				}
				if (!material->prefix.empty()) {
					out.append(fromCP437(material->prefix));
				}
				out.append(fromCP437(material->state_adj[state]));
			}
			else {
				if constexpr (df::ItemHasItemDef<T>)
					if constexpr (df::ItemDefHasMatPlaceholder<df::item_itemdef_t<T>>)
						if (!item.subtype->material_placeholder.empty())
							out.append(fromCP437(item.subtype->material_placeholder));
			}
		}
		if constexpr (df::ItemHasItemDef<T>) {
			if constexpr (df::ItemDefHasName<df::item_itemdef_t<T>>) {
				if constexpr (df::ItemDefHasPlural<df::item_itemdef_t<T>>) {
					if (stack_size > 1)
						out.append(fromCP437(item.subtype->name_plural));
					else
						out.append(fromCP437(item.subtype->name));
				}
				else
					out.append(fromCP437(item.subtype->name));
			}
		}
		else if constexpr (requires { { T::item_type } -> std::convertible_to<df::item_type_t>; })
			out.append(QString::fromLocal8Bit(caption(T::item_type)));
		else
			out.append("item");
		return out.join(' ');
	}
};

QVariant UnitInventoryModel::data(const QModelIndex &index, int role) const
{
	auto item = (*_u)->inventory.at(index.row()).get();
	switch (static_cast<Column>(index.column())) {
	case Column::Item:
		switch (role) {
		case Qt::DisplayRole:
			return df::visit_item(ItemDescriptionGenerator{_df}, *item->item);
		default:
			return {};
		}
	case Column::Mode:
		switch (role) {
		case Qt::DisplayRole:
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

QVariant UnitInventoryModel::headerData(int section, Qt::Orientation orientation, int role) const
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
