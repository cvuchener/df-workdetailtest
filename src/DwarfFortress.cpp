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

#include <QtConcurrent>
#include <QCoroFuture>
#include <QCoroSignal>

#include <dfs/Structures.h>
#include <dfs/Reader.h>
#include "DFHackProcess.h"
#include "ProcessStats.h"

#include "Application.h"
#include "StandardPaths.h"
#include "LogCategory.h"

#include "DwarfFortressData.h"
#include "DwarfFortressReader.h"

#include "df/types.h"

#include <dfhack-client-qt/Basic.h>

static void StructuresLogger(std::string_view msg)
{
	qCWarning(StructuresLog) << msg;
};

struct DwarfFortress::StructuresInfo
{
	const dfs::Structures *structures;
	const dfs::Structures::VersionInfo *version;
	QString source;
};

bool DwarfFortress::IdLess::operator()(std::span<const uint8_t> lhs, std::span<const uint8_t> rhs) const
{
	return std::ranges::lexicographical_compare(lhs, rhs);
}

static std::string id_to_string(std::span<const uint8_t> id)
{
	std::string out;
	out.reserve(2*id.size());
	auto it = std::back_inserter(out);
	for (auto byte: id)
		it = std::format_to(it, "{:02x}", byte);
	return out;
}
// DFHack API
static const DFHack::Basic Basic;
static const DFHack::Function<
	dfproto::EmptyMessage,
	dfproto::workdetailtest::ProcessInfo
> GetProcessInfo = {"workdetailtest", "GetProcessInfo"};
static const DFHack::Function<
	dfproto::EmptyMessage,
	dfproto::workdetailtest::GameState
> GetGameState = {"workdetailtest", "GetGameState"};

DwarfFortress::DwarfFortress(QObject *parent):
	QObject(parent),
	_state(Disconnected),
	_world_loaded(0),
	_map_loaded(0),
	_last_viewscreen(Viewscreen::Other)
{
	_data = std::make_shared<DwarfFortressData>(&_dfhack);
	for (QDir data_dir: StandardPaths::data_locations()) {
		QDir structs_dir = data_dir.filePath("structures");
		if (!structs_dir.exists())
			continue;
		for (const auto &subdir: structs_dir.entryList({},
					QDir::Dirs | QDir::NoDotAndDotDot,
					QDir::Name  | QDir::Reversed)) {
			QDir struct_dir = structs_dir.filePath(subdir);
			qCInfo(StructuresLog) << "Loading structures from"
				<< struct_dir.absolutePath();
			try {
				auto structures = std::make_unique<dfs::Structures>(
						struct_dir.filesystemAbsolutePath(),
						StructuresLogger);
				if (!DwarfFortressReader::testStructures(*structures)) {
					qCCritical(StructuresLog) << "Incompatible structures";
					continue;
				}
				for (const auto &version: structures->allVersions()) {
					auto [it, inserted] = _structures_by_id.try_emplace(
							version.id,
							StructuresInfo{
								structures.get(),
								&version,
								struct_dir.absolutePath()
							});
					if (inserted) {
						qCInfo(StructuresLog) << "Adding version"
							<< version.version_name
							<< id_to_string(version.id)
							<< "from" << struct_dir.absolutePath();
					}
					else {
						qCInfo(StructuresLog) << "Version already added"
							<< version.version_name
							<< id_to_string(version.id)
							<< "from" << it->second.source
							<< "as" << it->second.version->version_name;
					}
				}
				_structures.push_back(std::move(structures));
			}
			catch (std::exception &e) {
				qCCritical(StructuresLog) << "Failed to load structures from"
					<< struct_dir.absolutePath() << e.what();
			}
		}
	}

	const auto &settings = Application::settings();
	connect(&_dfhack, &DFHack::Client::connectionChanged,
		this, &DwarfFortress::onConnectionChanged);
	connect(&_dfhack, &DFHack::Client::notification,
		this, &DwarfFortress::onNotification);
	connect(&settings.autorefresh_interval, &SettingPropertyBase::valueChanged,
		this, &DwarfFortress::onAutorefreshIntervalChanged);
	onAutorefreshIntervalChanged();
	connect(&settings.autorefresh_enabled, &SettingPropertyBase::valueChanged,
		this, &DwarfFortress::onAutorefreshEnabledChanged);
	_refresh_timer.setSingleShot(true);
	connect(&_refresh_timer, &QTimer::timeout,
		this, &DwarfFortress::heartbeat);
}

