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

#include "DwarfFortress.h"

#include "ObjectList.h"
#include "Unit.h"
#include "WorkDetail.h"
#include "DFTypes.h"
#include "DFItems.h"

#include <dfs/Reader.h>
using namespace dfs;

template <static_string Path, auto Output>
struct GlobalRead {
	static constexpr auto path = parse_path<Path>();
	static constexpr auto output = Output;
};

template <typename, typename>
struct test_all_t;

template <typename T, typename... Reads>
struct test_all_t<T, std::tuple<Reads...>>
{
	bool operator()(ReaderFactory &factory) const {
		bool ok = true;
		([&]<typename Read>() {
			using field_type = std::remove_cvref_t<std::invoke_result_t<decltype(Read::output), T &>>;
			std::optional<AnyTypeRef> df_type;
			try {
				df_type = factory.structures.findGlobalObjectType(Read::path);
			}
			catch (std::exception &e) {
				qCCritical(StructuresLog) << "Failed to find type for" << path::to_string(Read::path) << e.what();
				ok = false;
				return;
			}
			try {
				factory.make_item_reader<field_type>(*df_type);
			}
			catch (std::exception &e) {
				qCCritical(StructuresLog) << "Failed to init reader for" << path::to_string(Read::path) << "as" << typeid(field_type).name() << e.what();
				ok = false;
			}
		}.template operator()<Reads>(), ...);
		return ok;
	}
};

template <typename T>
bool test_all(ReaderFactory &factory)
{
	return test_all_t<T, typename T::reads>{}(factory);
}

template <typename T>
struct read_all_t;

template <typename... Reads>
struct read_all_t<std::tuple<Reads...>>
{
	template <typename T>
	bool operator()(ReadSession &session, T &&out) const {
	{
		return session.sync([&]<typename Read>() {
			auto &out_field = std::invoke(Read::output, out);
			try {
				return session.read(Read::path, out_field);
			}
			catch (std::exception &e) {
				qCCritical(StructuresLog) << "Failed to read" << path::to_string(Read::path) << "as" << typeid(out_field).name() << "error:" << e.what();
				throw;
			}
		}.template operator()<Reads>()...);
		}(std::index_sequence_for<Reads...>{});
	}
};

template <typename T>
bool read_all(ReadSession &session, T &&out)
{
	return read_all_t<typename std::remove_cvref_t<T>::reads>{}(session, out);
}

struct df_game_state
{
	uintptr_t world_data_addr;
	struct world_data_t
	{
		std::vector<uintptr_t> regions;
		using reader_type = StructureReader<world_data_t, "world_data",
			Field<&world_data_t::regions, "regions">
		>;
	};
	std::unique_ptr<world_data_t> world_data;

	using reads = std::tuple<
		GlobalRead<"world.world_data", &df_game_state::world_data_addr>,
		GlobalRead<"world.world_data", &df_game_state::world_data>
	>;
};

uintptr_t DwarfFortress::getWorldDataPtr(ReadSession &session)
{
	df_game_state state;
	if (!read_all(session, state))
		throw tr("Error while reading game state");
	if (state.world_data && !state.world_data->regions.empty())
		return state.world_data_addr;
	else
		return 0;
}

struct df_raws
{
	std::unique_ptr<df::world_raws> raws = std::make_unique<df::world_raws>();
	df::world_raws &get_raws() { return *raws; }

	using reads = std::tuple<
		GlobalRead<"world.raws", &df_raws::get_raws>
	>;
};

void DwarfFortress::loadRaws(ReadSession &session)
{
	_shared_raws_objects.clear();
	df_raws raws;
	if (!read_all(session, raws))
		throw tr("Error while reading raws");
	QMetaObject::invokeMethod(this, [this, raws = std::move(raws.raws)]() mutable {
		_raws = std::move(raws);
	}, Qt::BlockingQueuedConnection);
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
	df::viewscreen viewscreen;
	uintptr_t map_block_index;

	using reads = std::tuple<
		GlobalRead<"gview.view", &df_game_data::viewscreen>,
		GlobalRead<"plotinfo.civ_id", &df_game_data::current_civ_id>,
		GlobalRead<"plotinfo.group_id", &df_game_data::current_group_id>,
		GlobalRead<"cur_year", &df_game_data::current_year>,
		GlobalRead<"cur_year_tick", &df_game_data::current_tick>,
		GlobalRead<"world.units.all", &df_game_data::units>,
		GlobalRead<"world.entities.all", &df_game_data::entities>,
		GlobalRead<"world.history.figures", &df_game_data::histfigs>,
		GlobalRead<"world.identities.all", &df_game_data::identities>,
		GlobalRead<"plotinfo.labor_info.work_details", &df_game_data::work_details>,
		GlobalRead<"world.map.block_index", &df_game_data::map_block_index>
	>;
};

void DwarfFortress::loadGameData(ReadSession &session)
{
	auto data = std::make_unique<df_game_data>();
	if (!read_all(session, *data))
		throw tr("Error while reading game data");
	QMetaObject::invokeMethod(this, [this, data = std::move(data)]() mutable {
		_map_loaded = data->map_block_index;
		auto units = &data->units;
		_last_viewscreen = Viewscreen::Other;
		for (auto view = &data->viewscreen; view; view = view->child.get()) {
			if (auto setupdwarfgame = dynamic_cast<df::viewscreen_setupdwarfgame *>(view)) {
				qDebug() << "Use embark screen";
				units = &setupdwarfgame->units;
				_last_viewscreen = Viewscreen::SetupDwarfGame;
				break;
			}
		}
		_current_civ_id = data->current_civ_id;
		_current_group_id = data->current_group_id;
		_current_time = df::time(data->current_year) + data->current_tick;
		_entities = std::move(data->entities);
		_histfigs = std::move(data->histfigs);
		_identities = std::move(data->identities);
		_units->update(std::move(*units), [this](auto &&u) {
			return std::make_shared<Unit>(std::move(u), *this);
		});
		_work_details->update(std::move(data->work_details), [this](auto &&wd) {
			return std::make_shared<WorkDetail>(std::move(wd), *this);
		});
	}, Qt::BlockingQueuedConnection);
}

bool DwarfFortress::testStructures(const Structures &structures)
{
	bool ok = true;
	bool first = true;
	for (const auto &version: structures.allVersions()) {
		std::optional<ReaderFactory> factory;
		qCDebug(StructuresLog) << "Testing" << version.version_name;
		try {
			factory.emplace(structures, version);
		}
		catch (std::exception &e) {
			qCCritical(StructuresLog) << "Failed to create reader factory for" << version.version_name << e.what();
			ok = false;
			continue;
		}
		if (first)
			first = false;
		else
			continue; // Skip fields test after first
		factory->log = [](auto message){ qCWarning(StructuresLog) << message; };
		if (!test_all<df_game_state>(*factory))
			ok = false;
		if (!test_all<df_raws>(*factory))
			ok = false;
		if (!test_all<df_game_data>(*factory))
			ok = false;
	}
	return ok;
}
