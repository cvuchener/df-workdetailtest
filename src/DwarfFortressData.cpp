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
	dfhack(dfhack),
	units(std::make_unique<ObjectList<Unit>>()),
	work_details(std::make_unique<WorkDetailModel>(*this))
{
}

DwarfFortressData::~DwarfFortressData()
{
}

const df::creature_raw *DwarfFortressData::creature(int creature_id) const noexcept
{
	if (raws)
		return df::get(raws->creatures.all, creature_id);
	else
		return nullptr;
}

const df::caste_raw *DwarfFortressData::caste(int creature_id, int caste_id) const noexcept
{
	if (auto c = creature(creature_id))
		return df::get(c->caste, caste_id);
	return nullptr;
}

QString DwarfFortressData::creatureName(int creature_id, bool plural, int caste_id) const noexcept
{
	if (auto crt = creature(creature_id)) {
		if (auto cst = df::get(crt->caste, caste_id))
			return df::fromCP437(cst->caste_name[plural ? 1 : 0]);
		else
			return df::fromCP437(crt->name[plural ? 1 : 0]);
	}
	return {};
}

const df::plant_raw *DwarfFortressData::plant(int plant_id) const noexcept
{
	if (raws)
		return df::get(raws->plants.all, plant_id);
	else
		return nullptr;
}

void DwarfFortressData::updateRaws(std::unique_ptr<df::world_raws> &&new_raws)
{
	raws = std::move(new_raws);
	rawsUpdated();
}

void DwarfFortressData::updateGameData(
		std::unique_ptr<df_game_data> &&new_data,
		std::vector<std::unique_ptr<df::unit>> &&new_units)
{
	current_civ_id = new_data->current_civ_id;
	current_group_id = new_data->current_group_id;
	current_time = df::time(new_data->current_year) + new_data->current_tick;
	entities = std::move(new_data->entities);
	histfigs = std::move(new_data->histfigs);
	identities = std::move(new_data->identities);
	units->update(std::move(new_units), [this](auto &&u) {
		return std::make_shared<Unit>(std::move(u), *this);
	});
	work_details->update(std::move(new_data->work_details), [this](auto &&wd) {
		return std::make_shared<WorkDetail>(std::move(wd), *this);
	});
	gameDataUpdated();
}

void DwarfFortressData::clear()
{
	units->clear();
	work_details->clear();
	gameDataUpdated();

	identities.clear();
	raws.reset();
	rawsUpdated();
}
