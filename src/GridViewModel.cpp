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

#include "GridViewModel.h"

#include "DwarfFortress.h"
#include "AbstractColumn.h"
#include "UnitFilterProxyModel.h"
#include "Unit.h"
#include "ObjectList.h"
#include "NameColumn.h"
#include "WorkDetailColumn.h"
#include "SpecialistColumn.h"
#include "GroupByCreature.h"

GridViewModel::GridViewModel(DwarfFortress &df, QObject *parent):
	QAbstractItemModel(parent),
	_df(df),
	_unit_filter(std::make_unique<UnitFilterProxyModel>())
{
	_columns.push_back(std::make_unique<NameColumn>());
	_columns.push_back(std::make_unique<WorkDetailColumn>(df));
	_columns.push_back(std::make_unique<SpecialistColumn>(df));

	_group_by = std::make_unique<GroupByCreature>(_df);

	_unit_filter->setUnitFilter(&Unit::isFortControlled);
	_unit_filter->setSourceModel(&_df.units());
	connect(_unit_filter.get(), &QAbstractItemModel::modelAboutToBeReset,
		this, &GridViewModel::unitBeginReset);
	connect(_unit_filter.get(), &QAbstractItemModel::modelReset,
		this, &GridViewModel::unitEndReset);
	connect(_unit_filter.get(), &QAbstractItemModel::dataChanged,
		this, &GridViewModel::unitDataChanged);
	connect(_unit_filter.get(), &QAbstractItemModel::rowsAboutToBeInserted,
		this, &GridViewModel::unitBeginInsert);
	connect(_unit_filter.get(), &QAbstractItemModel::rowsInserted,
		this, &GridViewModel::unitEndInsert);
	connect(_unit_filter.get(), &QAbstractItemModel::rowsAboutToBeRemoved,
		this, &GridViewModel::unitBeginRemove);
	connect(_unit_filter.get(), &QAbstractItemModel::rowsRemoved,
		this, &GridViewModel::unitEndRemove);

	int count = 0;
	for (auto &col: _columns) {
		col->begin_column = count;
		col->end_column = (count += col->count());
		connect(col.get(), &AbstractColumn::columnsAboutToBeReset,
			this, &GridViewModel::columnBeginReset);
		connect(col.get(), &AbstractColumn::columnsReset,
			this, &GridViewModel::columnEndReset);
		connect(col.get(), &AbstractColumn::unitDataChanged,
			this, &GridViewModel::cellDataChanged);
		connect(col.get(), &AbstractColumn::columnDataChanged,
			this, &GridViewModel::columnDataChanged);
		connect(col.get(), &AbstractColumn::columnsAboutToBeInserted,
			this, &GridViewModel::columnBeginInsert);
		connect(col.get(), &AbstractColumn::columnsInserted,
			this, &GridViewModel::columnEndInsert);
		connect(col.get(), &AbstractColumn::columnsAboutToBeRemoved,
			this, &GridViewModel::columnBeginRemove);
		connect(col.get(), &AbstractColumn::columnsRemoved,
			this, &GridViewModel::columnEndRemove);
	}
}

GridViewModel::~GridViewModel()
{
}

/*
 * QModelIndex internal id is used for the parent row. Top-level index has
 * NoParent as internal id.
 *
 * With no grouping, all QModelIndex have NoParent internal id. Rows are the
 * same as the source unit model.
 *
 * With grouping, group QModelIndex have NoParent internal id and rows are
 * index in _groups. Leaf QModelIndex have the group index/row as internal id,
 * rows are index in group_t::units.
 */

static constexpr quintptr NoParent = static_cast<quintptr>(-1);

QModelIndex GridViewModel::index(int row, int column, const QModelIndex &parent) const
{
	if (parent.isValid())
		return createIndex(row, column, parent.row());
	else
		return createIndex(row, column, NoParent);
}

QModelIndex GridViewModel::parent(const QModelIndex &index) const
{
	if (index.internalId() == NoParent)
		return {};
	else
		return createIndex(index.internalId(), 0, NoParent);
}

QModelIndex GridViewModel::sibling(int row, int column, const QModelIndex &index) const
{
	return createIndex(row, column, index.internalId());
}

int GridViewModel::rowCount(const QModelIndex &parent) const
{
	if (_group_by) {
		if (parent.isValid()) {
			if (parent.internalId() == NoParent) // group
				return _groups[parent.row()].units.size();
			else // unit
				return 0;
		}
		else // root
			return _groups.size();
	}
	else {
		if (parent.isValid()) // unit
			return 0;
		else // root
			return _unit_filter->rowCount();
	}
}

