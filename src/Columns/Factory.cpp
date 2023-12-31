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

#include "Factory.h"

#include <QJsonObject>

#include "GridViewModel.h"

#include "WorkDetailColumn.h"
#include "SpecialistColumn.h"

Columns::Factory Columns::makeFactory(const QJsonObject &col)
{
	auto type = col.value("type").toString();
	if (type == "WorkDetail")
		return WorkDetailColumn::makeFactory(col);
	else if (type == "Specialist")
		return SpecialistColumn::makeFactory(col);
	else {
		qCCritical(GridViewLog) << "Unsupported column type:" << type;
		return {};
	}
}
