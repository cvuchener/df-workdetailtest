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

class GroupBar;
class FilterBar;
class DwarfFortress;

class GridViewTabs: public QTabWidget
{
	Q_OBJECT
public:
	GridViewTabs(QWidget *parent = nullptr);
	~GridViewTabs() override;

	void setGroupBar(GroupBar *group_bar);
	void setFilterBar(FilterBar *filter_bar);
	void init(DwarfFortress *df);

	void addView(const QString &name);

signals:
	void currentUnitChanged(const QModelIndex &);

protected:
	void tabRemoved(int index) override;

private:
	QPointer<GroupBar> _group_bar;
	QPointer<FilterBar> _filter_bar;
	QPointer<DwarfFortress> _df;
};

#endif
