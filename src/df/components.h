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

#ifndef DF_COMPONENTS_H
#define DF_COMPONENTS_H

#include <tuple>
#include <dfs/CompoundReader.h>

namespace df {

namespace details {

template <typename... Tuples>
using concat_tuple_types_t = decltype(std::tuple_cat(std::declval<Tuples>()...));

template <template <typename...> typename Tmpl, typename Tuple>
struct apply_tuple_types;
template <template <typename...> typename Tmpl, typename... Ts>
struct apply_tuple_types<Tmpl, std::tuple<Ts...>>: std::type_identity<Tmpl<Ts...>> {};
template <template <typename...> typename Tmpl, typename Tuple>
using apply_tuple_types_t = apply_tuple_types<Tmpl, Tuple>::type;

} // namespace details

template <typename Base, dfs::static_string Name, typename... Ts>
struct FromComponents: Base, Ts...
{
	template <typename... Us>
	using reader_type_fn = dfs::StructureReader<FromComponents, Name, dfs::Base<Base>, Us...>;
	using reader_type = details::apply_tuple_types_t<
		reader_type_fn,
		details::concat_tuple_types_t<typename Ts::fields...>
	>;
};

} // namespace df

#endif
