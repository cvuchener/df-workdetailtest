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

#include "GroupBy.h"

GroupBy::GroupBy()
{
}

GroupBy::~GroupBy()
{
}

QVariant GroupBy::sortValue(quint64 group_id) const
{
	return groupName(group_id);
}

#include "GroupByCreature.h"
#include "GroupByMigration.h"
#include "GroupByWorkDetailAssigned.h"

static std::unique_ptr<GroupBy> null_factory(const DwarfFortress &)
{
	return nullptr;
}

template <std::derived_from<GroupBy> T>
static std::unique_ptr<GroupBy> factory(const DwarfFortress &df)
{
	return std::make_unique<T>(df);
}

const std::vector<std::pair<const char *, GroupBy::GroupByFactory>> GroupBy::AllMethods = {
	{ QT_TRANSLATE_NOOP("GroupBy", "No group"), null_factory },
	{ QT_TRANSLATE_NOOP("GroupBy", "Creature"), factory<GroupByCreature> },
	{ QT_TRANSLATE_NOOP("GroupBy", "Migration wave"), factory<GroupByMigration> },
	{ QT_TRANSLATE_NOOP("GroupBy", "Work detail assigned"), factory<GroupByWorkDetailAssigned> },
};
