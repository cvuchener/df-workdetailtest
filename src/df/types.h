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

#ifndef DF_TYPES_H
#define DF_TYPES_H

#include <dfs/CompoundReader.h>
#include <dfs/PolymorphicReader.h>
#include <dfs/ItemReader.h>

#include "df_enums.h"
#include "df/raws.h"
#include "df/time.h"

namespace df {

using namespace dfs;

struct item;

struct occupation
{
	occupation_type_t type;

	using reader_type = StructureReader<occupation, "occupation",
		Field<&occupation::type, "type">
	>;
};

struct unit_attribute
{
	int value;
	int max_value;
	int soft_demotion;

	using reader_type = StructureReader<unit_attribute, "unit_attribute",
		Field<&unit_attribute::value, "value">,
		Field<&unit_attribute::max_value, "max_value">,
		Field<&unit_attribute::soft_demotion, "soft_demotion">
	>;
};

struct unit_preference
{
	unit_preference_type_t type;
	union id_t {
		item_type_t item_type;
		int creature_id;
		int color_id;
		int shape_id;
		int plant_id;
		int poetic_form_id;
		int musical_form_id;
		int dance_form_id;

		using reader_type = UnionReader<id_t, "unit_preference.(item_type)",
			Field<&id_t::item_type, "item_type">,
			Field<&id_t::creature_id, "creature_id">,
			Field<&id_t::color_id, "color_id">,
			Field<&id_t::shape_id, "shape_id">,
			Field<&id_t::plant_id, "plant_id">,
			Field<&id_t::poetic_form_id, "poetic_form_id">,
			Field<&id_t::musical_form_id, "musical_form_id">,
			Field<&id_t::dance_form_id, "dance_form_id">
		>;
	} id;
	std::size_t get_id_type() const {
		switch (type) {
		case unit_preference_type::LikeMaterial: return -1;
		case unit_preference_type::LikeCreature: return 1;
		case unit_preference_type::LikeFood: return 0;
		case unit_preference_type::HateCreature: return 1;
		case unit_preference_type::LikeItem: return 0;
		case unit_preference_type::LikePlant: return 4;
		case unit_preference_type::LikeTree: return 4;
		case unit_preference_type::LikeColor: return 2;
		case unit_preference_type::LikeShape: return 3;
		case unit_preference_type::LikePoeticForm: return 5;
		case unit_preference_type::LikeMusicalForm: return 6;
		case unit_preference_type::LikeDanceForm: return 7;
		default: return -1;
		}
	}

	int item_subtype;
	int mat_type;
	int mat_index;
	matter_state_t mat_state;

	using reader_type = StructureReaderSeq<unit_preference, "unit_preference",
		Field<&unit_preference::type, "type">,
		Field<&unit_preference::id, "(item_type)", &unit_preference::get_id_type>,
		Field<&unit_preference::item_subtype, "item_subtype">,
		Field<&unit_preference::mat_type, "mattype">,
		Field<&unit_preference::mat_index, "matindex">,
		Field<&unit_preference::mat_state, "mat_state">
	>;

	template <unit_preference_type_t Type>
	auto data() const noexcept {
		if constexpr (Type == unit_preference_type_t::LikeMaterial)
			return std::make_tuple(mat_type, mat_index, mat_state);
		else if constexpr (Type == unit_preference_type::LikeCreature)
			return id.creature_id;
		else if constexpr (Type == unit_preference_type::LikeFood)
			return std::make_tuple(id.item_type, item_subtype, mat_type, mat_index);
		else if constexpr (Type == unit_preference_type::HateCreature)
			return id.creature_id;
		else if constexpr (Type == unit_preference_type::LikeItem)
			return std::make_tuple(id.item_type, item_subtype);
		else if constexpr (Type == unit_preference_type::LikePlant)
			return id.plant_id;
		else if constexpr (Type == unit_preference_type::LikeTree)
			return id.plant_id;
		else if constexpr (Type == unit_preference_type::LikeColor)
			return id.color_id;
		else if constexpr (Type == unit_preference_type::LikeShape)
			return id.shape_id;
		else if constexpr (Type == unit_preference_type::LikePoeticForm)
			return id.poetic_form_id;
		else if constexpr (Type == unit_preference_type::LikeMusicalForm)
			return id.musical_form_id;
		else if constexpr (Type == unit_preference_type::LikeDanceForm)
			return id.dance_form_id;
	}

