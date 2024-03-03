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

#include "df_enums.h"
#include "df/raws.h"
#include "df/components.h"

namespace df {

using namespace dfs;

struct itemimprovement
{
	using fallback = fallback_base;

	virtual ~itemimprovement() = default;

	item_quality_t quality;

	virtual bool isDecoration() const noexcept { return true; }

	using reader_type = StructureReader<itemimprovement, "itemimprovement",
		Field<&itemimprovement::quality, "quality">
	>;
};

struct itemimprovement_thread: itemimprovement
{
	bool isDecoration() const noexcept override { return false; }

	using reader_type = StructureReader<itemimprovement_thread, "itemimprovement_threadst",
		Base<itemimprovement>
	>;
};

struct itemimprovement_cloth: itemimprovement
{
	bool isDecoration() const noexcept override { return false; }

	using reader_type = StructureReader<itemimprovement_cloth, "itemimprovement_clothst",
		Base<itemimprovement>
	>;
};

struct itemimprovement_instrument_piece: itemimprovement
{
	bool isDecoration() const noexcept override { return quality > 0; }

	using reader_type = StructureReader<itemimprovement_instrument_piece, "itemimprovement_instrument_piecest",
		Base<itemimprovement>
	>;
};

struct item
{
	virtual ~item() = default;

	item_flags flags;
	item_flags2 flags2;
	int id;

	virtual item_type_t getType() const noexcept { return item_type::NONE; }

	using reader_type = StructureReader<item, "item",
		Field<&item::flags, "flags">,
		Field<&item::flags2, "flags2">,
		Field<&item::id, "id">
	>;
};

struct item_actual: item
{
	int stack_size;
	int wear;

	using reader_type = StructureReader<item_actual, "item_actual",
		Base<item>,
		Field<&item_actual::stack_size, "stack_size">,
		Field<&item_actual::wear, "wear">
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
	std::vector<std::unique_ptr<itemimprovement>> improvements;

