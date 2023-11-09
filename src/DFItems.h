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

#ifndef DF_ITEMS_H
#define DF_ITEMS_H

#include <dfs/CompoundReader.h>
#include <dfs/PolymorphicReader.h>
#include <dfs/ItemReader.h>

#include "DFEnums.h"
#include "DFRaws.h"

namespace df {

using namespace dfs;

struct item
{
	virtual ~item() = default;

	int id;

	virtual item_type_t getType() const noexcept { return item_type::NONE; }

	using reader_type = StructureReader<item, "item",
		Field<&item::id, "id">
	>;
};

struct item_actual: item
{
	int stack_size;

	using reader_type = StructureReader<item_actual, "item_actual",
		Base<item>,
		Field<&item_actual::stack_size, "stack_size">
	>;
};

struct item_crafted: item_actual
{
	int mat_type;
	int mat_index;
	item_quality_t quality;

	using reader_type = StructureReader<item_crafted, "item_crafted",
		Base<item_actual>,
		Field<&item_crafted::mat_type, "mat_type">,
		Field<&item_crafted::mat_index, "mat_index">,
		Field<&item_crafted::quality, "quality">
	>;
};

struct item_constructed: item_crafted
{
	using reader_type = StructureReader<item_constructed, "item_constructed",
		Base<item_crafted>
	>;
};

struct item_body_component: item_actual
{
	int race;
	int caste;
	int hist_figure_id;
	int unit_id;

	using reader_type = StructureReader<item_body_component, "item_body_component",
		Base<item_actual>,
		Field<&item_body_component::race, "race">,
		Field<&item_body_component::caste, "caste">,
		Field<&item_body_component::hist_figure_id, "hist_figure_id">,
		Field<&item_body_component::unit_id, "unit_id">
	>;
};

struct item_critter: item_actual
{
	int race;
	int caste;
	language_name name;

	using reader_type = StructureReader<item_critter, "item_critter",
		Base<item_actual>,
		Field<&item_critter::race, "race">,
		Field<&item_critter::caste, "caste">,
		Field<&item_critter::name, "name">
	>;
};

struct item_liquipowder: item_actual
{
	item_matstate mat_state;

