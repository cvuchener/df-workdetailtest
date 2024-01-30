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

#ifndef UNIT_DETAILS_INVENTORY_MODEL_H
#define UNIT_DETAILS_INVENTORY_MODEL_H

#include "UnitDataModel.h"

namespace UnitDetails
{

class InventoryModel: public UnitDataModel
{
	Q_OBJECT
public:
	InventoryModel(const DwarfFortressData &df, QObject *parent = nullptr);
	~InventoryModel() override;

	enum class Column {
		Item = 0,
		Mode,
		Count,
	};

	int rowCount(const QModelIndex &parent = {}) const override;
	int columnCount(const QModelIndex &parent = {}) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
};

} // namespace UnitDetails

#endif