	std::strong_ordering operator<=>(const unit_preference &other) const
	{
		std::strong_ordering type_comp = type <=> other.type;
		if (type_comp != std::strong_ordering::equal)
			return type_comp;
		auto comp = [&]<auto Type>() -> std::strong_ordering {
			return data<Type>() <=> other.data<Type>();
		};
		switch (type) {
		case unit_preference_type::LikeMaterial:
			return comp.operator()<df::unit_preference_type::LikeMaterial>();
		case unit_preference_type::LikeCreature:
			return comp.operator()<df::unit_preference_type::LikeCreature>();
		case unit_preference_type::LikeFood:
			return comp.operator()<df::unit_preference_type::LikeFood>();
		case unit_preference_type::HateCreature:
			return comp.operator()<df::unit_preference_type::HateCreature>();
		case unit_preference_type::LikeItem:
			return comp.operator()<df::unit_preference_type::LikeItem>();
		case unit_preference_type::LikePlant:
			return comp.operator()<df::unit_preference_type::LikePlant>();
		case unit_preference_type::LikeTree:
			return comp.operator()<df::unit_preference_type::LikeTree>();
		case unit_preference_type::LikeColor:
			return comp.operator()<df::unit_preference_type::LikeColor>();
		case unit_preference_type::LikeShape:
			return comp.operator()<df::unit_preference_type::LikeShape>();
		case unit_preference_type::LikePoeticForm:
			return comp.operator()<df::unit_preference_type::LikePoeticForm>();
		case unit_preference_type::LikeMusicalForm:
			return comp.operator()<df::unit_preference_type::LikeMusicalForm>();
		case unit_preference_type::LikeDanceForm:
			return comp.operator()<df::unit_preference_type::LikeDanceForm>();
		default:
			return std::strong_ordering::equal;
		}
	}
	bool operator==(const unit_preference &other) const {
		return *this <=> other == std::strong_ordering::equal;
	}
};

struct unit_skill
{
	job_skill_t id;
	skill_rating_t rating;
	int experience;
	int rusty;

	static constexpr int experience_for_next_level(int rating) {
		return 500 + 100 * rating;
	}

	static constexpr int cumulated_experience(int rating) {
		// sum of experience_for_next_level for 0..rating-1
		return 50 * (rating + 9) * rating;
	}

	enum RustLevel {
		NotRusty,
		Rusty,
		VeryRusty,
	};

	RustLevel rustLevel() const {
		auto rusty_rating = std::max(rating - rusty, 0);
		if (rusty_rating < rating) {
			if (rating > 3 && rusty_rating <= rating/4)
				return VeryRusty;
			else if (rusty_rating <= rating/2)
				return Rusty;
		}
		return NotRusty;
	}


	using reader_type = StructureReader<unit_skill, "unit_skill",
		Field<&unit_skill::id, "id">,
		Field<&unit_skill::rating, "rating">,
		Field<&unit_skill::experience, "experience">,
		Field<&unit_skill::rusty, "rusty">
	>;
};

struct unit_soul
{
	std::array<unit_attribute, mental_attribute_type::Count> mental_attrs;
	std::vector<std::unique_ptr<unit_skill>> skills;
	std::vector<std::unique_ptr<unit_preference>> preferences;

	using reader_type = StructureReader<unit_soul, "unit_soul",
		Field<&unit_soul::mental_attrs, "mental_attrs">,
		Field<&unit_soul::skills, "skills">,
		Field<&unit_soul::preferences, "preferences">
	>;
};

struct unit_inventory_item
{
	std::shared_ptr<df::item> item;
	unit_inventory_item_mode_t mode;

	using reader_type = StructureReader<unit_inventory_item, "unit_inventory_item",
		Field<&unit_inventory_item::item, "item">,
		Field<&unit_inventory_item::mode, "mode">
	>;
};

struct curse_attr_change
{
	std::array<int, physical_attribute_type::Count> physical_att_perc;
	std::array<int, physical_attribute_type::Count> physical_att_add;
	std::array<int, mental_attribute_type::Count> mental_att_perc;
	std::array<int, mental_attribute_type::Count> mental_att_add;

