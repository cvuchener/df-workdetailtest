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

#ifndef UNIT_DETAILS_DOCK_H
#define UNIT_DETAILS_DOCK_H

#include <QDockWidget>

#include "UnitInventoryModel.h"

namespace Ui { class UnitDetailsDock; }

class UnitDetailsDock: public QDockWidget
{
	Q_OBJECT
public:
	UnitDetailsDock(const DwarfFortress &df, QWidget *parent = nullptr);
	~UnitDetailsDock() override;

public slots:
	void setUnit(const Unit *unit);

private:
	std::unique_ptr<Ui::UnitDetailsDock> _ui;
	const DwarfFortress &_df;
	UnitInventoryModel _inventory_model;
	QMetaObject::Connection _current_unit_destroyed;
};


#endif
