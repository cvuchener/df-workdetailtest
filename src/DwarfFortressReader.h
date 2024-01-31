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

#ifndef DWARF_FORTRESS_READER_H
#define DWARF_FORTRESS_READER_H

#include <dfs/Reader.h>

#include "df/time.h"

namespace df {
struct creature_raw;
struct historical_entity;
struct historical_figure;
struct identity;
struct inorganic_raw;
struct language_name;
struct material;
struct plant_raw;
struct unit;
struct viewscreen;
struct work_detail;
struct world_raws;
}

struct df_game_data {
	int current_civ_id;
	int current_group_id;
	df::year current_year;
	df::tick current_tick;
	std::vector<std::unique_ptr<df::unit>> units;
	std::vector<std::unique_ptr<df::historical_entity>> entities;
	std::vector<std::unique_ptr<df::historical_figure>> histfigs;
	std::vector<std::unique_ptr<df::identity>> identities;
	std::vector<std::unique_ptr<df::work_detail>> work_details;
	std::unique_ptr<df::viewscreen> viewscreen;
	uintptr_t map_block_index;
};

struct DwarfFortressReader
{
	dfs::ReadSession session;

	uintptr_t getWorldDataPtr();
	std::unique_ptr<df::world_raws> loadRaws();
	std::unique_ptr<df_game_data> loadGameData();
	static bool testStructures(const dfs::Structures &structures);
};

#endif
