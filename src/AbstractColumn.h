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

#ifndef ABSTRACT_COLUMN_H
#define ABSTRACT_COLUMN_H

#include <QObject>

class QMenu;
class Unit;

class AbstractColumn: public QObject
{
	Q_OBJECT
public:
	AbstractColumn(QObject *parent = nullptr);
	~AbstractColumn() override;

	virtual int count() const;
	virtual QVariant headerData(int section, int role = Qt::DisplayRole) const = 0;
	virtual QVariant unitData(int section, const Unit &unit, int role = Qt::DisplayRole) const = 0;
	virtual bool setUnitData(int section, Unit &unit, const QVariant &value, int role = Qt::EditRole);
	virtual Qt::ItemFlags flags(int section, const Unit &unit) const;

	virtual void makeUnitMenu(int section, const Unit &unit, QMenu *menu, QWidget *parent);
	virtual void makeHeaderMenu(int section, QMenu *menu, QWidget *parent);

	int begin_column, end_column;

signals:
	void unitDataChanged(int first, int last, int unit_id);
	void columnDataChanged(int first, int last);
	void columnsAboutToBeRemoved(int first, int last);
	void columnsRemoved(int first, int last);
	void columnsAboutToBeInserted(int first, int last);
	void columnsInserted(int first, int last);
	void columnsAboutToBeReset();
	void columnsReset();

private:
	int _offset;
};

#endif
