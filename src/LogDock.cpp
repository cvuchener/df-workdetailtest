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

#include "LogDock.h"

#include "ui_LogDock.h"

#include "MessageHandler.h"

LogFilter::LogFilter(QObject *parent):
	QSortFilterProxyModel(parent)
{
	_enabled[QtWarningMsg] = true;
	_enabled[QtCriticalMsg] = true;
	_enabled[QtFatalMsg] = true;
#ifdef QT_DEBUG
	_enabled[QtDebugMsg] = true;
	_enabled[QtInfoMsg] = true;
#else
	_enabled[QtDebugMsg] = false;
	_enabled[QtInfoMsg] = false;
#endif
};

LogFilter::~LogFilter()
{
}

void LogFilter::setTypeEnabled(QtMsgType type, bool enabled)
{
	_enabled[type] = enabled;
	invalidateRowsFilter();
}

void LogFilter::setMessageFilter(const QString &text)
{
	_message_filter = text;
	invalidateRowsFilter();
}

bool LogFilter::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	auto type_index = sourceModel()->index(
			source_row,
			static_cast<int>(MessageHandler::Columns::Type),
			source_parent);
	if (!_enabled.at(type_index.data(Qt::UserRole).value<QtMsgType>()))
		return false;
	auto category_index = type_index.siblingAtColumn(static_cast<int>(MessageHandler::Columns::Category));
	auto message_index = type_index.siblingAtColumn(static_cast<int>(MessageHandler::Columns::Message));
	return category_index.data().toString().contains(_message_filter)
		|| message_index.data().toString().contains(_message_filter);
}

LogDock::LogDock(QWidget *parent, Qt::WindowFlags flags):
	QDockWidget(parent, flags),
	_ui(std::make_unique<Ui::LogDock>())
{
	_ui->setupUi(this);
	_filter.setSourceModel(&MessageHandler::instance());
	_ui->message_view->setModel(&_filter);

	for (auto [button, type]: {
			std::pair{_ui->filter_errors, QtCriticalMsg},
			std::pair{_ui->filter_warnings, QtWarningMsg},
			std::pair{_ui->filter_info, QtInfoMsg},
			std::pair{_ui->filter_debug, QtDebugMsg}}) {
		connect(button, &QAbstractButton::toggled, this, [this, type](bool toggled) {
			_filter.setTypeEnabled(type, toggled);
		});
		button->setChecked(_filter.typeEnabled(type));
	}
#ifndef QT_DEBUG
	_ui->filter_debug->setVisible(false);
	_ui->message_view->header()->setSectionHidden(static_cast<int>(MessageHandler::Columns::Location), true);
	_ui->message_view->header()->setSectionHidden(static_cast<int>(MessageHandler::Columns::Function), true);
#endif
	connect(_ui->message_filter, &QLineEdit::textChanged, this, [this](const QString &text) {
		_filter.setMessageFilter(text);
	});

	connect(&MessageHandler::instance(), &QAbstractItemModel::rowsInserted,
		this, &LogDock::onNewMessages);
}

LogDock::~LogDock()
{
}

void LogDock::onNewMessages(const QModelIndex &parent, int first, int last)
{
	if (!isVisible()) {
		auto &message_handler = MessageHandler::instance();
		for (int i = first; i <= last; ++i) {
			auto index = message_handler.index(
					i,
					static_cast<int>(MessageHandler::Columns::Type),
					parent);
			if (index.data(Qt::UserRole).value<QtMsgType>() == QtCriticalMsg) {
				setVisible(true);
				break;
			}
		}
	}
	_ui->message_view->scrollToBottom();
}
