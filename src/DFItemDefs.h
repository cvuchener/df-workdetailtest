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

#ifndef DF_ITEMDEFS_H
#define DF_ITEMDEFS_H

#include <dfs/CompoundReader.h>
#include <dfs/PolymorphicReader.h>
#include <dfs/ItemReader.h>

#include "DFEnums.h"

namespace df {

using namespace dfs;

struct itemdef
{
	virtual ~itemdef() = default;

	std::string id;
	int subtype;

	using reader_type = StructureReader<itemdef, "itemdef",
		Field<&itemdef::id, "id">,
		Field<&itemdef::subtype, "subtype">
	>;
};

#define ITEMDEF_END_FIELDS(...)
#define ITEMDEF_END_READER(...)

#define ITEMDEF_NAME_FIELDS(next, ...) \
	std::string name; \
	ITEMDEF_##next##_FIELDS(__VA_ARGS__)
#define ITEMDEF_NAME_READER(type, next, ...) \
	, Field<&type::name, "name"> \
	ITEMDEF_##next##_READER(type, __VA_ARGS__)

#define ITEMDEF_PLURAL_FIELDS(next, ...) \
	std::string name_plural; \
	ITEMDEF_##next##_FIELDS(__VA_ARGS__)
#define ITEMDEF_PLURAL_READER(type, next, ...) \
	, Field<&type::name_plural, "name_plural"> \
	ITEMDEF_##next##_READER(type, __VA_ARGS__)

#define ITEMDEF_PREPLURAL_FIELDS(next, ...) \
	std::string name_preplural; \
	ITEMDEF_##next##_FIELDS(__VA_ARGS__)
#define ITEMDEF_PREPLURAL_READER(type, next, ...) \
	, Field<&type::name_preplural, "name_preplural"> \
	ITEMDEF_##next##_READER(type, __VA_ARGS__)

#define ITEMDEF_ADJ_FIELDS(next, ...) \
	std::string adjective; \
	ITEMDEF_##next##_FIELDS(__VA_ARGS__)
#define ITEMDEF_ADJ_READER(type, next, ...) \
	, Field<&type::adjective, "adjective"> \
	ITEMDEF_##next##_READER(type, __VA_ARGS__)

#define ITEMDEF_MAT_PH_FIELDS(next, ...) \
	std::string material_placeholder; \
	ITEMDEF_##next##_FIELDS(__VA_ARGS__)
#define ITEMDEF_MAT_PH_READER(type, next, ...) \
	, Field<&type::material_placeholder, "material_placeholder"> \
	ITEMDEF_##next##_READER(type, __VA_ARGS__)

#define MAKE_ITEMDEF_FIELDS(first, ...) \
	ITEMDEF_##first##_FIELDS(__VA_ARGS__)
#define MAKE_ITEMDEF_READER(type, first, ...) \
	ITEMDEF_##first##_READER(type, __VA_ARGS__)

#define MAKE_ITEMDEF_TYPE(type, ...) \
struct type: itemdef \
{ \
	MAKE_ITEMDEF_FIELDS(__VA_ARGS__) \
	using reader_type = StructureReader<type, #type, \
		Base<itemdef> \
		MAKE_ITEMDEF_READER(type, __VA_ARGS__) \
	>; \
};

#define FOR_ALL_ITEMDEFS(DO) \
	DO(itemdef_ammost, NAME, PLURAL, ADJ, END) \
	DO(itemdef_armorst, NAME, PLURAL, PREPLURAL, MAT_PH, ADJ, END) \
	DO(itemdef_foodst, NAME, END) \
	DO(itemdef_glovesst, NAME, PLURAL, ADJ, END) \
	DO(itemdef_helmst, NAME, PLURAL, ADJ, END) \
	DO(itemdef_instrumentst, NAME, PLURAL, END) \
	DO(itemdef_pantsst, NAME, PLURAL, PREPLURAL, MAT_PH, ADJ, END) \
	DO(itemdef_shieldst, NAME, PLURAL, ADJ, END) \
	DO(itemdef_shoesst, NAME, PLURAL, ADJ, END) \
	DO(itemdef_siegeammost, NAME, PLURAL, END) \
	DO(itemdef_toyst, NAME, PLURAL, END) \
	DO(itemdef_trapcompst, NAME, PLURAL, ADJ, END) \
	DO(itemdef_weaponst, NAME, PLURAL, ADJ, END) \
	DO(itemdef_toolst, NAME, PLURAL, ADJ, END)

FOR_ALL_ITEMDEFS(MAKE_ITEMDEF_TYPE)

template <typename T>
concept ItemDefHasName = std::derived_from<T, itemdef> &&
	std::same_as<decltype(T::name), std::string>;

template <typename T>
concept ItemDefHasPlural = std::derived_from<T, itemdef> &&
	std::same_as<decltype(T::name_plural), std::string>;

template <typename T>
concept ItemDefHasPrePlural = std::derived_from<T, itemdef> &&
	std::same_as<decltype(T::name_preplural), std::string>;

template <typename T>
concept ItemDefHasAdjective = std::derived_from<T, itemdef> &&
	std::same_as<decltype(T::adjective), std::string>;

template <typename T>
concept ItemDefHasMatPlaceholder = std::derived_from<T, itemdef> &&
	std::same_as<decltype(T::material_placeholder), std::string>;

} // namespace df

template <>
struct dfs::polymorphic_reader_type<df::itemdef> {
	using type = PolymorphicReader<df::itemdef
#define ADD_TYPE(type, ...) , df::type
		FOR_ALL_ITEMDEFS(ADD_TYPE)
#undef ADD_TYPE
	>;
};

#endif
