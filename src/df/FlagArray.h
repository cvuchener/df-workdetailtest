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

#ifndef DF_FLAG_ARRAY_H
#define DF_FLAG_ARRAY_H

#include <vector>

namespace df {

template <typename Enum>
class FlagArray: public std::vector<bool>
{
public:
	bool isSet(Enum n) const {
		return n < size() && operator[](n);
	}
};

} // namespace df

#endif
