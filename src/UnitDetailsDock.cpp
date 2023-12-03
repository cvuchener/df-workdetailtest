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

#include "UnitDetailsDock.h"

#include "ui_UnitDetailsDock.h"

#include "DwarfFortress.h"
#include "Unit.h"

static const char *NamePlaceholder = QT_TRANSLATE_NOOP(UnitDetailsDock, "Select a unit");

UnitDetailsDock::UnitDetailsDock(const DwarfFortress &df, QWidget *parent):
	QDockWidget(tr("Unit details"), parent),
	_ui(std::make_unique<Ui::UnitDetailsDock>()),
	_df(df),
	_inventory_model(df)
{
	_ui->setupUi(this);
	_ui->unit_name->setText(tr(NamePlaceholder));
	_ui->unit_profession->setText({});
	_ui->unit_age->setText({});
	_ui->inventory_view->setModel(&_inventory_model);
}

UnitDetailsDock::~UnitDetailsDock()
{
}

void UnitDetailsDock::setUnit(const Unit *unit)
{
	if (_current_unit_destroyed)
		disconnect(_current_unit_destroyed);
	if (unit) {
		_current_unit_destroyed = connect(unit, &QObject::destroyed,
				this, [this](){setUnit(nullptr);});
		_ui->unit_name->setText(unit->displayName());
		_ui->unit_profession->setText(QString::fromLocal8Bit(caption((*unit)->profession)));
		auto age = df::date<df::year, df::month>(unit->age());
		if (get<df::year>(age).count() > 0) {
			auto years = get<df::year>(age).count();
			_ui->unit_age->setText(tr("%1 year(s) old", nullptr, years).arg(years));
		}
		else {
			auto months = get<df::month>(age).count();
			_ui->unit_age->setText(tr("%1 month(s) old", nullptr, months).arg(months));
		}

	}
	else {
		_ui->unit_name->setText(NamePlaceholder);
		_ui->unit_profession->setText({});
		_ui->unit_age->setText({});
	}
	_inventory_model.setUnit(unit);
}

