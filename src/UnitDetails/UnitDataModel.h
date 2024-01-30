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

#ifndef UNIT_DETAILS_UNIT_DATA_MODEL_H
#define UNIT_DETAILS_UNIT_DATA_MODEL_H

#include <QAbstractTableModel>

class DwarfFortressData;
class Unit;

namespace UnitDetails
{

class UnitDataModel: public QAbstractTableModel
{
	Q_OBJECT
public:
	UnitDataModel(const DwarfFortressData &df, QObject *parent = nullptr);
	~UnitDataModel() override;

	void setUnit(const Unit *unit);

protected:
	const DwarfFortressData &_df;
	const Unit *_u;
};

} // namespace UnitDetails

#endif