int GridViewModel::columnCount(const QModelIndex &parent) const
{
	return _columns.back()->end_column;
}

QVariant GridViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal)
		return {};
	auto [col, s] = getColumn(section);
	return col->headerData(s, role);
}

// Check if index refers to a group or unit and call unit_action or
// group_action accordingly passing the unit/group, column and args.
template <typename Model, typename UnitAction, typename GroupAction, typename... Args>
auto GridViewModel::applyToIndex(Model &&model, const QModelIndex &index, UnitAction &&unit_action, GroupAction &&group_action, Args &&...args)
{
	Q_ASSERT(index.isValid());
	if (model._group_by) {
		if (index.internalId() == NoParent) {
			return group_action(model._groups[index.row()], index.column(), std::forward<Args>(args)...);
		}
		else {
			auto unit = model._groups[index.internalId()].units[index.row()];
			Q_ASSERT(unit);
			return unit_action(*unit, index.column(), std::forward<Args>(args)...);
		}
	}
	else {
		auto unit = model._unit_filter->get(index.row());
		Q_ASSERT(unit);
		return unit_action(*unit, index.column(), std::forward<Args>(args)...);
	}
}

static std::span<const Unit *> as_const_span(const std::vector<Unit *> &units)
{
	return std::span<const Unit *>((const Unit **)units.data(), units.size());
}

QVariant GridViewModel::data(const QModelIndex &index, int role) const
{
	return applyToIndex(*this, index,
		[this, role](const Unit &unit, int column) -> QVariant {
			auto [col, section] = getColumn(column);
			return col->unitData(section, unit, role);
		},
		[this, role](const group_t &group, int column) -> QVariant {
			auto [col, section] = getColumn(column);
			return col->groupData(section, _group_by->groupName(group.id), as_const_span(group.units), role);
		});
}

bool GridViewModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	return applyToIndex(*this, index,
		[this, &value, role](Unit &unit, int column) -> bool {
			auto [col, section] = getColumn(column);
			return col->setUnitData(section, unit, value, role);
		},
		[this, &value, role](group_t &group, int column) -> bool {
			auto [col, section] = getColumn(column);
			return col->setGroupData(section, group.units, value, role);
		});
}

Qt::ItemFlags GridViewModel::flags(const QModelIndex &index) const
{
	return applyToIndex(*this, index,
		[this](const Unit &unit, int column) -> Qt::ItemFlags {
			auto [col, section] = getColumn(column);
			return col->unitFlags(section, unit) | Qt::ItemNeverHasChildren;
		},
		[this](const group_t &group, int column) -> Qt::ItemFlags {
			auto [col, section] = getColumn(column);
			return col->groupFlags(section, as_const_span(group.units));
		});
}

QModelIndex GridViewModel::sourceUnitIndex(const QModelIndex &index) const
{
	if (_group_by) {
		if (index.internalId() == NoParent)
			return {};
		else {
			auto unit = _groups[index.internalId()].units[index.row()];
			return _df.units().find(*unit);
		}
	}
	else
		return _unit_filter->mapToSource(_unit_filter->index(index.row(), 0));
}

void GridViewModel::makeColumnMenu(int section, QMenu *menu, QWidget *parent)
{
	auto [col, idx] = getColumn(section);
	col->makeHeaderMenu(idx, menu, parent);
}

void GridViewModel::makeCellMenu(const QModelIndex &index, QMenu *menu, QWidget *parent)
{
	applyToIndex(*this, index,
		[this, menu, parent](Unit &unit, int column) {
			auto [col, sec] = getColumn(column);
			col->makeUnitMenu(sec, unit, menu, parent);
		},
		[](group_t &group, int column) {
		});
}

void GridViewModel::toggleCells(const QModelIndexList &indexes)
{
	// Sort indexes by column and type
	struct targets_t {
		std::vector<group_t *> groups;
		std::vector<Unit *> units;
	};
	std::vector<targets_t> targets(columnCount());
	for (auto index: indexes) {
		applyToIndex(*this, index,
			[&](Unit &unit, int column) {
				targets[column].units.push_back(&unit);
			},
			[&](group_t &group, int column) {
				targets[column].groups.push_back(&group);
			});
	}
	for (std::size_t i = 0; i < targets.size(); ++i) {
		auto &[groups, units] = targets[i];
		auto [col, sec] = getColumn(i);
		if (units.empty()) {
			// Only toggle groups if no units are selected
			for (auto g: groups) {
				auto state = col->groupData(sec,
						_group_by->groupName(g->id),
						as_const_span(g->units),
						Qt::CheckStateRole).value<Qt::CheckState>();
				col->setGroupData(sec,
						g->units,
						state == Qt::Checked
							? Qt::Unchecked
							: Qt::Checked,
						Qt::CheckStateRole);
			}
		}
		else
			col->toggleUnits(sec, units);
	}
}

