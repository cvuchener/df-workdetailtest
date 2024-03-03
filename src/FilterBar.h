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

class UserUnitFilters;

class FilterBar: public QToolBar
{
	Q_OBJECT
public:
	FilterBar(QWidget *parent);
	~FilterBar() override;

	void setFilters(std::shared_ptr<UserUnitFilters> filters);
	const std::shared_ptr<UserUnitFilters> &filters() const { return _filters; }

private:
	void insertFilterButtons(int first, int last);
	void filterInserted(const QModelIndex &parent, int first, int last);
	void filterRemoved(const QModelIndex &parent, int first, int last);

	void setupFilters();
	void updateAutoFilters();
	void updateFilterUi();
	void updateTemporaryFilter();
	void filterEditChanged();
	void completionActivated(const QString &text);
	void completionHighlighted(const QModelIndex &index);

	struct Ui;
	std::unique_ptr<Ui> _ui;
	std::shared_ptr<UserUnitFilters> _filters;
	QMetaObject::Connection _inserted_signal, _removed_signal, _auto_filter_signal;
};

#endif
