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

#ifndef WORK_DETAIL_COLUMN_H
#define WORK_DETAIL_COLUMN_H

#include "AbstractColumn.h"

class DwarfFortress;

class WorkDetailColumn: public AbstractColumn
{
	Q_OBJECT
public:
	WorkDetailColumn(DwarfFortress &df, QObject *parent = nullptr);
	~WorkDetailColumn() override;

	int count() const override;
	QVariant headerData(int section, int role = Qt::DisplayRole) const override;
	QVariant unitData(int section, const Unit &unit, int role = Qt::DisplayRole) const override;
	bool setUnitData(int section, Unit &unit, const QVariant &value, int role = Qt::EditRole) override;
	Qt::ItemFlags flags(int section, const Unit &unit) const override;

	void makeHeaderMenu(int section, QMenu *menu, QWidget *parent) override;

private:
	DwarfFortress &_df;
};

#endif
