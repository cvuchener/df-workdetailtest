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

#ifndef FILTER_VIEW_H
#define FILTER_VIEW_H

#include <QWidget>

class QAbstractItemModel;

class FilterView: public QWidget
{
	Q_OBJECT
public:
	FilterView(QWidget *parent = nullptr);
	~FilterView() override;

	void setModel(QAbstractItemModel *model);

private slots:
	void addItems(const QModelIndex &parent, int first, int last);
	void removeItems(const QModelIndex &parent, int first, int last);

private:
	void addButton(int i);

	QAbstractItemModel *_model;
	struct Ui;
	std::unique_ptr<Ui> _ui;
};

#endif