	using reader_type = StructureReader<curse_attr_change, "curse_attr_change",
		Field<&curse_attr_change::physical_att_perc, "phys_att_perc">,
		Field<&curse_attr_change::physical_att_add, "phys_att_add">,
		Field<&curse_attr_change::mental_att_perc, "ment_att_perc">,
		Field<&curse_attr_change::mental_att_add, "ment_att_add">
	>;
};

struct unit
{
	language_name name;
	profession_t profession;
	int32_t race;
	int16_t caste;
	unit_flags1 flags1;
	unit_flags2 flags2;
	unit_flags3 flags3;
	unit_flags4 flags4;
	int32_t id;
	int32_t civ_id;
	mood_type_t mood;
	std::array<unit_attribute, physical_attribute_type::Count> physical_attrs;
	struct curse_t {
		cie_add_tag_mask1 add_tags1, rem_tags1;
		std::unique_ptr<curse_attr_change> attr_change;

		using reader_type = StructureReader<curse_t, "unit.curse",
			Field<&curse_t::add_tags1, "add_tags1">,
			Field<&curse_t::rem_tags1, "rem_tags1">,
			Field<&curse_t::attr_change, "attr_change">
		>;
	} curse;
	uintptr_t undead;
	std::array<bool, unit_labor::Count> labors;
	int hist_figure_id;
	std::vector<std::unique_ptr<occupation>> occupations;
	std::unique_ptr<unit_soul> current_soul;
	std::vector<std::unique_ptr<unit_inventory_item>> inventory;
	year birth_year;
	tick birth_tick;
	time time_on_site;
	int pet_owner;

	using reader_type = StructureReader<unit, "unit",
		Field<&unit::name, "name">,
		Field<&unit::profession, "profession">,
		Field<&unit::race, "race">,
		Field<&unit::caste, "caste">,
		Field<&unit::flags1, "flags1">,
		Field<&unit::flags2, "flags2">,
		Field<&unit::flags3, "flags3">,
		Field<&unit::flags4, "flags4">,
		Field<&unit::id, "id">,
		Field<&unit::civ_id, "civ_id">,
		Field<&unit::mood, "mood">,
		Field<&unit::physical_attrs, "body.physical_attrs">,
		Field<&unit::curse, "curse">,
		Field<&unit::undead, "enemy.undead">,
		Field<&unit::labors, "status.labors">,
		Field<&unit::hist_figure_id, "hist_figure_id">,
		Field<&unit::occupations, "occupations">,
		Field<&unit::current_soul, "status.current_soul">,
		Field<&unit::inventory, "inventory">,
		Field<&unit::birth_year, "birth_year">,
		Field<&unit::birth_tick, "birth_time">,
		Field<&unit::time_on_site, "curse.time_on_site">,
		Field<&unit::pet_owner, "relationship_ids[Pet]">
	>;
};

struct identity
{
	int id;
	language_name name;
	identity_type_t type;

	using reader_type = StructureReader<identity, "identity",
		Field<&identity::id, "id">,
		Field<&identity::name, "name">,
		Field<&identity::type, "type">
	>;
};

struct historical_figure_info {
	struct reputation_t {
		int cur_identity;

		using reader_type = StructureReader<reputation_t, "historical_figure_info.reputation",
		      Field<&reputation_t::cur_identity, "cur_identity">
		>;
	};
	std::unique_ptr<reputation_t> reputation;

	using reader_type = StructureReader<historical_figure_info, "historical_figure_info",
	      Field<&historical_figure_info::reputation, "reputation">
	>;
};

struct histfig_hf_link
{
	int target;
	int strength;

	virtual histfig_hf_link_type_t type() const
	{
		return static_cast<histfig_hf_link_type_t>(-1);
	}

	using reader_type = StructureReader<histfig_hf_link, "histfig_hf_link",
		Field<&histfig_hf_link::target, "target_hf">,
		Field<&histfig_hf_link::strength, "link_strength">
	>;
};

struct histfig_hf_link_spouse: histfig_hf_link
{
	histfig_hf_link_type_t type() const override
	{
		return histfig_hf_link_type_t::SPOUSE;
	}

	using reader_type = StructureReader<histfig_hf_link_spouse, "histfig_hf_link_spousest",
		Base<histfig_hf_link>
	>;
};

struct histfig_entity_link
{
	using fallback = dfs::fallback_base;

	virtual ~histfig_entity_link() = default;

	virtual histfig_entity_link_type_t type() const
	{
		return static_cast<histfig_entity_link_type_t>(-1);
	}

	int entity_id;
	int link_strength;

	using reader_type = StructureReader<histfig_entity_link, "histfig_entity_link",
		Field<&histfig_entity_link::entity_id, "entity_id">,
		Field<&histfig_entity_link::link_strength, "link_strength">
	>;
};

struct histfig_entity_link_member: histfig_entity_link
{
	histfig_entity_link_type_t type() const override
	{
		return histfig_entity_link_type::MEMBER;
	}

