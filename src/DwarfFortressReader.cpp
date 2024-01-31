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

#include "DwarfFortressReader.h"

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(StructuresLog);

#include <dfs/Reader.h>

#include <df/types.h>
#include <df/items.h>

using namespace dfs;

template <static_string Path, auto Output>
struct GlobalRead {
	static constexpr auto path = parse_path<Path>();
	static constexpr auto output = Output;
};

template <typename T>
struct reads;

template <typename T>
using reads_t = reads<T>::type;

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
	return test_all_t<T, reads_t<T>>{}(factory);
}

template <typename T>
struct read_all_t;

template <typename... Reads>
struct read_all_t<std::tuple<Reads...>>
{
	template <typename T>
	bool operator()(ReadSession &session, T &&out) const {
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
	}
};

template <typename T>
bool read_all(ReadSession &session, T &&out)
{
	return read_all_t<reads_t<std::remove_cvref_t<T>>>{}(session, out);
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
};


template <>
struct reads<df_game_state>
{
	using type = std::tuple<
		GlobalRead<"world.world_data", &df_game_state::world_data_addr>,
		GlobalRead<"world.world_data", &df_game_state::world_data>
	>;
};

uintptr_t DwarfFortressReader::getWorldDataPtr()
{
	df_game_state state;
	if (!read_all(session, state))
		throw std::runtime_error("Error while reading game state");
	if (state.world_data && !state.world_data->regions.empty())
		return state.world_data_addr;
	else
		return 0;
}

struct df_raws
{
	std::unique_ptr<df::world_raws> raws = std::make_unique<df::world_raws>();
};

template <>
struct reads<df_raws>
{
	using type = std::tuple<
		GlobalRead<"world.raws", [](df_raws &self) -> decltype(auto) { return *self.raws; }>
	>;
};

std::unique_ptr<df::world_raws> DwarfFortressReader::loadRaws()
{
	df_raws raws;
	if (!read_all(session, raws))
		throw std::runtime_error("Error while reading raws");
	return std::move(raws.raws);
}

template <>
struct reads<df_game_data>
{
	using type = std::tuple<
		GlobalRead<"gview.view", [](df_game_data &self) -> decltype(auto) { return *self.viewscreen; }>,
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

std::unique_ptr<df_game_data> DwarfFortressReader::loadGameData()
{
	auto data = std::make_unique<df_game_data>();
	data->viewscreen = std::make_unique<df::viewscreen>();
	if (!read_all(session, *data))
		throw std::runtime_error("Error while reading game data");
	return data;
}

bool DwarfFortressReader::testStructures(const Structures &structures)
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
