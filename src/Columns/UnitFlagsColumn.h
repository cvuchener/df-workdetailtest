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

#ifndef COLUMNS_UNIT_FLAGS_COLUMN_H
#define COLUMNS_UNIT_FLAGS_COLUMN_H

#include "AbstractColumn.h"
#include "Columns/Factory.h"
#include "Unit.h"

namespace Columns {

class UnitFlagsColumn: public AbstractColumn
{
	Q_OBJECT
public:
	UnitFlagsColumn(std::span<const Unit::Flag> flags, DwarfFortressData &df, QObject *parent = nullptr);
	~UnitFlagsColumn() override;

	int count() const override;
	QVariant headerData(int section, int role = Qt::DisplayRole) const override;
	QVariant unitData(int section, const Unit &unit, int role = Qt::DisplayRole) const override;
	QVariant groupData(int section, GroupBy::Group group, std::span<const Unit *> units, int role = Qt::DisplayRole) const override;
	bool setUnitData(int section, Unit &unit, const QVariant &value, int role = Qt::EditRole) override;
	bool setGroupData(int section, std::span<Unit *> units, const QVariant &value, int role = Qt::EditRole) override;
	void toggleUnits(int section, std::span<Unit *> units) override;
	Qt::ItemFlags unitFlags(int section, const Unit &unit) const override;
	Qt::ItemFlags groupFlags(int section, std::span<const Unit *> units) const override;

	static Factory makeFactory(const QJsonObject &);

private:
	static QString title(Unit::Flag flag);
	static QString countText(Unit::Flag flag, int count);
	std::vector<Unit::Flag> _flags;
	DwarfFortressData &_df;
};

}

#endif
