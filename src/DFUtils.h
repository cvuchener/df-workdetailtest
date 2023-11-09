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

#ifndef DF_UTILS_H
#define DF_UTILS_H

#include <ranges>
#include <algorithm>
#include <memory>

namespace df {

template <std::ranges::random_access_range Vector, typename Id> requires requires (std::ranges::range_value_t<Vector>::element_type item) {
	{ item.id } -> std::totally_ordered_with<Id>;
}
auto find(const Vector &vec, Id id)
	-> std::ranges::range_value_t<Vector>::pointer
{
	auto it = std::ranges::lower_bound(vec, id, std::less{}, [](const auto &ptr) { return ptr->id; });
	if (it != end(vec) && (*it)->id == id)
		return it->get();
	else
		return nullptr;
}

}

#endif