void GridViewModel::cellDataChanged(int first, int last, int unit_id)
{
	auto col = qobject_cast<AbstractColumn *>(sender());
	Q_ASSERT(col);
	auto index = unitIndex(unit_id);
	Q_ASSERT(index.isValid());
	dataChanged(
		index.siblingAtColumn(col->begin_column+first),
		index.siblingAtColumn(col->begin_column+last));
	if (_group_by) {
		auto group_index = createIndex(index.internalId(), 0, NoParent);
		dataChanged(
			group_index.siblingAtColumn(col->begin_column+first),
			group_index.siblingAtColumn(col->begin_column+last));
	}
}

void GridViewModel::unitDataChanged(const QModelIndex &first, const QModelIndex &last, const QList<int> &roles)
{
	if (_group_by) {
		for (int unit_index = first.row(); unit_index <= last.row(); ++unit_index) {
			auto &unit = *_unit_filter->get(unit_index);
			auto current_index = unitIndex(unit->id);
			auto new_group_id = _group_by->unitGroup(unit);
			if (_groups[current_index.internalId()].id != new_group_id) {
				removeFromGroup(current_index);
				addUnitToGroup(unit, new_group_id);
			}
			else {
				dataChanged(
					current_index.siblingAtColumn(0),
					current_index.siblingAtColumn(columnCount()-1));
				auto group_index = createIndex(current_index.internalId(), 0, NoParent);
				dataChanged(
					group_index.siblingAtColumn(0),
					group_index.siblingAtColumn(columnCount()-1));
			}
		}
	}
	else {
		dataChanged(
			index(first.row(), 0),
			index(last.row(), columnCount()-1));
	}
}

void GridViewModel::unitBeginReset()
{
	beginResetModel();
}

void GridViewModel::unitEndReset()
{
	rebuildGroups();
	endResetModel();
}

void GridViewModel::unitBeginInsert(const QModelIndex &, int first, int last)
{
	if (!_group_by)
		beginInsertRows({}, first, last);
}

void GridViewModel::unitEndInsert(const QModelIndex &, int first, int last)
{
	if (_group_by) {
		for (int unit_index = first; unit_index <= last; ++unit_index) {
			auto &unit = *_unit_filter->get(unit_index);
			addUnitToGroup(unit, _group_by->unitGroup(unit));
		}
	}
	else
		endInsertRows();
}

void GridViewModel::unitBeginRemove(const QModelIndex &, int first, int last)
{
	if (_group_by) {
		for (int unit_index = first; unit_index <= last; ++unit_index) {
			auto &unit = *_unit_filter->get(unit_index);
			auto index = unitIndex(unit->id);
			removeFromGroup(index);
			_unit_group.erase(unit->id);
		}
	}
	else
		beginRemoveRows({}, first, last);
}

void GridViewModel::unitEndRemove(const QModelIndex &, int first, int last)
{
	if (!_group_by)
		endRemoveRows();
}

QModelIndex GridViewModel::unitIndex(int unit_id) const
{
	if (_group_by) {
		auto it = _unit_group.find(unit_id);
		Q_ASSERT(it != _unit_group.end());
		auto group_it = std::ranges::lower_bound( _groups, it->second, {}, &group_t::id);
		Q_ASSERT(group_it != _groups.end() && group_it->id == it->second);
		auto unit_it = std::ranges::lower_bound(
				group_it->units,
				unit_id,
				{},
				[](Unit *unit) { return (*unit)->id; });
		Q_ASSERT(unit_it != group_it->units.end() && (**unit_it)->id == unit_id);
		return createIndex(
				distance(group_it->units.begin(), unit_it),
				0,
				distance(_groups.begin(), group_it)
		);
	}
	else {
		auto index = _unit_filter->mapFromSource(_df.units().find(unit_id));
		Q_ASSERT(index.isValid());
		return createIndex(index.row(), 0, NoParent);
	}
}

