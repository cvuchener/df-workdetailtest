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

#ifndef LOG_DOCK_H
#define LOG_DOCK_H

#include <QSortFilterProxyModel>
#include <QDockWidget>

class LogFilter: public QSortFilterProxyModel
{
	Q_OBJECT
public:
	LogFilter(QObject *parent = nullptr);
	~LogFilter() override;

	bool typeEnabled(QtMsgType type) const { return _enabled.at(type); }
	void setTypeEnabled(QtMsgType type, bool enabled);

	void setMessageFilter(const QString &text);

protected:
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
	std::array<bool, 5> _enabled;
	QString _message_filter;
};

namespace Ui { class LogDock; }

class LogDock: public QDockWidget
{
	Q_OBJECT
public:
	LogDock(QWidget *parent = nullptr, Qt::WindowFlags flags = {});
	~LogDock() override;

private:
	std::unique_ptr<Ui::LogDock> _ui;
	LogFilter _filter;
};

#endif