	using reader_type = StructureReader<histfig_entity_link_member, "histfig_entity_link_memberst",
		Base<histfig_entity_link>
	>;
};

struct histfig_entity_link_position: histfig_entity_link
{
	int assignment_id;

	histfig_entity_link_type_t type() const override
	{
		return histfig_entity_link_type::POSITION;
	}

	using reader_type = StructureReader<histfig_entity_link_position, "histfig_entity_link_positionst",
		Base<histfig_entity_link>,
		Field<&histfig_entity_link_position::assignment_id, "assignment_id">
	>;
};

struct historical_figure
{
	int race, caste;
	language_name name;
	int id;
	std::unique_ptr<historical_figure_info> info;
	std::vector<std::unique_ptr<histfig_entity_link>> entity_links;
	std::vector<std::unique_ptr<histfig_hf_link>> histfig_links;

	using reader_type = StructureReader<historical_figure, "historical_figure",
		Field<&historical_figure::race, "race">,
		Field<&historical_figure::caste, "caste">,
		Field<&historical_figure::name, "name">,
		Field<&historical_figure::id, "id">,
		Field<&historical_figure::info, "info">,
		Field<&historical_figure::entity_links, "entity_links">,
		Field<&historical_figure::histfig_links, "histfig_links">
	>;
};

struct entity_position
{
	int id;
	FlagArray<entity_position_flags_t> flags;

	using reader_type = StructureReader<entity_position, "entity_position",
		Field<&entity_position::id, "id">,
		Field<&entity_position::flags, "flags">
	>;
};

struct entity_position_assignment
{
	int id;
	int position_id;

	using reader_type = StructureReader<entity_position_assignment, "entity_position_assignment",
		Field<&entity_position_assignment::id, "id">,
		Field<&entity_position_assignment::position_id, "position_id">
	>;
};

struct historical_entity
{
	int id;

	struct positions_t {
		std::vector<std::unique_ptr<entity_position>> own;
		std::vector<std::unique_ptr<entity_position_assignment>> assignments;

		using reader_type = StructureReader<positions_t, "historical_entity.positions",
			Field<&positions_t::own, "own">,
			Field<&positions_t::assignments, "assignments">
		>;
	} positions;

	using reader_type = StructureReader<historical_entity, "historical_entity",
		Field<&historical_entity::id, "id">,
		Field<&historical_entity::positions, "positions">
	>;
};

struct work_detail
{
	std::string name;
	work_detail_flags flags;
	std::vector<int> assigned_units;
	std::array<bool, unit_labor::Count> allowed_labors;
	work_detail_icon_t icon;

	using reader_type = StructureReader<work_detail, "work_detail",
	      Field<&work_detail::name, "name">,
	      Field<&work_detail::flags, "work_detail_flags">,
	      Field<&work_detail::assigned_units, "assigned_units">,
	      Field<&work_detail::allowed_labors, "allowed_labors">,
	      Field<&work_detail::icon, "icon">
	>;
};

struct viewscreen
{
	using fallback = fallback_base;

	virtual ~viewscreen() = default;

	std::unique_ptr<viewscreen> child;

	using reader_type = StructureReader<viewscreen, "viewscreen",
	      Field<&viewscreen::child, "child">
	>;
};

struct viewscreen_setupdwarfgame: viewscreen
{
	std::vector<std::unique_ptr<df::unit>> units;

	using reader_type = StructureReader<viewscreen_setupdwarfgame, "viewscreen_setupdwarfgamest",
	      Base<viewscreen>,
	      Field<&viewscreen_setupdwarfgame::units, "s_unit">
	>;
};

} // namespace df

template <>
struct dfs::polymorphic_reader_type<df::histfig_hf_link> {
	using type = PolymorphicReader<df::histfig_hf_link,
	      df::histfig_hf_link_spouse
	>;
};

template <>
struct dfs::polymorphic_reader_type<df::histfig_entity_link> {
	using type = PolymorphicReader<df::histfig_entity_link,
	      df::histfig_entity_link_member,
	      df::histfig_entity_link_position
	>;
};

template <>
struct dfs::polymorphic_reader_type<df::viewscreen> {
	using type = PolymorphicReader<df::viewscreen,
	      df::viewscreen_setupdwarfgame
	>;
};


#endif
