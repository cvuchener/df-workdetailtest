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

#ifndef DWARF_FORTRESS_DATA_H
#define DWARF_FORTRESS_DATA_H

#include "DwarfFortressReader.h"

#include <QObject>
#include <QPointer>

class Unit;
class WorkDetailModel;
template <typename T>
class ObjectList;

namespace DFHack { class Client; }

namespace df {
struct creature_raw;
struct caste_raw;
}

class DwarfFortressData: public QObject, public std::enable_shared_from_this<DwarfFortressData>
{
	Q_OBJECT
public:
	QPointer<DFHack::Client> dfhack;

	std::unique_ptr<df::world_raws> raws;

	const df::creature_raw *creature(int creature_id) const noexcept;
	const df::caste_raw *caste(int creature_id, int caste_id) const noexcept;
	const df::plant_raw *plant(int plant_id) const noexcept;

	QString creatureName(int creature_id, bool plural, int caste_id = -1) const noexcept;

	int current_civ_id;
	int current_group_id;
	df::time current_time;
	std::vector<std::unique_ptr<df::historical_entity>> entities;
	std::vector<std::unique_ptr<df::historical_figure>> histfigs;
	std::vector<std::unique_ptr<df::identity>> identities;
	std::unique_ptr<ObjectList<Unit>> units;
	std::unique_ptr<WorkDetailModel> work_details;

	DwarfFortressData(QPointer<DFHack::Client> dfhack);
	~DwarfFortressData();

	void updateRaws(std::unique_ptr<df::world_raws> &&new_raws);
	void updateGameData(
			std::unique_ptr<df_game_data> &&new_data,
			std::vector<std::unique_ptr<df::unit>> &&new_units);

	void clear();

signals:
	void rawsUpdated();
	void gameDataUpdated();
};

#endif
