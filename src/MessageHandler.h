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

#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include <QAbstractTableModel>
#include <QtLogging>
#include <fstream>

class MessageHandler: public QAbstractTableModel
{
public:
	MessageHandler();
	~MessageHandler();

	void setLogFile(const QString &filename);

	static void init();
	static MessageHandler &instance();

	// Model
	enum class Columns {
		Time,
		Type,
		Category,
		Message,
		Location,
		Function,
		Count
	};
	int rowCount(const QModelIndex &) const override;
	int columnCount(const QModelIndex &) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
	static void handler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
	void handleMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg);

	struct Message;
	std::vector<Message> _messages;
	std::ofstream _output;
};

#endif
