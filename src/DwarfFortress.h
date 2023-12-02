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

#ifndef DWARF_FORTRESS_H
#define DWARF_FORTRESS_H

#include <QObject>
#include <QTimer>

#include <QCoroTask>
#include <QLoggingCategory>

#include <dfhack-client-qt/Client.h>
#include "workdetailtest.pb.h"
#include "Counter.h"

#include <dfs/Reader.h>

namespace dfs {
class Structures;
class Process;
}

class Unit;
class WorkDetail;
template <typename T>
class ObjectList;

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
struct work_detail;
struct world_raws;
}

Q_DECLARE_LOGGING_CATEGORY(DFHackLog);
Q_DECLARE_LOGGING_CATEGORY(StructuresLog);
Q_DECLARE_LOGGING_CATEGORY(ProcessLog);

class DwarfFortress: public QObject
{
	Q_OBJECT
	Q_PROPERTY(State state READ state NOTIFY stateChanged)
public:
	DwarfFortress(QObject *parent = nullptr);
	~DwarfFortress() override;

	// Game connection
	enum State {
		Disconnected,
		Connecting,
		Connected,
		Updating,
	};
	Q_ENUM(State)
	State state() const { return _state; }

	const QString &getDFHackVersion() const { return _dfhack_version; };
	const QString &getDFVersion() const { return _df_version; };

public slots:
	QCoro::Task<bool> connectToDF(const QString &host, quint16 port);
	QCoro::Task<> disconnectFromDF();
	QCoro::Task<bool> heartbeat();
	QCoro::Task<bool> update();

signals:
	void stateChanged(State);
	void error(const QString &);
	void connectionProgress(const QString &);

public:
	// Game data access
	const df::world_raws &raws() const;

	ObjectList<Unit> &units() { return *_units; }
	ObjectList<WorkDetail> &workDetails() { return *_work_details; }
	const ObjectList<WorkDetail> &workDetails() const { return *_work_details; }
	int currentCivId() const { return _current_civ_id; }
	int currentGroupId() const { return _current_group_id; }

	const df::historical_figure *findHistoricalFigure(int id) const;
	const df::historical_entity *findHistoricalEntity(int id) const;
	const df::identity *findIdentity(int id) const;

	using material_origin = std::variant<std::monostate,
			const df::inorganic_raw *,
			const df::historical_figure *,
			const df::creature_raw *,
			const df::plant_raw *
	>;
	std::pair<const df::material *, material_origin> findMaterial(int type, int index) const;

	DFHack::Client &dfhack() { return _dfhack; }

	static bool testStructures(const dfs::Structures &structures);

private slots:
	void onConnectionChanged(bool);
	void onNotification(DFHack::Color color, const QString &text);
	void onAutorefreshIntervalChanged();
	void onAutorefreshEnabledChanged();

	void clearData();

private:
	// Connection state
	State _state;
	QString _dfhack_version;
	QString _df_version;

	void setState(State state);

	// Update game data
	uintptr_t getWorldDataPtr(dfs::ReadSession &session);
	void loadRaws(dfs::ReadSession &session);
	void loadGameData(dfs::ReadSession &session);

	QTimer _refresh_timer;

	// Process info
	static std::unique_ptr<dfs::Process> findNativeProcess(const dfproto::workdetailtest::ProcessInfo &info);
	std::vector<std::unique_ptr<dfs::Structures>> _structures;
	struct StructuresInfo;
	struct IdLess {
		bool operator()(std::span<const uint8_t>, std::span<const uint8_t>) const;
	};
	std::map<std::span<const uint8_t>, StructuresInfo, IdLess> _structures_by_id;
	std::unique_ptr<dfs::Process> _process;
	std::unique_ptr<dfs::ReaderFactory> _reader_factory;
	uintptr_t _world_loaded;
	uintptr_t _map_loaded;
	enum class Viewscreen {
		SetupDwarfGame,
		Other
	} _last_viewscreen;

	// Game data
	std::shared_ptr<df::world_raws> _raws;
	dfs::ReadSession::shared_objects_cache_t _shared_raws_objects;

	int _current_civ_id;
	int _current_group_id;
	std::vector<std::unique_ptr<df::historical_entity>> _entities;
	std::vector<std::unique_ptr<df::historical_figure>> _histfigs;
	std::vector<std::unique_ptr<df::identity>> _identities;
	std::unique_ptr<ObjectList<Unit>> _units;
	std::unique_ptr<ObjectList<WorkDetail>> _work_details;

	Counter _coroutine_counter;
	// make dfhack client last so it is destroyed first
	DFHack::Client _dfhack;
};

#endif
