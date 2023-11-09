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

#ifndef UNIT_INVENTORY_MODEL_H
#define UNIT_INVENTORY_MODEL_H

#include <QAbstractTableModel>

class DwarfFortress;
class Unit;

class UnitInventoryModel: public QAbstractTableModel
{
	Q_OBJECT
public:
	UnitInventoryModel(const DwarfFortress &df, QObject *parent = nullptr);
	~UnitInventoryModel() override;

	void setUnit(const Unit *unit);

	enum class Column {
		Item = 0,
		Mode,
		Count,
	};

	int rowCount(const QModelIndex &parent = {}) const override;
	int columnCount(const QModelIndex &parent = {}) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
	const DwarfFortress &_df;
	const Unit *_u;
};

#endif
