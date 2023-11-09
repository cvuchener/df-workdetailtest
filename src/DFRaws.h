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

#ifndef DF_RAWS_H
#define DF_RAWS_H

#include <dfs/CompoundReader.h>
#include <dfs/ItemReader.h>

#include "DFEnums.h"
#include "DFItemDefs.h"

#include <QString>

namespace df {

using namespace dfs;

template <typename Enum>
class FlagArray: public std::vector<bool>
{
public:
	bool isSet(Enum n) const {
		return n < size() && operator[](n);
	}
};

struct material_common
{
	std::array<std::string, 6> state_name;
	std::array<std::string, 6> state_adj;

	using reader_type = StructureReader<material_common, "material_common",
		Field<&material_common::state_name, "state_name">,
		Field<&material_common::state_adj, "state_adj">
	>;
};

struct material: material_common
{
	std::string prefix;

	using reader_type = StructureReader<material, "material",
		Base<material_common>,
		Field<&material::prefix, "prefix">
	>;
};

struct inorganic_raw
{
	std::string id;
	df::material material;

	using reader_type = StructureReader<inorganic_raw, "inorganic_raw",
		Field<&inorganic_raw::id, "id">,
		Field<&inorganic_raw::material, "material">
	>;
};

struct plant_raw
{
	std::string id;
	std::string name;
	std::string name_plural;
	std::string adj;
	std::vector<std::unique_ptr<df::material>> material;

	using reader_type = StructureReader<plant_raw, "plant_raw",
	      Field<&plant_raw::id, "id">,
	      Field<&plant_raw::name, "name">,
	      Field<&plant_raw::name_plural, "name_plural">,
	      Field<&plant_raw::adj, "adj">,
	      Field<&plant_raw::material, "material">
	>;
};

struct caste_raw
{
	std::string caste_id;
	std::array<std::string, 3> caste_name;
	std::array<std::string, 2> baby_name;
	std::array<std::string, 2> child_name;
	FlagArray<caste_raw_flags_t> flags;

	using reader_type = StructureReader<caste_raw, "caste_raw",
	      Field<&caste_raw::caste_id, "caste_id">,
	      Field<&caste_raw::caste_name, "caste_name">,
	      Field<&caste_raw::baby_name, "baby_name">,
	      Field<&caste_raw::child_name, "child_name">,
	      Field<&caste_raw::flags, "flags">
	>;
};

struct creature_raw
{
	std::string creature_id;
	std::array<std::string, 3> name;
	std::array<std::string, 2> general_baby_name;
	std::array<std::string, 2> general_child_name;
	std::vector<std::unique_ptr<caste_raw>> caste;
	std::vector<std::unique_ptr<df::material>> material;

	using reader_type = StructureReader<creature_raw, "creature_raw",
		Field<&creature_raw::creature_id, "creature_id">,
		Field<&creature_raw::name, "name">,
		Field<&creature_raw::general_baby_name, "general_baby_name">,
		Field<&creature_raw::general_child_name, "general_child_name">,
		Field<&creature_raw::caste, "caste">,
		Field<&creature_raw::material, "material">
	>;
};

struct language_name
{
	std::string first_name;
	std::string nickname;
	std::array<int32_t, 7> words;
	std::array<int16_t, 7> parts_of_speech;
	int32_t language;

	using reader_type = StructureReader<language_name, "language_name",
		Field<&language_name::first_name, "first_name">,
		Field<&language_name::nickname, "nickname">,
		Field<&language_name::words, "words">,
		Field<&language_name::parts_of_speech, "parts_of_speech">,
		Field<&language_name::language, "language">
	>;
};

struct language_word
{
	std::string word;
	std::array<std::string, 9> forms;

	using reader_type = StructureReader<language_word, "language_word",
		Field<&language_word::word, "word">,
		Field<&language_word::forms, "forms">
	>;
};

struct language_translation
{
	std::string name;
	std::vector<std::unique_ptr<std::string>> words;