	using reader_type = StructureReader<item_liquipowder, "item_liquipowder",
		Base<item_actual>,
		Field<&item_liquipowder::mat_state, "mat_state">
	>;
};

#define ITEM_END_FIELDS(...)
#define ITEM_END_READER(...)

#define ITEM_MAT_FIELDS(next, ...) \
	int mat_type; \
	int mat_index; \
	ITEM_##next##_FIELDS(__VA_ARGS__)
#define ITEM_MAT_READER(type, next, ...) \
	, Field<&type::mat_type, "mat_type"> \
	, Field<&type::mat_index, "mat_index"> \
	ITEM_##next##_READER(type, __VA_ARGS__)

#define ITEM_SUBTYPE_ID_FIELDS(next, ...) \
	int subtype; \
	ITEM_##next##_FIELDS(__VA_ARGS__)
#define ITEM_SUBTYPE_ID_READER(type, next, ...) \
	, Field<&type::subtype, "subtype"> \
	ITEM_##next##_READER(type, __VA_ARGS__)

#define ITEM_SUBTYPE_PTR_FIELDS(itemdef, next, ...) \
	std::shared_ptr<itemdef> subtype; \
	ITEM_##next##_FIELDS(__VA_ARGS__)
#define ITEM_SUBTYPE_PTR_READER(type, itemdef, next, ...) \
	, Field<&type::subtype, "subtype"> \
	ITEM_##next##_READER(type, __VA_ARGS__)

#define ITEM_CREATURE_FIELDS(next, ...) \
	int race; \
	int caste; \
	ITEM_##next##_FIELDS(__VA_ARGS__)
#define ITEM_CREATURE_READER(type, next, ...) \
	, Field<&type::race, "race"> \
	, Field<&type::caste, "caste"> \
	ITEM_##next##_READER(type, __VA_ARGS__)

#define ITEM_SHARP_FIELDS(next, ...) \
	int sharpness; \
	ITEM_##next##_FIELDS(__VA_ARGS__)
#define ITEM_SHARP_READER(type, next, ...) \
	, Field<&type::sharpness, "sharpness"> \
	ITEM_##next##_READER(type, __VA_ARGS__)

#define MAKE_ITEM_FIELDS(first, ...) \
	ITEM_##first##_FIELDS(__VA_ARGS__)
#define MAKE_ITEM_READER(type, first, ...) \
	ITEM_##first##_READER(type, __VA_ARGS__)

#define MAKE_ITEM_TYPE(type, item_type_value, base, ...) \
struct type: base \
{ \
	static inline constexpr auto item_type = item_type::item_type_value; \
	MAKE_ITEM_FIELDS(__VA_ARGS__) \
	item_type_t getType() const noexcept override { return item_type::item_type_value; } \
	using reader_type = StructureReader<type, #type, \
		Base<base> \
		MAKE_ITEM_READER(type, __VA_ARGS__) \
	>; \
};

MAKE_ITEM_TYPE(item_liquid, NONE, item_liquipowder, MAT, END)
MAKE_ITEM_TYPE(item_powder, NONE, item_liquipowder, MAT, END)

#define FOR_ALL_CONCRETE_ITEMS(DO) \
	DO(item_barst, BAR, item_actual, SUBTYPE_ID, MAT, END) \
	DO(item_smallgemst, SMALLGEM, item_actual, MAT, END) \
	DO(item_blocksst, BLOCKS, item_actual, MAT, END) \
	DO(item_roughst, ROUGH, item_actual, MAT, END) \
	DO(item_boulderst, BOULDER, item_actual, MAT, END) \
	DO(item_woodst, WOOD, item_actual, MAT, END) \
	DO(item_doorst, DOOR, item_constructed, END) \
	DO(item_floodgatest, FLOODGATE, item_constructed, END) \
	DO(item_bedst, BED, item_constructed, END) \
	DO(item_chairst, CHAIR, item_constructed, END) \
	DO(item_chainst, CHAIN, item_constructed, END) \
	DO(item_flaskst, FLASK, item_constructed, END) \
	DO(item_gobletst, GOBLET, item_constructed, END) \
	DO(item_instrumentst, INSTRUMENT, item_constructed, SUBTYPE_PTR, itemdef_instrumentst, END) \
	DO(item_toyst, TOY, item_constructed, SUBTYPE_PTR, itemdef_toyst, END) \
	DO(item_windowst, WINDOW, item_constructed, END) \
	DO(item_cagest, CAGE, item_constructed, END) \
	DO(item_barrelst, BARREL, item_constructed, END) \
	DO(item_bucketst, BUCKET, item_constructed, END) \
	DO(item_animaltrapst, ANIMALTRAP, item_constructed, END) \
	DO(item_tablest, TABLE, item_constructed, END) \
	DO(item_coffinst, COFFIN, item_constructed, END) \
	DO(item_statuest, STATUE, item_constructed, END) \
	DO(item_corpsest, CORPSE, item_body_component, END) \
	DO(item_weaponst, WEAPON, item_constructed, SUBTYPE_PTR, itemdef_weaponst, SHARP, END) \
	DO(item_armorst, ARMOR, item_constructed, SUBTYPE_PTR, itemdef_armorst, END) \
	DO(item_shoesst, SHOES, item_constructed, SUBTYPE_PTR, itemdef_shoesst, END) \
	DO(item_shieldst, SHIELD, item_constructed, SUBTYPE_PTR, itemdef_shieldst, END) \
	DO(item_helmst, HELM, item_constructed, SUBTYPE_PTR, itemdef_helmst, END) \
	DO(item_glovesst, GLOVES, item_constructed, SUBTYPE_PTR, itemdef_glovesst, END) \
	DO(item_bagst, BAG, item_constructed, END) \
	DO(item_boxst, BOX, item_constructed, END) \
	DO(item_binst, BIN, item_constructed, END) \
	DO(item_armorstandst, ARMORSTAND, item_constructed, END) \
	DO(item_weaponrackst, WEAPONRACK, item_constructed, END) \
	DO(item_cabinetst, CABINET, item_constructed, END) \
	DO(item_figurinest, FIGURINE, item_constructed, END) \
	DO(item_amuletst, AMULET, item_constructed, END) \
	DO(item_scepterst, SCEPTER, item_constructed, END) \
	DO(item_ammost, AMMO, item_constructed, SUBTYPE_PTR, itemdef_ammost, SHARP, END) \
	DO(item_crownst, CROWN, item_constructed, END) \
	DO(item_ringst, RING, item_constructed, END) \
	DO(item_earringst, EARRING, item_constructed, END) \
	DO(item_braceletst, BRACELET, item_constructed, END) \
	DO(item_gemst, GEM, item_constructed, END) \
	DO(item_anvilst, ANVIL, item_constructed, END) \
	DO(item_corpsepiecest, CORPSEPIECE, item_body_component, END) \
	DO(item_remainsst, REMAINS, item_actual, CREATURE, END) \
	DO(item_meatst, MEAT, item_actual, MAT, END) \
	DO(item_fishst, FISH, item_actual, CREATURE, END) \
	DO(item_fish_rawst, FISH_RAW, item_actual, CREATURE, END) \
	DO(item_verminst, VERMIN, item_critter, END) \
	DO(item_petst, PET, item_critter, END) \
	DO(item_seedsst, SEEDS, item_actual, MAT, END) \
	DO(item_plantst, PLANT, item_actual, MAT, END) \
	DO(item_skin_tannedst, SKIN_TANNED, item_actual, MAT, END) \
	DO(item_plant_growthst, PLANT_GROWTH, item_actual, MAT, END) \
	DO(item_threadst, THREAD, item_actual, MAT, END) \
	DO(item_clothst, CLOTH, item_constructed, END) \
	DO(item_totemst, TOTEM, item_constructed, CREATURE, END) \
	DO(item_pantsst, PANTS, item_constructed, SUBTYPE_PTR, itemdef_pantsst, END) \
	DO(item_backpackst, BACKPACK, item_constructed, END) \
	DO(item_quiverst, QUIVER, item_constructed, END) \
	DO(item_catapultpartsst, CATAPULTPARTS, item_constructed, END) \
	DO(item_ballistapartsst, BALLISTAPARTS, item_constructed, END) \
	DO(item_siegeammost, SIEGEAMMO, item_constructed, SUBTYPE_PTR, itemdef_siegeammost, SHARP, END) \
	DO(item_ballistaarrowheadst, BALLISTAARROWHEAD, item_actual, MAT, SHARP, END) \
	DO(item_trappartsst, TRAPPARTS, item_constructed, END) \
	DO(item_trapcompst, TRAPCOMP, item_constructed, SUBTYPE_PTR, itemdef_trapcompst, SHARP, END) \
	DO(item_drinkst, DRINK, item_liquid, END) \
	DO(item_powder_miscst, POWDER_MISC, item_powder, END) \
	DO(item_cheesest, CHEESE, item_actual, MAT, END) \
	DO(item_foodst, FOOD, item_crafted, SUBTYPE_PTR, itemdef_foodst, END) \
	DO(item_liquid_miscst, LIQUID_MISC, item_liquid, END) \
	DO(item_coinst, COIN, item_constructed, END) \
	DO(item_globst, GLOB, item_actual, MAT, END) \
	DO(item_rockst, ROCK, item_actual, MAT, SHARP, END) \
	DO(item_pipe_sectionst, PIPE_SECTION, item_constructed, END) \
	DO(item_hatch_coverst, HATCH_COVER, item_constructed, END) \
	DO(item_gratest, GRATE, item_constructed, END) \
	DO(item_quernst, QUERN, item_constructed, END) \
	DO(item_millstonest, MILLSTONE, item_constructed, END) \
	DO(item_splintst, SPLINT, item_constructed, END) \
	DO(item_crutchst, CRUTCH, item_constructed, END) \
	DO(item_traction_benchst, TRACTION_BENCH, item_constructed, END) \
	DO(item_orthopedic_castst, ORTHOPEDIC_CAST, item_constructed, END) \
	DO(item_toolst, TOOL, item_constructed, SUBTYPE_PTR, itemdef_toolst, SHARP, END) \
	DO(item_slabst, SLAB, item_constructed, END) \
	DO(item_eggst, EGG, item_actual, CREATURE, END) \
	DO(item_bookst, BOOK, item_constructed, END) \
	DO(item_sheetst, SHEET, item_constructed, END) \
	DO(item_branchst, BRANCH, item_actual, MAT, END)

FOR_ALL_CONCRETE_ITEMS(MAKE_ITEM_TYPE)

template <typename Visitor>
auto visit_item(Visitor &&visitor, const item &base_item) {
	switch (base_item.getType()) {
#define CALL_VISITOR(type, value, ...) case item_type::value: return std::forward<Visitor>(visitor)(static_cast<const type &>(base_item));
	FOR_ALL_CONCRETE_ITEMS(CALL_VISITOR)
#undef CALL_VISITOR
	case item_type::NONE:
	default:
		return std::forward<Visitor>(visitor)(base_item);
	}
}

template <typename T>
concept ItemHasStackSize = std::derived_from<T, item> && requires (T v) {
	{ v.stack_size } -> std::convertible_to<int>;
};

template <typename T>
concept ItemHasMaterial = std::derived_from<T, item> && requires (T v) {
	{ v.mat_type } -> std::convertible_to<int>;
	{ v.mat_index } -> std::convertible_to<int>;
};

template <typename T>
concept ItemHasCreature = std::derived_from<T, item> && requires (T v) {
	{ v.race } -> std::convertible_to<int>;
	{ v.caste } -> std::convertible_to<int>;
};

template <typename T>
concept ItemHasItemDef = std::derived_from<T, item> &&
	std::derived_from<typename decltype(T::subtype)::element_type, itemdef>;

template <typename T>
struct item_itemdef;

template <ItemHasItemDef T>
struct item_itemdef<T>
{
	using type = decltype(T::subtype)::element_type;
};

template <typename T>
using item_itemdef_t = item_itemdef<T>::type;

} // namespace df

template <>
struct dfs::polymorphic_reader_type<df::item> {
	using type = PolymorphicReader<df::item
#define ADD_TYPE(type, ...) , df::type
		ADD_TYPE(item_actual)
		//ADD_TYPE(item_crafted)
		ADD_TYPE(item_constructed)
		ADD_TYPE(item_body_component)
		ADD_TYPE(item_critter)
		//ADD_TYPE(item_liquipowder)
		//ADD_TYPE(item_liquid)
		//ADD_TYPE(item_powder)
		FOR_ALL_CONCRETE_ITEMS(ADD_TYPE)
#undef ADD_TYPE
	>;
};
#endif
