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

#include <dfhack-client-qt/Client.h>
#include "workdetailtest.pb.h"

#include "Counter.h"

#include <dfs/Reader.h>

namespace dfs {
class Process;
}

class DwarfFortressData;

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

	DFHack::Client &dfhack() { return _dfhack; }
	const std::shared_ptr<DwarfFortressData> &data() { return _data; }

public slots:
	QCoro::Task<bool> connectToDF(const QString &host, quint16 port);
	QCoro::Task<> disconnectFromDF();
	QCoro::Task<bool> heartbeat();
	QCoro::Task<bool> update();

signals:
	void stateChanged(State);
	void error(const QString &);
	void connectionProgress(const QString &);

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

	QTimer _refresh_timer;

	// Process info
	static std::unique_ptr<dfs::Process> findNativeProcess(const dfproto::workdetailtest::ProcessInfo &info);
	std::unique_ptr<dfs::Process> _process;
	std::unique_ptr<dfs::ReaderFactory> _reader_factory;
	uintptr_t _world_loaded;
	uintptr_t _map_loaded;
	enum class Viewscreen {
		SetupDwarfGame,
		Other
	} _last_viewscreen;

	dfs::ReadSession::shared_objects_cache_t _shared_raws_objects;
	std::shared_ptr<DwarfFortressData> _data;

	Counter _coroutine_counter;
	DFHack::Client _dfhack;
};

#endif
