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

#include "AttributeModel.h"

#include "Unit.h"
#include "UnitDescriptors.h"
#include "DataRole.h"

#include <QColor>

using namespace UnitDetails;

AttributeModel::AttributeModel(const DwarfFortressData &df, QObject *parent):
	UnitDataModel(df, parent)
{
}

AttributeModel::~AttributeModel()
{
}

int AttributeModel::rowCount(const QModelIndex &parent) const
{
	if (_u)
		return df::physical_attribute_type::Count + df::mental_attribute_type::Count;
	else
		return 0;
}

int AttributeModel::columnCount(const QModelIndex &parent) const
{
	return static_cast<int>(Column::Count);
}

QVariant AttributeModel::data(const QModelIndex &index, int role) const
{
	Unit::attribute_t attr;
	if (unsigned(index.row()) < df::physical_attribute_type::Count)
		attr = static_cast<df::physical_attribute_type_t>(index.row());
	else
		attr = static_cast<df::mental_attribute_type_t>(index.row()
				- df::physical_attribute_type::Count);
	return visit([&](auto attr) -> QVariant {
		switch (static_cast<Column>(index.column())) {
		case Column::Attribute:
			switch (role) {
			case Qt::DisplayRole:
			case DataRole::SortRole:
				return UnitDescriptors::attributeName(attr);
			default:
				return {};
			}
		case Column::Value:
			switch (role) {
			case Qt::DisplayRole:
			case DataRole::SortRole:
				return _u->attributeValue(attr);
			case Qt::TextAlignmentRole:
				return Qt::AlignRight;
			default:
				return {};
			}
		case Column::Max:
			switch (role) {
			case Qt::DisplayRole:
			case DataRole::SortRole:
				if constexpr (std::same_as<decltype(attr), df::physical_attribute_type_t>)
					return (*_u)->physical_attrs[attr].max_value;
				else if constexpr (std::same_as<decltype(attr), df::mental_attribute_type_t>)
					if ((*_u)->current_soul)
						return (*_u)->current_soul->mental_attrs[attr].max_value;
				return {};
			case Qt::TextAlignmentRole:
				return Qt::AlignRight;
			default:
				return {};
			}
		case Column::Description:
			switch (role) {
			case Qt::DisplayRole:
			case Qt::ToolTipRole:
				return UnitDescriptors::attributeDescription(attr, _u->attributeCasteRating(attr));
			case Qt::ForegroundRole:
				if (_u->attributeCasteRating(attr) < 0)
					return QColor(Qt::red);
				else
					return {};
			default:
				return {};
			}
		default:
			return {};
		}
	}, attr);
}

QVariant AttributeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal)
		return {};
	if (role != Qt::DisplayRole)
		return {};
	switch (static_cast<Column>(section)) {
	case Column::Attribute:
		return tr("Attribute");
	case Column::Value:
		return tr("Value");
	case Column::Max:
		return tr("Max");
	case Column::Description:
		return tr("Description");
	default:
		return {};
	}
}
