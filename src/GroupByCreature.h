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

#ifndef GROUP_BY_CREATURE_H
#define GROUP_BY_CREATURE_H

#include "GroupBy.h"

class DwarfFortress;

class GroupByCreature: public GroupBy
{
public:
	GroupByCreature(const DwarfFortress &df);
	~GroupByCreature() override;

	quint64 unitGroup(const Unit &unit) const override;
	QString groupName(quint64 group_id) const override;

private:
	const DwarfFortress &_df;
};

#endif
