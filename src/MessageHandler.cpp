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

MessageHandler::MessageHandler()
{
}

MessageHandler::~MessageHandler()
{
}

void MessageHandler::init()
{
#ifdef QT_MESSAGELOGCONTEXT
	qSetMessagePattern("%{time yyyy-MM-ddTHH:mm:ss.zzz}\t%{type}\t%{category}\t%{message} (in %{function}, %{file}:%{line})%{if-fatal}\n%{backtrace}%{endif}");
#else
	qSetMessagePattern("%{time yyyy-MM-ddTHH:mm:ss.zzz}\t%{type}\t%{category}\t%{message}%{if-fatal}\n%{backtrace}%{endif}");
#endif
	qInstallMessageHandler(MessageHandler::handler);
}

void MessageHandler::setLogFile(const QString &filename)
{
	output = std::ofstream(filename.toLocal8Bit());
}

MessageHandler &MessageHandler::instance()
{
	static MessageHandler message_handler;
	return message_handler;
}

void MessageHandler::handler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	auto &self = instance();
	auto formatted = qFormatLogMessage(type, {QFileInfo(context.file).fileName().toLocal8Bit(), context.line, context.function, context.category}, msg).toLocal8Bit();
	formatted += "\n";
	if (self.output)
		self.output.write(formatted.data(), formatted.size());
	// TODO: Windows use OutputDebugStringA
	std::cerr.write(formatted.data(), formatted.size());
}
