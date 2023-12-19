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

#include "DwarfFortressData.h"

#include <ObjectList.h>
#include <Unit.h>
#include <WorkDetail.h>
#include <WorkDetailModel.h>

#include "df/utils.h"

DwarfFortressData::DwarfFortressData(QPointer<DFHack::Client> dfhack):
	units(std::make_unique<ObjectList<Unit>>()),
	work_details(std::make_unique<WorkDetailModel>(*this, dfhack))
{
}

DwarfFortressData::~DwarfFortressData()
{
}

std::pair<const df::material *, DwarfFortressData::material_origin>
DwarfFortressData::findMaterial(int type, int index) const
{
	static constexpr std::size_t CreatureBase = 19;
	static constexpr std::size_t HistFigureBase = 219;
	static constexpr std::size_t PlantBase = 419;
	static constexpr std::size_t MaxMaterialType = 200;

	if (!raws)
		return {nullptr, {}};

	auto check_index = [](const auto &vec, std::size_t index) -> decltype(vec[0].get()) {
		if (index < vec.size())
			return vec[index].get();
		else
			return nullptr;
	};

	std::size_t creature_mat = type - CreatureBase;
	if (creature_mat < MaxMaterialType) {
		auto default_creature_mat = raws->builtin_mats[CreatureBase].get();
		if (auto creature = check_index(raws->creatures.all, index)) {
			if (auto mat = check_index(creature->material, creature_mat))
				return {mat, creature};
			return {default_creature_mat, creature};
		}
		return {default_creature_mat, {}};
	}

	std::size_t histfig_mat = type - HistFigureBase;
	if (histfig_mat < MaxMaterialType) {
		auto default_creature_mat = raws->builtin_mats[CreatureBase].get();
		if (auto histfig = df::find(histfigs, index)) {
			if (auto creature = check_index(raws->creatures.all, histfig->race))
				if (auto mat = check_index(creature->material, histfig_mat))
					return {mat, histfig};
			return {default_creature_mat, histfig};
		}
		return {default_creature_mat, {}};
	}

	std::size_t plant_mat = type - PlantBase;
	if (plant_mat < MaxMaterialType) {
		auto default_plant_mat = raws->builtin_mats[PlantBase].get();
		if (auto plant = check_index(raws->plants.all, index)) {
			if (auto mat = check_index(plant->material, plant_mat))
				return {mat, plant};
			return {default_plant_mat, plant};
		}
		return {default_plant_mat, {}};
	}

	if (type == df::builtin_mats::INORGANIC) {
		if (auto inorganic = check_index(raws->inorganics, index))
			return {&inorganic->material, inorganic};
		return {raws->builtin_mats[df::builtin_mats::INORGANIC].get(), {}};
	}

	if (type >= 0 && unsigned(type) < raws->builtin_mats.size())
		return {raws->builtin_mats[type].get(), {}};

	return {nullptr, {}};
}

void DwarfFortressData::updateRaws(std::unique_ptr<df::world_raws> &&new_raws)
{
	raws = std::move(new_raws);
}

void DwarfFortressData::updateGameData(
		std::unique_ptr<df_game_data> &&new_data,
		std::vector<std::unique_ptr<df::unit>> &&new_units,
		DFHack::Client &dfhack)
{
	current_civ_id = new_data->current_civ_id;
	current_group_id = new_data->current_group_id;
	current_time = df::time(new_data->current_year) + new_data->current_tick;
	entities = std::move(new_data->entities);
	histfigs = std::move(new_data->histfigs);
	identities = std::move(new_data->identities);
	units->update(std::move(new_units), [this, &dfhack](auto &&u) {
		return std::make_shared<Unit>(std::move(u), *this, dfhack);
	});
	work_details->update(std::move(new_data->work_details), [this, &dfhack](auto &&wd) {
		return std::make_shared<WorkDetail>(std::move(wd), *this, dfhack);
	});
}

void DwarfFortressData::clear()
{
	units->clear();
	work_details->clear();

	identities.clear();
	raws.reset();
}
