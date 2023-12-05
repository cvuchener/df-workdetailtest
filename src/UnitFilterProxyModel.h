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
#include <QAbstractListModel>
#include <QJSValue>

class Unit;
template <typename T>
class ObjectList;

using UnitFilter = std::variant<std::function<bool(const Unit &)>, QJSValue>;

struct AllUnits
{
	bool operator()(const Unit &) const noexcept { return true; }
};

class UnitFilterProxyModel;

class UnitFilterList: public QAbstractListModel
{
	Q_OBJECT
public:
	int rowCount(const QModelIndex &parent) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	bool removeRows(int row, int count, const QModelIndex &parent = {}) override;

	template <std::predicate<const Unit &> Filter>
	void addFilter(const QString &name, Filter &&filter) {
		addFilter(name, UnitFilter(
				std::in_place_type<std::function<bool(const Unit &)>>,
				std::forward<Filter>(filter)));
	}
	void addFilter(const QString &name, QJSValue filter) {
		addFilter(name, UnitFilter(filter));
	}
	void addFilter(const QString &name, UnitFilter &&filter);
	void clear();
private:
	UnitFilterList(UnitFilterProxyModel &parent);
	~UnitFilterList() override;

	UnitFilterProxyModel &_parent;
	std::vector<std::pair<QString, UnitFilter>> _filters;
	friend class UnitFilterProxyModel;
};

class UnitFilterProxyModel: public QSortFilterProxyModel
{
	Q_OBJECT
public:
	UnitFilterProxyModel(QObject *parent = nullptr);
	~UnitFilterProxyModel() override;

	template <std::predicate<const Unit &> Filter>
	void setBaseFilter(Filter &&filter)
	{
		_base_filter.emplace<std::function<bool(const Unit &)>>(std::forward<Filter>(filter));
		invalidateRowsFilter();
	}

	template <std::predicate<const Unit &> Filter>
	void setTemporaryFilter(Filter &&filter)
	{
		_temporary_filter.emplace<std::function<bool(const Unit &)>>(std::forward<Filter>(filter));
		invalidateRowsFilter();
	}

	UnitFilterList &filterList() { return _filter_list; }

	const Unit *get(int row) const;
	Unit *get(int row);

	QModelIndex find(int unit_id) const;

	void setSourceModel(QAbstractItemModel *source_model) override;

protected:
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
	ObjectList<Unit> *_units;
	UnitFilter _base_filter;
	UnitFilterList _filter_list;
	UnitFilter _temporary_filter;

	friend class UnitFilterList;
};

#endif
