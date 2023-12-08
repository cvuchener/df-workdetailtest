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

#ifndef UNIT_FILTER_PROXY_MODEL_H
#define UNIT_FILTER_PROXY_MODEL_H

#include <QSortFilterProxyModel>

#include "UserUnitFilters.h"

template <typename T>
class ObjectList;

class UnitFilterProxyModel: public QSortFilterProxyModel
{
	Q_OBJECT
public:
	UnitFilterProxyModel(QObject *parent = nullptr);
	~UnitFilterProxyModel() override;

	template <std::predicate<const Unit &> Filter>
	void setBaseFilter(Filter &&filter)
	{
		_base_filter = UnitFilter(std::forward<Filter>(filter));
		invalidateRowsFilter();
	}

	const std::shared_ptr<const UserUnitFilters> &userFilters() const { return _user_filters; }
	void setUserFilters(std::shared_ptr<const UserUnitFilters> user_filters);

	const Unit *get(int row) const;
	Unit *get(int row);

	QModelIndex find(int unit_id) const;

	void setSourceModel(QAbstractItemModel *source_model) override;

protected:
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
	ObjectList<Unit> *_units;
	UnitFilter _base_filter;
	std::shared_ptr<const UserUnitFilters> _user_filters;
};

#endif