	using reader_type = StructureReader<language_translation, "language_translation",
		Field<&language_translation::name, "name">,
		Field<&language_translation::words, "words">
	>;
};

struct world_raws
{
	std::vector<std::unique_ptr<inorganic_raw>> inorganics;

	struct plants_t
	{
		std::vector<std::shared_ptr<plant_raw>> all;

		using reader_type = StructureReader<plants_t, "world.raws.plants",
			Field<&plants_t::all, "all">
		>;
	} plants;

	struct creature_handler
	{
		std::vector<std::shared_ptr<creature_raw>> alphabetic;
		std::vector<std::shared_ptr<creature_raw>> all;

		using reader_type = StructureReader<creature_handler, "creature_handler",
			Field<&creature_handler::alphabetic, "alphabetic">,
			Field<&creature_handler::all, "all">
		>;
	} creatures;

	struct itemdefs_t
	{
		std::vector<std::shared_ptr<itemdef>> all;
		std::vector<std::shared_ptr<itemdef_weaponst>> weapons;
		std::vector<std::shared_ptr<itemdef_toyst>> toys;
		std::vector<std::shared_ptr<itemdef_toolst>> tools;
		std::array<std::vector<std::shared_ptr<itemdef_toolst>>, tool_uses::Count> tools_by_type;
		std::vector<std::shared_ptr<itemdef_instrumentst>> instruments;
		std::vector<std::shared_ptr<itemdef_armorst>> armor;
		std::vector<std::shared_ptr<itemdef_ammost>> ammo;
		std::vector<std::shared_ptr<itemdef_siegeammost>> siege_ammo;
		std::vector<std::shared_ptr<itemdef_glovesst>> gloves;
		std::vector<std::shared_ptr<itemdef_shoesst>> shoes;
		std::vector<std::shared_ptr<itemdef_shieldst>> shields;
		std::vector<std::shared_ptr<itemdef_helmst>> helms;
		std::vector<std::shared_ptr<itemdef_pantsst>> pants;
		std::vector<std::shared_ptr<itemdef_foodst>> food;

		using reader_type = StructureReader<itemdefs_t, "world.raws.itemdefs",
			Field<&itemdefs_t::all, "all">,
			Field<&itemdefs_t::weapons, "weapons">,
			Field<&itemdefs_t::toys, "toys">,
			Field<&itemdefs_t::tools, "tools">,
			Field<&itemdefs_t::tools_by_type, "tools_by_type">,
			Field<&itemdefs_t::instruments, "instruments">,
			Field<&itemdefs_t::armor, "armor">,
			Field<&itemdefs_t::ammo, "ammo">,
			Field<&itemdefs_t::siege_ammo, "siege_ammo">,
			Field<&itemdefs_t::gloves, "gloves">,
			Field<&itemdefs_t::shoes, "shoes">,
			Field<&itemdefs_t::shields, "shields">,
			Field<&itemdefs_t::helms, "helms">,
			Field<&itemdefs_t::pants, "pants">,
			Field<&itemdefs_t::food, "food">
		>;
	} itemdefs;

	struct language_t
	{
		std::vector<std::unique_ptr<language_word>> words;
		std::vector<std::unique_ptr<language_translation>> translations;

		std::string_view english_word(const df::language_name &name, df::language_name_component_t comp) const;
		std::string_view local_word(const df::language_name &name, df::language_name_component_t comp) const;
		QString translate_name(const df::language_name &name, bool english = false) const;


		using reader_type = StructureReader<world_raws::language_t, "world_raws.language",
			Field<&world_raws::language_t::words, "words">,
			Field<&world_raws::language_t::translations, "translations">
		>;
	} language;

	std::array<std::unique_ptr<material>, 659> builtin_mats;

	using reader_type = StructureReader<world_raws, "world_raws",
		Field<&world_raws::inorganics, "inorganics">,
		Field<&world_raws::plants, "plants">,
		Field<&world_raws::creatures, "creatures">,
		Field<&world_raws::itemdefs, "itemdefs">,
		Field<&world_raws::language, "language">,
		Field<&world_raws::builtin_mats, "mat_table.builtin">
	>;
};

} // namespace df

#endif
