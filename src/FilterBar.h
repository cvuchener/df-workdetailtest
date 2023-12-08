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

#ifndef FILTER_BAR_H
#define FILTER_BAR_H

#include <QToolBar>

enum class FilterType {
	Simple,
	Regex,
	Script,
};

class UnitFilterList;

class FilterBar: public QToolBar
{
	Q_OBJECT
public:
	FilterBar(QWidget *parent);
	~FilterBar() override;

	void setFilterModel(UnitFilterList *model);

signals:
	void filterChanged(FilterType type, const QString &text);

private:
	void insertFilterButtons(int first, int last);
	void filterInserted(const QModelIndex &parent, int first, int last);
	void filterRemoved(const QModelIndex &parent, int first, int last);

	void updateFilterUi();

	struct Ui;
	std::unique_ptr<Ui> _ui;
	UnitFilterList *_filters;
};

#endif
