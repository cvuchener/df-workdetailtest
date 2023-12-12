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
	std::ranges::fill(_enabled, true);
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
	auto message_index = type_index.siblingAtColumn(static_cast<int>(MessageHandler::Columns::Category));
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

	connect(_ui->filter_errors, &QAbstractButton::toggled, this, [this](bool toggled) {
		_filter.setTypeEnabled(QtCriticalMsg, toggled);
	});
	_ui->filter_errors->setChecked(_filter.typeEnabled(QtCriticalMsg));
	connect(_ui->filter_warnings, &QAbstractButton::toggled, this, [this](bool toggled) {
		_filter.setTypeEnabled(QtWarningMsg, toggled);
	});
	_ui->filter_warnings->setChecked(_filter.typeEnabled(QtWarningMsg));
	connect(_ui->filter_info, &QAbstractButton::toggled, this, [this](bool toggled) {
		_filter.setTypeEnabled(QtInfoMsg, toggled);
	});
	_ui->filter_info->setChecked(_filter.typeEnabled(QtInfoMsg));
	connect(_ui->filter_debug, &QAbstractButton::toggled, this, [this](bool toggled) {
		_filter.setTypeEnabled(QtDebugMsg, toggled);
	});
	_ui->filter_debug->setChecked(_filter.typeEnabled(QtDebugMsg));
#ifndef QT_DEBUG
	_ui->filter_debug->setVisible(false);
	_filter.setTypeEnabled(QtDebugMsg, false);
	_ui->message_view->header()->setSectionHidden(static_cast<int>(MessageHandler::Columns::Location), true);
	_ui->message_view->header()->setSectionHidden(static_cast<int>(MessageHandler::Columns::Function), true);
#endif
	connect(_ui->message_filter, &QLineEdit::textChanged, this, [this](const QString &text) {
		_filter.setMessageFilter(text);
	});

	connect(&MessageHandler::instance(), &QAbstractItemModel::rowsInserted, this, [this]() {
		_ui->message_view->scrollToBottom();
	});
}

LogDock::~LogDock()
{
}
