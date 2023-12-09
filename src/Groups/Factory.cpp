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

const std::vector<std::pair<const char *, Groups::Factory>> Groups::All = {
	{ QT_TRANSLATE_NOOP("Groups", "No group"), null_factory },
	{ QT_TRANSLATE_NOOP("Groups", "Creature"), factory<GroupByCreature> },
	{ QT_TRANSLATE_NOOP("Groups", "Migration wave"), factory<GroupByMigration> },
	{ QT_TRANSLATE_NOOP("Groups", "Work detail assigned"), factory<GroupByWorkDetailAssigned> },
};
