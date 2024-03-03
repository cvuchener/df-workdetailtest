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

#ifndef ITEM_H
#define ITEM_H

#include "DwarfFortressData.h"
#include "df_enums.h"

namespace df { struct item; }

namespace Item
{
	QString toString(const DwarfFortressData &df, const df::item &);
	QString toString(const DwarfFortressData &df, df::item_type_t type, int subtype = -1, int mattype = -1, int matindex = -1, bool plural = false);
};

#endif