DwarfFortress::~DwarfFortress()
{
	qDebug() << "DwarfFortress clean up";
	QCoro::waitFor([this]() -> QCoro::Task<void> {
		if (_state != Disconnected) {
			qDebug() << "Disconnecting...";
			co_await _dfhack.disconnect();
		}
		if (_coroutine_counter.value() != 0) {
			qDebug() << "Waiting for running coroutines...";
			co_await qCoro(&_coroutine_counter, &Counter::zero);
		}
	}());
	_dfhack.QObject::disconnect(this);
	qDebug() << "DwarfFortress cleaned up";
}

template <typename T>
static QString ErrorCodeMessage(T ec)
{
	return QString::fromStdString(make_error_code(ec).message());
}

QCoro::Task<bool> DwarfFortress::connectToDF(const QString &host, quint16 port)
{
	if (_state != Disconnected)
		co_return false;
	CounterGuard coroutine_guard(_coroutine_counter);
	setState(Connecting);
	connectionProgress(tr("Connecting to DFHack"));
	if (!co_await _dfhack.connect(host, port)) {
		setState(Disconnected);
		qCCritical(DFHackLog) << "Failed to connect to DFHack";
		error(tr("Connection failed"));
		co_return false;
	}
	try {
		connectionProgress(tr("Retrieving DFHack information"));
		auto calls = std::make_tuple(
			Basic.getVersion(_dfhack).first,
			Basic.getDFVersion(_dfhack).first,
			GetProcessInfo(_dfhack).first
		);
		auto version_result = co_await get<0>(calls);
		auto df_version_result = co_await get<1>(calls);
		if (!version_result || !df_version_result)
			throw tr("Failed to get versions (%1, %2)")
				.arg(ErrorCodeMessage(version_result.cr))
				.arg(ErrorCodeMessage(df_version_result.cr));
		_dfhack_version = QString::fromUtf8(version_result->value());
		qCInfo(DFHackLog) << "DFHack version:" << _dfhack_version;
		_df_version = QString::fromUtf8(df_version_result->value());
		qCInfo(DFHackLog) << "DF version:" << _df_version;
		auto process_info = co_await get<2>(calls);
		if (!process_info)
			throw tr("Failed to get process info (%1)")
				.arg(ErrorCodeMessage(process_info.cr));

		connectionProgress(tr("Opening DF process"));
		co_await QtConcurrent::run([this, &process_info](){
			std::unique_ptr<dfs::Process> process = nullptr;
			if (Application::settings().use_native_process())
				process = findNativeProcess(*process_info);
			if (!process) {
				qCInfo(ProcessLog) << "Fallback to DFHack for memory access";
				process = std::make_unique<DFHackProcess>(_dfhack);
			}
			_process = std::make_unique<dfs::ProcessVectorizer>(
#ifdef QT_DEBUG
					std::make_unique<ProcessStats>(std::move(process)),
#else
					std::move(process),
#endif
					48*1024*1024); // keep a margin below DFHack max message size for protocol overhead
		});
		if (!_process)
			throw tr("Failed to open DF process");
		qCInfo(ProcessLog) << "Process id" << id_to_string(_process->id());
		auto it = _structures_by_id.find(_process->id());
		if (it == _structures_by_id.end())
			throw tr("Unsupported DF version");
		auto structures = it->second.structures;
		auto version = it->second.version;
		qCInfo(StructuresLog) << "Version found as" << version->version_name
				<< "from" << it->second.source;
		_reader_factory = std::make_unique<dfs::ReaderFactory>(*structures, *version);
		_reader_factory->log = StructuresLogger;

		setState(Connected);
		update();
	}
	catch (std::exception &e) {
		_reader_factory.reset();
		_process.reset();
		_dfhack.disconnect();
		qCritical() << "Failed to connect" << e.what();
		error(e.what());
		co_return false;
	}
	catch (QString &message) {
		_reader_factory.reset();
		_process.reset();
		_dfhack.disconnect();
		qCritical() << "Failed to connect" << message;
		error(message);
		co_return false;
	}
	co_return true;
}

QCoro::Task<> DwarfFortress::disconnectFromDF()
{
	CounterGuard coroutine_guard(_coroutine_counter);
	co_await _dfhack.disconnect();
}

