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

#ifndef GRID_VIEW_H
#define GRID_VIEW_H

#include <QTreeView>

class GridView: public QTreeView
{
	Q_OBJECT
public:
	GridView(QWidget *parent = nullptr);
	~GridView() override;

	void setModel(QAbstractItemModel *model) override;

signals:
	void contextMenuRequestedForHeader(int section, const QPoint &pos);
	void contextMenuRequestedForCell(QModelIndex, const QPoint &pos);

private:
	std::unique_ptr<QStyle> _style;
	std::unique_ptr<QAbstractItemDelegate> _delegate;
};

#endif
