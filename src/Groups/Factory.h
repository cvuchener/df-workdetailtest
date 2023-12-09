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

#ifndef GROUPS_FACTORY_H
#define GROUPS_FACTORY_H

#include <functional>
#include <memory>

class GroupBy;
class DwarfFortress;

namespace Groups {

using Factory = std::function<std::unique_ptr<GroupBy>(const DwarfFortress &)>;

extern const std::vector<std::pair<const char *, Factory>> All;

}

#endif