void GridViewModel::addUnitToGroup(Unit &unit, quint64 group_id, bool reseting)
{
	_unit_group[unit->id] = group_id;
	auto new_group_it = std::ranges::lower_bound( _groups, group_id, {}, &group_t::id);
	if (new_group_it == _groups.end() || new_group_it->id != group_id) {
		// Add new group
		if (!reseting) {
			auto row = distance(_groups.begin(), new_group_it);
			beginInsertRows({}, row, row);
		}
		_groups.insert(new_group_it, {group_id, {&unit}});
		if (!reseting)
			endInsertRows();
	}
	else {
		// Add in existing group
		auto insert_pos = std::ranges::lower_bound( new_group_it->units, unit->id, {},
				[](Unit *unit){ return (*unit)->id; });
		auto group_index = createIndex(distance(_groups.begin(), new_group_it), 0, NoParent);
		if (!reseting) {
			auto row = distance(new_group_it->units.begin(), insert_pos);
			beginInsertRows(group_index, row, row);
		}
		new_group_it->units.insert(insert_pos, &unit);
		if (!reseting)
			endInsertRows();
		dataChanged(
			group_index.siblingAtColumn(0),
			group_index.siblingAtColumn(columnCount()-1));
	}
}

void GridViewModel::removeFromGroup(const QModelIndex &index)
{
	auto &group = _groups[index.internalId()];
	if (group.units.size() == 1) {
		// Last unit in the group, remove the whole group
		beginRemoveRows({}, index.internalId(), index.internalId());
		_groups.erase(next(_groups.begin(), index.internalId()));
		endRemoveRows();
	}
	else {
		beginRemoveRows(index.parent(), index.row(), index.row());
		group.units.erase(next(group.units.begin(), index.row()));
		endRemoveRows();
		auto group_index = createIndex(index.internalId(), 0, NoParent);
		dataChanged(
			group_index.siblingAtColumn(0),
			group_index.siblingAtColumn(columnCount()-1));
	}
}

void GridViewModel::rebuildGroups()
{
	_groups.clear();
	_unit_group.clear();
	if (_group_by)
		for (int unit_index = 0; unit_index < _unit_filter->rowCount(); ++unit_index) {
			auto &unit = *_unit_filter->get(unit_index);
			addUnitToGroup(unit, _group_by->unitGroup(unit), true);
		}
}

void GridViewModel::columnDataChanged(int first, int last)
{
	auto col = qobject_cast<AbstractColumn *>(sender());
	Q_ASSERT(col);
	headerDataChanged(Qt::Horizontal, col->begin_column+first, col->begin_column+last);
	dataChanged(
		index(0, col->begin_column+first),
		index(rowCount(), col->begin_column+last)
	);
}

void GridViewModel::columnBeginInsert(int first, int last)
{
	auto col = qobject_cast<AbstractColumn *>(sender());
	Q_ASSERT(col);
	layoutAboutToBeChanged();
	beginInsertColumns({}, col->begin_column+first, col->begin_column+last);
	int count = last-first+1;
	auto it = _columns.rbegin();
	while (it->get() != col) {
		(*it)->begin_column += count;
		(*it)->end_column += count;
		++it;
	}
	col->end_column += count;
}

void GridViewModel::columnEndInsert(int first, int last)
{
	endInsertColumns();
	layoutChanged();
}

void GridViewModel::columnBeginRemove(int first, int last)
{
	auto col = qobject_cast<AbstractColumn *>(sender());
	Q_ASSERT(col);
	if (_group_by)
		layoutAboutToBeChanged();
	beginRemoveColumns({}, col->begin_column+first, col->begin_column+last);
	int count = last-first+1;
	auto it = _columns.rbegin();
	while (it->get() != col) {
		(*it)->begin_column -= count;
		(*it)->end_column -= count;
		++it;
	}
	col->end_column -= count;
}

void GridViewModel::columnEndRemove(int first, int last)
{
	endRemoveColumns();
	if (_group_by)
		layoutChanged();
}

void GridViewModel::columnBeginReset()
{
	beginResetModel();
}

void GridViewModel::columnEndReset()
{
	auto col = qobject_cast<AbstractColumn *>(sender());
	Q_ASSERT(col);
	col->end_column = col->begin_column + col->count();
	auto it = std::ranges::find(_columns, col, &std::unique_ptr<AbstractColumn>::get);
	Q_ASSERT(it != _columns.end());
	for (++it; it != _columns.end(); ++it) {
		(*it)->begin_column = (*prev(it))->end_column;
		(*it)->end_column = (*it)->begin_column + (*it)->count();
	}
	endResetModel();
}

std::pair<const AbstractColumn *, int> GridViewModel::getColumn(int col) const
{
	auto it = std::ranges::upper_bound(_columns, col, {}, [](const auto &col){return col->end_column;});
	if (it == _columns.end())
		return {nullptr, -1};
	else
		return { it->get(), col - (*it)->begin_column };
}

std::pair<AbstractColumn *, int> GridViewModel::getColumn(int col)
{
	auto it = std::ranges::upper_bound(_columns, col, {}, [](const auto &col){return col->end_column;});
	if (it == _columns.end())
		return {nullptr, -1};
	else
		return { it->get(), col - (*it)->begin_column };
}
