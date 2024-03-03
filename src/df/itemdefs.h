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

#include "df_enums.h"
#include "df/FlagArray.h"
#include "df/components.h"

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

struct itemdef_component_name {
	std::string name;
	using fields = std::tuple<Field<&itemdef_component_name::name, "name">>;
};
struct itemdef_component_name_plural {
	std::string name_plural;
	using fields = std::tuple<Field<&itemdef_component_name_plural::name_plural, "name_plural">>;
};
struct itemdef_component_name_preplural {
	std::string name_preplural;
	using fields = std::tuple<Field<&itemdef_component_name_preplural::name_preplural, "name_preplural">>;
};
struct itemdef_component_adjective {
	std::string adjective;
	using fields = std::tuple<Field<&itemdef_component_adjective::adjective, "adjective">>;
};
struct itemdef_component_material_placeholder {
	std::string material_placeholder;
	using fields = std::tuple<Field<&itemdef_component_material_placeholder::material_placeholder, "material_placeholder">>;
};
struct itemdef_component_armor_level {
	int armorlevel;
	using fields = std::tuple<Field<&itemdef_component_armor_level::armorlevel, "armorlevel">>;
};
struct itemdef_component_armor_props {
	FlagArray<armor_general_flags_t> armor_flags;
	using fields = std::tuple<Field<&itemdef_component_armor_props::armor_flags, "props.flags">>;
};
template <typename Flags>
struct itemdef_component_flags {
	FlagArray<Flags> flags;
	using fields = std::tuple<Field<&itemdef_component_flags::flags, "flags">>;
};

#define FOR_ALL_ITEMDEFS(DO) \
	DO(itemdef_ammost, NAME, PLURAL, ADJ, FLAGS(ammo)) \
	DO(itemdef_armorst, NAME, PLURAL, PREPLURAL, MAT_PH, ADJ, ARMOR_LEVEL, ARMOR_PROPS, FLAGS(armor)) \
	DO(itemdef_foodst, NAME) \
	DO(itemdef_glovesst, NAME, PLURAL, ADJ, ARMOR_LEVEL, ARMOR_PROPS, FLAGS(gloves)) \
	DO(itemdef_helmst, NAME, PLURAL, ADJ, ARMOR_LEVEL, ARMOR_PROPS, FLAGS(helm)) \
	DO(itemdef_instrumentst, NAME, PLURAL, FLAGS(instrument)) \
	DO(itemdef_pantsst, NAME, PLURAL, PREPLURAL, MAT_PH, ADJ, ARMOR_LEVEL, ARMOR_PROPS, FLAGS(pants)) \
	DO(itemdef_shieldst, NAME, PLURAL, ADJ, ARMOR_LEVEL) \
	DO(itemdef_shoesst, NAME, PLURAL, ADJ, ARMOR_LEVEL, ARMOR_PROPS, FLAGS(shoes)) \
	DO(itemdef_siegeammost, NAME, PLURAL) \
	DO(itemdef_toyst, NAME, PLURAL, FLAGS(toy)) \
	DO(itemdef_trapcompst, NAME, PLURAL, ADJ, FLAGS(trapcomp)) \
	DO(itemdef_weaponst, NAME, PLURAL, ADJ, FLAGS(weapon)) \
	DO(itemdef_toolst, NAME, PLURAL, ADJ, FLAGS(tool))

#define NAME itemdef_component_name
#define PLURAL itemdef_component_name_plural
#define PREPLURAL itemdef_component_name_preplural
#define ADJ itemdef_component_adjective
#define MAT_PH itemdef_component_material_placeholder
#define ARMOR_LEVEL itemdef_component_armor_level
#define ARMOR_PROPS itemdef_component_armor_props
#define FLAGS(n) itemdef_component_flags<n##_flags_t>

#define MAKE_ITEMDEF_TYPE(type, ...) \
struct type: FromComponents<itemdef, #type, __VA_ARGS__> { };
FOR_ALL_ITEMDEFS(MAKE_ITEMDEF_TYPE)
#undef MAKE_ITEMDEF_TYPE

#undef NAME
#undef PLURAL
#undef PREPLURAL
#undef ADJ
#undef MAT_PH
#undef ARMOR_LEVEL
#undef ARMOR_PROPS
#undef FLAGS

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

template <typename T>
concept ItemDefHasArmorFlags = std::derived_from<T, itemdef> &&
	std::same_as<decltype(T::armor_flags), FlagArray<armor_general_flags_t>>;

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
