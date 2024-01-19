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

#ifndef UNIT_DESCRIPTORS_H
#define UNIT_DESCRIPTORS_H

#include <QObject>

#include "df_enums.h"

class UnitDescriptors: public QObject
{
	Q_OBJECT
	UnitDescriptors() = delete;
public:

	static QString attributeName(df::physical_attribute_type_t attr);
	static QString attributeName(df::mental_attribute_type_t attr);
	static QString attributeDescription(df::physical_attribute_type_t attr, int caste_rating);
	static QString attributeDescription(df::mental_attribute_type_t attr, int caste_rating);
};

#endif