	using reader_type = StructureReader<item_constructed, "item_constructed",
		Base<item_crafted>,
		Field<&item_constructed::improvements, "improvements">
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

struct item_liquid: item_liquipowder
{
	int mat_type;
	int mat_index;
	using reader_type = StructureReader<item_liquid, "item_liquid",
		Base<item_liquipowder>,
		Field<&item_liquid::mat_type, "mat_type">,
		Field<&item_liquid::mat_index, "mat_index">
	>;
};

struct item_powder: item_liquipowder
{
	int mat_type;
	int mat_index;
	using reader_type = StructureReader<item_powder, "item_powder",
		Base<item_liquipowder>,
		Field<&item_powder::mat_type, "mat_type">,
		Field<&item_powder::mat_index, "mat_index">
	>;
};

// Concrete items components

struct item_component_material {
	int mat_type;
	int mat_index;
	using fields = std::tuple<
		Field<&item_component_material::mat_type, "mat_type">,
		Field<&item_component_material::mat_index, "mat_index">
	>;
};

template <typename ItemDef>
struct item_component_subtype_ptr {
	std::shared_ptr<ItemDef> subtype;
	using fields = std::tuple<Field<&item_component_subtype_ptr::subtype, "subtype">>;
};

struct item_component_subtype_id {
	int subtype;
	using fields = std::tuple<Field<&item_component_subtype_id::subtype, "subtype">>;
};

struct item_component_creature {
	int race;
	int caste;
	using fields = std::tuple<
		Field<&item_component_creature::race, "race">,
		Field<&item_component_creature::caste, "caste">
	>;
};

struct item_component_sharp {
	int sharpness;
	using fields = std::tuple<Field<&item_component_sharp::sharpness, "sharpness">>;
};

#define FOR_ALL_CONCRETE_ITEMS(DO) \
	DO(item_barst, BAR, item_actual, SUBTYPE_ID, MAT) \
	DO(item_smallgemst, SMALLGEM, item_actual, MAT) \
	DO(item_blocksst, BLOCKS, item_actual, MAT) \
	DO(item_roughst, ROUGH, item_actual, MAT) \
	DO(item_boulderst, BOULDER, item_actual, MAT) \
	DO(item_woodst, WOOD, item_actual, MAT) \
	DO(item_doorst, DOOR, item_constructed) \
	DO(item_floodgatest, FLOODGATE, item_constructed) \
	DO(item_bedst, BED, item_constructed) \
	DO(item_chairst, CHAIR, item_constructed) \
	DO(item_chainst, CHAIN, item_constructed) \
	DO(item_flaskst, FLASK, item_constructed) \
	DO(item_gobletst, GOBLET, item_constructed) \
	DO(item_instrumentst, INSTRUMENT, item_constructed, SUBTYPE_PTR(instrument)) \
	DO(item_toyst, TOY, item_constructed, SUBTYPE_PTR(toy)) \
	DO(item_windowst, WINDOW, item_constructed) \
	DO(item_cagest, CAGE, item_constructed) \
	DO(item_barrelst, BARREL, item_constructed) \
	DO(item_bucketst, BUCKET, item_constructed) \
	DO(item_animaltrapst, ANIMALTRAP, item_constructed) \
	DO(item_tablest, TABLE, item_constructed) \
	DO(item_coffinst, COFFIN, item_constructed) \
	DO(item_statuest, STATUE, item_constructed) \
	DO(item_corpsest, CORPSE, item_body_component) \
	DO(item_weaponst, WEAPON, item_constructed, SUBTYPE_PTR(weapon), SHARP) \
	DO(item_armorst, ARMOR, item_constructed, SUBTYPE_PTR(armor)) \
	DO(item_shoesst, SHOES, item_constructed, SUBTYPE_PTR(shoes)) \
	DO(item_shieldst, SHIELD, item_constructed, SUBTYPE_PTR(shield)) \
	DO(item_helmst, HELM, item_constructed, SUBTYPE_PTR(helm)) \
	DO(item_glovesst, GLOVES, item_constructed, SUBTYPE_PTR(gloves)) \
	DO(item_bagst, BAG, item_constructed) \
	DO(item_boxst, BOX, item_constructed) \
	DO(item_binst, BIN, item_constructed) \
	DO(item_armorstandst, ARMORSTAND, item_constructed) \
	DO(item_weaponrackst, WEAPONRACK, item_constructed) \
	DO(item_cabinetst, CABINET, item_constructed) \
	DO(item_figurinest, FIGURINE, item_constructed) \
	DO(item_amuletst, AMULET, item_constructed) \
	DO(item_scepterst, SCEPTER, item_constructed) \
	DO(item_ammost, AMMO, item_constructed, SUBTYPE_PTR(ammo), SHARP) \
	DO(item_crownst, CROWN, item_constructed) \
	DO(item_ringst, RING, item_constructed) \
	DO(item_earringst, EARRING, item_constructed) \
	DO(item_braceletst, BRACELET, item_constructed) \
	DO(item_gemst, GEM, item_constructed) \
	DO(item_anvilst, ANVIL, item_constructed) \
	DO(item_corpsepiecest, CORPSEPIECE, item_body_component) \
	DO(item_remainsst, REMAINS, item_actual, CREATURE) \
	DO(item_meatst, MEAT, item_actual, MAT) \
	DO(item_fishst, FISH, item_actual, CREATURE) \
	DO(item_fish_rawst, FISH_RAW, item_actual, CREATURE) \
	DO(item_verminst, VERMIN, item_critter) \
	DO(item_petst, PET, item_critter) \
	DO(item_seedsst, SEEDS, item_actual, MAT) \
	DO(item_plantst, PLANT, item_actual, MAT) \
	DO(item_skin_tannedst, SKIN_TANNED, item_actual, MAT) \
	DO(item_plant_growthst, PLANT_GROWTH, item_actual, MAT) \
	DO(item_threadst, THREAD, item_actual, MAT) \
	DO(item_clothst, CLOTH, item_constructed) \
	DO(item_totemst, TOTEM, item_constructed, CREATURE) \
	DO(item_pantsst, PANTS, item_constructed, SUBTYPE_PTR(pants)) \
	DO(item_backpackst, BACKPACK, item_constructed) \
	DO(item_quiverst, QUIVER, item_constructed) \
	DO(item_catapultpartsst, CATAPULTPARTS, item_constructed) \
	DO(item_ballistapartsst, BALLISTAPARTS, item_constructed) \
	DO(item_siegeammost, SIEGEAMMO, item_constructed, SUBTYPE_PTR(siegeammo), SHARP) \
	DO(item_ballistaarrowheadst, BALLISTAARROWHEAD, item_actual, MAT, SHARP) \
	DO(item_trappartsst, TRAPPARTS, item_constructed) \
	DO(item_trapcompst, TRAPCOMP, item_constructed, SUBTYPE_PTR(trapcomp), SHARP) \
	DO(item_drinkst, DRINK, item_liquid) \
	DO(item_powder_miscst, POWDER_MISC, item_powder) \
	DO(item_cheesest, CHEESE, item_actual, MAT) \
	DO(item_foodst, FOOD, item_crafted, SUBTYPE_PTR(food)) \
	DO(item_liquid_miscst, LIQUID_MISC, item_liquid) \
	DO(item_coinst, COIN, item_constructed) \
	DO(item_globst, GLOB, item_actual, MAT) \
	DO(item_rockst, ROCK, item_actual, MAT, SHARP) \
	DO(item_pipe_sectionst, PIPE_SECTION, item_constructed) \
	DO(item_hatch_coverst, HATCH_COVER, item_constructed) \
	DO(item_gratest, GRATE, item_constructed) \
	DO(item_quernst, QUERN, item_constructed) \
	DO(item_millstonest, MILLSTONE, item_constructed) \
	DO(item_splintst, SPLINT, item_constructed) \
	DO(item_crutchst, CRUTCH, item_constructed) \
	DO(item_traction_benchst, TRACTION_BENCH, item_constructed) \
	DO(item_orthopedic_castst, ORTHOPEDIC_CAST, item_constructed) \
	DO(item_toolst, TOOL, item_constructed, SUBTYPE_PTR(tool), SHARP) \
	DO(item_slabst, SLAB, item_constructed) \
	DO(item_eggst, EGG, item_actual, CREATURE) \
	DO(item_bookst, BOOK, item_constructed) \
	DO(item_sheetst, SHEET, item_constructed) \
	DO(item_branchst, BRANCH, item_actual, MAT)

#define MAT item_component_material
#define SUBTYPE_PTR(n) item_component_subtype_ptr<itemdef_##n##st>
#define SUBTYPE_ID item_component_subtype_id
#define CREATURE item_component_creature
#define SHARP item_component_sharp

#define MAKE_ITEM_TYPE(type, item_type_value, base, ...) \
struct type: FromComponents<base, #type __VA_OPT__(,) __VA_ARGS__> \
{ \
	static inline constexpr auto item_type = item_type::item_type_value; \
	item_type_t getType() const noexcept override { return item_type::item_type_value; } \
};

FOR_ALL_CONCRETE_ITEMS(MAKE_ITEM_TYPE)
#undef MAKE_ITEM_TYPE

#undef MAT
#undef SUBTYPE_PTR
#undef SUBTYPE_ID
#undef CREATURE
#undef SHARP

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

template <typename T>
struct itemdef_item;

template <typename T>
using itemdef_item_t = itemdef_item<T>::type;

template <> struct itemdef_item<itemdef_ammost> { using type = item_ammost; };
template <> struct itemdef_item<itemdef_armorst> { using type = item_armorst; };
template <> struct itemdef_item<itemdef_foodst> { using type = item_foodst; };
template <> struct itemdef_item<itemdef_glovesst> { using type = item_glovesst; };
template <> struct itemdef_item<itemdef_helmst> { using type = item_helmst; };
template <> struct itemdef_item<itemdef_instrumentst> { using type = item_instrumentst; };
template <> struct itemdef_item<itemdef_pantsst> { using type = item_pantsst; };
template <> struct itemdef_item<itemdef_shieldst> { using type = item_shieldst; };
template <> struct itemdef_item<itemdef_shoesst> { using type = item_shoesst; };
template <> struct itemdef_item<itemdef_siegeammost> { using type = item_siegeammost; };
template <> struct itemdef_item<itemdef_toyst> { using type = item_toyst; };
template <> struct itemdef_item<itemdef_trapcompst> { using type = item_trapcompst; };
template <> struct itemdef_item<itemdef_weaponst> { using type = item_weaponst; };
template <> struct itemdef_item<itemdef_toolst> { using type = item_toolst; };

} // namespace df

template <>
struct dfs::polymorphic_reader_type<df::itemimprovement> {
	using type = PolymorphicReader<df::itemimprovement,
	      df::itemimprovement_thread,
	      df::itemimprovement_cloth,
	      df::itemimprovement_instrument_piece
	>;
};

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
