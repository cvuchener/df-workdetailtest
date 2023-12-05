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

#include "UnitFilterProxyModel.h"

#include "Application.h"
#include "ScriptManager.h"
#include "Unit.h"
#include "ObjectList.h"
#include "UnitScriptWrapper.h"

UnitFilterList::UnitFilterList(UnitFilterProxyModel &parent):
	_parent(parent)
{
}

UnitFilterList::~UnitFilterList()
{
}

int UnitFilterList::rowCount(const QModelIndex &parent) const
{
	return _filters.size();
}

QVariant UnitFilterList::data(const QModelIndex &index, int role) const
{
	switch (role) {
	case Qt::DisplayRole:
		return _filters[index.row()].first;
	default:
		return {};
	}
}

bool UnitFilterList::removeRows(int row, int count, const QModelIndex &parent)
{
	beginRemoveRows(parent, row, row+count-1);
	_filters.erase(next(_filters.begin(), row), next(_filters.begin(), row+count));
	endRemoveRows();
	_parent.invalidateRowsFilter();
	return true;
}

void UnitFilterList::addFilter(const QString &name, UnitFilter &&filter)
{
	beginInsertRows({}, _filters.size(), _filters.size());
	_filters.emplace_back(name, std::move(filter));
	endInsertRows();
	_parent.invalidateRowsFilter();
}

void UnitFilterList::clear()
{
	beginRemoveRows({}, 0, _filters.size()-1);
	_filters.clear();
	endRemoveRows();
	_parent.invalidateRowsFilter();
}

UnitFilterProxyModel::UnitFilterProxyModel(QObject *parent):
	QSortFilterProxyModel(parent),
	_units(nullptr),
	_filter_list(*this)
{
}

UnitFilterProxyModel::~UnitFilterProxyModel()
{
}

void UnitFilterProxyModel::setSourceModel(QAbstractItemModel *source_model)
{
	_units = dynamic_cast<ObjectList<Unit> *>(source_model);
	QSortFilterProxyModel::setSourceModel(_units);
}

const Unit *UnitFilterProxyModel::get(int row) const
{
	auto source = mapToSource(index(row, 0));
	return _units->get(source.row());
}

Unit *UnitFilterProxyModel::get(int row)
{
	auto source = mapToSource(index(row, 0));
	return _units->get(source.row());
}

QModelIndex UnitFilterProxyModel::find(int unit_id) const
{
	return mapFromSource(_units->find(unit_id));
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

static bool applyFilter(const UnitFilter &filter, const Unit &unit)
{
	return visit(overloaded{
		[&unit](const std::function<bool(const Unit &unit)> &filter) {
			return !filter || filter(unit);
		},
		[unit = Application::scripts().makeUnit(unit)](const QJSValue &filter) {
			auto result = filter.call({unit});
			if (result.isError()) {
				qCCritical(ScriptLog) << "Filter script failed:" << result.property("message").toString();
				return false;
			}
			return result.toBool();
		}}, filter);
}

bool UnitFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	if (source_parent.isValid())
		return true;
	if (auto unit = _units->get(source_row)) {
		for (const auto &[name, filter]: _filter_list._filters)
			if (!applyFilter(filter, *unit))
				return false;
		return applyFilter(_base_filter, *unit)
			&& applyFilter(_temporary_filter, *unit);
	}
	else
		return true;
}
