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

#include "MessageHandler.h"

#include <iostream>

#include <QFileInfo>
#include <QString>
#include <QVariant>
#include <QBrush>

MessageHandler::MessageHandler()
{
}

MessageHandler::~MessageHandler()
{
}

void MessageHandler::init()
{
#ifdef QT_MESSAGELOGCONTEXT
	qSetMessagePattern("%{time yyyy-MM-ddTHH:mm:ss.zzz}\t%{type}\t%{category}\t%{message} (in %{function}, %{file}:%{line})");
#else
	qSetMessagePattern("%{time yyyy-MM-ddTHH:mm:ss.zzz}\t%{type}\t%{category}\t%{message}%{if-fatal}\n%{backtrace}%{endif}");
#endif
	qInstallMessageHandler(MessageHandler::handler);
}

void MessageHandler::setLogFile(const QString &filename)
{
	_output = std::ofstream(filename.toLocal8Bit());
}

MessageHandler &MessageHandler::instance()
{
	static MessageHandler message_handler;
	return message_handler;
}

void MessageHandler::handler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	instance().handleMessage(type, context, msg);
}

struct MessageHandler::Message
{
	QDateTime time;
	QtMsgType type;
	QString category;
	QString message;
	QString location;
	QString function;
};

void MessageHandler::handleMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	auto short_filename = QFileInfo(context.file).fileName();
	auto formatted = qFormatLogMessage(type, {short_filename.toLocal8Bit(), context.line, context.function, context.category}, msg).toLocal8Bit();
	formatted += "\n";
	if (_output)
		_output.write(formatted.data(), formatted.size());
	// TODO: Windows use OutputDebugStringA
	std::cerr.write(formatted.data(), formatted.size());
	if (type != QtFatalMsg) {
		beginInsertRows({}, _messages.size(), _messages.size());
		_messages.emplace_back(
			QDateTime::currentDateTime(),
			type,
			context.category,
			msg,
			QString("%1:%2").arg(short_filename).arg(context.line),
			context.function);
		endInsertRows();
	}
}

int MessageHandler::rowCount(const QModelIndex &) const
{
	return _messages.size();
}

int MessageHandler::columnCount(const QModelIndex &) const
{
	return static_cast<int>(Columns::Count);
}

QVariant MessageHandler::data(const QModelIndex &index, int role) const
{
	auto column = static_cast<Columns>(index.column());
	const auto &msg = _messages.at(index.row());
	switch (role) {
	case Qt::DisplayRole:
	case Qt::ToolTipRole:
		switch (column) {
		case Columns::Time:
			return msg.time;
		case Columns::Type:
			switch (msg.type) {
			case QtFatalMsg:
				return {};
			case QtCriticalMsg:
				return tr("Error");
			case QtWarningMsg:
				return tr("Warning");
			case QtInfoMsg:
				return tr("Info");
			case QtDebugMsg:
				return tr("Debug");
			}
		case Columns::Category:
			return msg.category;
		case Columns::Message:
			return msg.message;
		case Columns::Location:
			return msg.location;
		case Columns::Function:
			return msg.function;
		default:
			return {};
		}
	case Qt::BackgroundRole:
		switch (msg.type) {
		case QtFatalMsg:
			return {};
		case QtCriticalMsg:
			return QBrush(QColor(255, 0, 0, 64));
		case QtWarningMsg:
			return QBrush(QColor(255, 128, 0, 64));
		case QtInfoMsg:
		case QtDebugMsg:
			return {};
		}
	case Qt::UserRole:
		return msg.type;
	default:
		return {};
	}
}

QVariant MessageHandler::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
		return {};
	switch (static_cast<Columns>(section)) {
	case Columns::Time: return tr("Time");
	case Columns::Type: return tr("Type");
	case Columns::Category: return tr("Category");
	case Columns::Message: return tr("Message");
	case Columns::Location: return tr("Location");
	case Columns::Function: return tr("Function");
	default: return {};
	}
}