QCoro::Task<bool> DwarfFortress::heartbeat()
{
	if (_state != Connected)
		co_return false;
	CounterGuard coroutine_guard(_coroutine_counter);
	auto reply = co_await GetGameState(_dfhack).first;
	if (!reply) {
		_dfhack.disconnect();
		qCCritical(DFHackLog) << "getGameState failed" << ErrorCodeMessage(reply.cr);
		error(tr("Failed to get game state (%1)").arg(ErrorCodeMessage(reply.cr)));
		co_return false;
	}
	auto world_changed = _world_loaded != reply->world_loaded();
	auto map_changed = _map_loaded != reply->map_loaded();
	auto current_viewscreen = Viewscreen::Other;
	if (reply->has_viewscreen()) {
		switch (reply->viewscreen()) {
		case dfproto::workdetailtest::Viewscreen::SetupDwarfGame:
			current_viewscreen = Viewscreen::SetupDwarfGame;
			break;
		default:
			current_viewscreen = Viewscreen::Other;
			break;
		}
	}
	auto viewscreen_changed = current_viewscreen != _last_viewscreen;
	if (world_changed || map_changed || viewscreen_changed) {
		co_return co_await update();
	}
	else {
		if (Application::settings().autorefresh_enabled())
			_refresh_timer.start();
		co_return true;
	}
}

QCoro::Task<bool> DwarfFortress::update()
{
	if (_state != Connected)
		co_return false;
	CounterGuard coroutine_guard(_coroutine_counter);
	setState(Updating);
	auto ret = co_await QtConcurrent::run([=, this]() {
		try {
			DwarfFortressReader reader({*_reader_factory, *_process});
			reader.session.addSharedObjectsCache<df::itemdef>(_shared_raws_objects);

			connectionProgress(tr("Reading world state"));
			auto current_world = reader.getWorldDataPtr();

			if (current_world != _world_loaded) {
				_world_loaded = current_world;
				if (_world_loaded == 0)
					return true;
				connectionProgress(tr("Loading raws"));
				_shared_raws_objects.clear();
				auto raws = reader.loadRaws();
				QMetaObject::invokeMethod(this, [this, raws = std::move(raws)]() mutable {
					_data->updateRaws(std::move(raws));
				}, Qt::BlockingQueuedConnection);
			}
			if (_world_loaded != 0) {
				connectionProgress(tr("Loading game data"));
				auto data = reader.loadGameData();
				QMetaObject::invokeMethod(this, [this, data = std::move(data)]() mutable {
					_map_loaded = data->map_block_index;
					auto units = &data->units;
					_last_viewscreen = Viewscreen::Other;
					for (auto view = data->viewscreen.get(); view; view = view->child.get()) {
						if (auto setupdwarfgame = dynamic_cast<df::viewscreen_setupdwarfgame *>(view)) {
							qDebug() << "Use embark screen";
							units = &setupdwarfgame->units;
							_last_viewscreen = Viewscreen::SetupDwarfGame;
							break;
						}
					}
					_data->updateGameData(std::move(data), std::move(*units));
				}, Qt::BlockingQueuedConnection);
			}
			return true;
		}
		catch (std::exception &e) {
			qCritical() << "Failed to update" << e.what();
			error(e.what());
			return false;
		}
	});
	if (_world_loaded == 0)
		clearData();
	if (_state == Updating) {
		setState(Connected);
		if (Application::settings().autorefresh_enabled())
			_refresh_timer.start();
	}
	co_return ret;
}

void DwarfFortress::onConnectionChanged(bool connected)
{
	if (!connected) {
		_refresh_timer.stop();
		QCoro::waitFor([this]() -> QCoro::Task<void> {
			if (_coroutine_counter.value() != 0) {
				qDebug() << "Waiting for running coroutines...";
				co_await qCoro(&_coroutine_counter, &Counter::zero);
				qDebug() << "All coroutines finished";
			}
		}());
		clearData();
		setState(Disconnected);
	}
}

void DwarfFortress::onNotification(DFHack::Color color, const QString &text)
{
	qCInfo(DFHackLog) << text;
}

void DwarfFortress::onAutorefreshIntervalChanged()
{
	auto interval = Application::settings().autorefresh_interval();
	_refresh_timer.setInterval(interval*1000);
}

void DwarfFortress::onAutorefreshEnabledChanged()
{
	if (_state != Connected)
		return;
	if (Application::settings().autorefresh_enabled())
		_refresh_timer.start();
	else
		_refresh_timer.stop();
}

void DwarfFortress::setState(State state)
{
	if (_state != state)
		stateChanged(_state = state);
}

void DwarfFortress::clearData()
{
	_world_loaded = 0;
	_map_loaded = 0;
	_last_viewscreen = Viewscreen::Other;

	_data->clear();
	_shared_raws_objects.clear();
}
