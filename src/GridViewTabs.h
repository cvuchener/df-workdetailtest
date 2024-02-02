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

#ifndef GRID_VIEW_TABS_H
#define GRID_VIEW_TABS_H

#include <QTabWidget>
#include <QPointer>
#include <QItemSelectionModel>

class GroupBar;
class FilterBar;
class DwarfFortress;
class GridView;

class GridViewTabs: public QTabWidget
{
	Q_OBJECT
public:
	GridViewTabs(QWidget *parent = nullptr);
	~GridViewTabs() override;

	void init(GroupBar *group_bar, FilterBar *filter_bar, DwarfFortress *df);

	void addView(const QString &name);

signals:
	void currentUnitChanged(const QModelIndex &);

protected:
	void tabRemoved(int index) override;

private:
	void onCurrentTabChanged(int index);
	void onCurrentUnitChanged(GridView *view, const QModelIndex &current, const QModelIndex &prev);
	void onSelectionChanged(GridView *view, const QItemSelection &selected, const QItemSelection &deselected);

	QPointer<GroupBar> _group_bar;
	QPointer<FilterBar> _filter_bar;
	QPointer<DwarfFortress> _df;

	// For syncing unit selection across tabs, indexes from _df->units
	QPersistentModelIndex _current_unit;
	QItemSelectionModel _unit_selection;
	QWidget *_controlling_view = nullptr;
};

#endif
