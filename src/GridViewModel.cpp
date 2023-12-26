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

#include <dfhack-client-qt/Client.h>

#include "DwarfFortressData.h"
#include "AbstractColumn.h"
#include "UnitFilterProxyModel.h"
#include "Unit.h"
#include "ObjectList.h"
#include "Columns/NameColumn.h"
#include "Application.h"
#include "ScriptManager.h"
#include "Groups/Factory.h"

Q_LOGGING_CATEGORY(GridViewLog, "gridview");

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

GridViewModel::Parameters GridViewModel::Parameters::fromJson(const QJsonDocument &doc)
{
	GridViewModel::Parameters params;

	params.title = doc.object().value("title").toString();

	// Parse base filter
	auto filter_string = doc.object().value("filter").toString();
	if (!filter_string.isNull()) {
		auto sep = filter_string.indexOf(':');
		auto type = filter_string.first(sep);
		auto value = filter_string.sliced(sep+1);
		if (type == "builtin") {
			auto filter = std::ranges::find(BuiltinUnitFilters, value, [](const auto &p) { return p.first; });
			if (filter == BuiltinUnitFilters.end())
				qCCritical(GridViewLog) << "Invalid builtin filter:" << value;
			else
				params.filter = filter->second;
		}
		else if (type == "script") {
			auto filter = Application::scripts().makeScript(value);
			if (filter.isError())
				qCCritical(GridViewLog) << "Invalid script filter:" << filter.property("message").toString();
			else
				params.filter = ScriptedUnitFilter{filter};
		}
		else
			qCCritical(GridViewLog) << "Unsupported filter type:" << type;
	}

	// Make column factories
	for (auto json_column: doc.object().value("columns").toArray()) {
		if (!json_column.isObject()) {
			qCCritical(GridViewLog) << "column must be an object";
			continue;
		}
		if (auto factory = Columns::makeFactory(json_column.toObject()))
			params.columns.push_back(std::move(factory));
	}
	return params;
}

GridViewModel::GridViewModel(const Parameters &parameters, std::shared_ptr<DwarfFortressData> df, DFHack::Client &dfhack, QObject *parent):
	QAbstractItemModel(parent),
	_df(std::move(df)),
	_dfhack(&dfhack),
	_group_index(0)
{
	_title = parameters.title;
	_unit_filter.setBaseFilter(parameters.filter);
	_columns.push_back(std::make_unique<Columns::NameColumn>());
	for (const auto &factory: parameters.columns)
		_columns.push_back(factory(*_df, _dfhack));

	_unit_filter.setSourceModel(_df->units.get());
	connect(&_unit_filter, &QAbstractItemModel::dataChanged,
		this, &GridViewModel::unitDataChanged);
	connect(&_unit_filter, &QAbstractItemModel::rowsAboutToBeInserted,
		this, &GridViewModel::unitBeginInsert);
	connect(&_unit_filter, &QAbstractItemModel::rowsInserted,
		this, &GridViewModel::unitEndInsert);
	connect(&_unit_filter, &QAbstractItemModel::rowsAboutToBeRemoved,
		this, &GridViewModel::unitBeginRemove);
	connect(&_unit_filter, &QAbstractItemModel::rowsRemoved,
		this, &GridViewModel::unitEndRemove);

	int count = 0;
	for (auto &col: _columns) {
		col->begin_column = count;
		col->end_column = (count += col->count());
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
		connect(col.get(), &AbstractColumn::columnsAboutToBeMoved,
			this, &GridViewModel::columnBeginMove);
		connect(col.get(), &AbstractColumn::columnsMoved,
			this, &GridViewModel::columnEndMove);
	}
}

GridViewModel::~GridViewModel()
{
}

void GridViewModel::setUserFilters(std::shared_ptr<UserUnitFilters> user_filters)
{
	_user_filters = std::move(user_filters);
	_unit_filter.setUserFilters(_user_filters);
}

/*
 * QModelIndex internal id is used for the group id. Top-level index has
 * NoParent as internal id.
 *
 * With no grouping, all QModelIndex have NoParent internal id. Rows are the
 * same as the source unit model.
 *
 * With grouping, group QModelIndex have NoParent internal id and rows are
 * index in _groups. Leaf QModelIndex have the group id as internal id,
 * rows are index in group_t::units.
 */

static_assert(std::same_as<quintptr, quint64>, "index internal id must be compatible with quint64");

// NoParent should be a value that GroupBy::unitGroup will never return
static constexpr quintptr NoParent = 0x8000'0000'0000'0000ull;

QModelIndex GridViewModel::index(int row, int column, const QModelIndex &parent) const
{
	if (parent.isValid())
		return createIndex(row, column, _groups.at(parent.row()).id);
	else
		return createIndex(row, column, NoParent);
}

QModelIndex GridViewModel::parent(const QModelIndex &index) const
{
	if (index.internalId() == NoParent)
		return {};
	else
		return createIndex(distance(_groups.begin(), findGroup(index.internalId())), 0, NoParent);
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
			return _unit_filter.rowCount();
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
			auto unit = model.findGroup(index.internalId())->units[index.row()];
			Q_ASSERT(unit);
			return unit_action(*unit, index.column(), std::forward<Args>(args)...);
		}
	}
	else {
		auto unit = model._unit_filter.get(index.row());
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
			return col->groupData(section, {_group_by.get(), group.id}, as_const_span(group.units), role);
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

const Unit *GridViewModel::unit(const QModelIndex &index) const
{
	if (!index.isValid())
		return nullptr;
	if (_group_by) {
		if (index.internalId() == NoParent)
			return nullptr;
		else {
			return findGroup(index.internalId())->units[index.row()];
		}
	}
	else
		return _unit_filter.get(index.row());
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
						{_group_by.get(), g->id},
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

void GridViewModel::setGroupBy(int index)
{
	layoutAboutToBeChanged();
	// Keep the unit id for each persistent index (or -1 for groups)
	QModelIndexList old_indexes = persistentIndexList();
	std::vector<int> units;
	units.reserve(old_indexes.size());
	for (auto index: old_indexes)
		units.push_back(applyToIndex(*this, index,
			[](const Unit &unit, int) { return unit->id; },
			[](const group_t &, int) { return -1; }));
	// Change grouping method
	_group_by = Groups::All.at(index).second(*_df);
	_group_index = index;
	rebuildGroups();
	// Rebuild indexes from unit ids
	QModelIndexList new_indexes(old_indexes.size());
	for (int i = 0; i < old_indexes.size(); ++i) {
		if (units[i] != -1)
			new_indexes[i] = unitIndex(units[i]).siblingAtColumn(old_indexes[i].column());
	}
	changePersistentIndexList(old_indexes, new_indexes);
	layoutChanged();
}

void GridViewModel::cellDataChanged(int first, int last, int unit_id)
{
	auto col = qobject_cast<AbstractColumn *>(sender());
	Q_ASSERT(col);
	auto index = unitIndex(unit_id);
	Q_ASSERT(index.isValid());
	if (_group_by) {
		auto unit = _df->units->get(_df->units->find(unit_id).row());
		updateGroupedUnit(*unit, col->begin_column+first, col->begin_column+last);
	}
	else {
		dataChanged(
			index.siblingAtColumn(col->begin_column+first),
			index.siblingAtColumn(col->begin_column+last));
	}
}

void GridViewModel::unitDataChanged(const QModelIndex &first, const QModelIndex &last, const QList<int> &roles)
{
	if (_group_by) {
		for (int unit_index = first.row(); unit_index <= last.row(); ++unit_index)
			updateGroupedUnit(*_unit_filter.get(unit_index), 0, columnCount()-1);
	}
	else {
		dataChanged(
			index(first.row(), 0),
			index(last.row(), columnCount()-1));
	}
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
			auto &unit = *_unit_filter.get(unit_index);
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
			auto &unit = *_unit_filter.get(unit_index);
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

QModelIndex GridViewModel::groupIndex(decltype(_groups)::const_iterator it) const
{
	return createIndex(distance(_groups.begin(), it), 0, NoParent);
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
				group_it->id
		);
	}
	else {
		auto index = _unit_filter.find(unit_id);
		Q_ASSERT(index.isValid());
		return createIndex(index.row(), 0, NoParent);
	}
}

void GridViewModel::addUnitToGroup(Unit &unit, quint64 group_id, bool reseting)
{
	Q_ASSERT(group_id != NoParent);
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
		auto group_index = groupIndex(new_group_it);
		if (!reseting) {
			auto row = distance(new_group_it->units.begin(), insert_pos);
			beginInsertRows(group_index, row, row);
		}
		new_group_it->units.insert(insert_pos, &unit);
		if (!reseting) {
			endInsertRows();
			dataChanged(
				group_index.siblingAtColumn(0),
				group_index.siblingAtColumn(columnCount()-1));
		}
	}
}

void GridViewModel::removeFromGroup(const QModelIndex &index)
{
	auto group = findGroup(index.internalId());
	auto group_index = groupIndex(group);
	if (group->units.size() == 1) {
		// Last unit in the group, remove the whole group
		beginRemoveRows({}, group_index.row(), group_index.row());
		_groups.erase(group);
		endRemoveRows();
	}
	else {
		beginRemoveRows(index.parent(), index.row(), index.row());
		group->units.erase(next(group->units.begin(), index.row()));
		endRemoveRows();
		dataChanged(
			group_index.siblingAtColumn(0),
			group_index.siblingAtColumn(columnCount()-1));
	}
}

void GridViewModel::updateGroupedUnit(Unit &unit, int first_col, int last_col)
{
	auto unit_index = unitIndex(unit->id);
	auto group_index = unit_index.parent();
	auto new_group_id = _group_by->unitGroup(unit);
	Q_ASSERT(new_group_id != NoParent);
	if (new_group_id != _groups[group_index.row()].id) {
		QPersistentModelIndex old_group_index = group_index; // old group index may be modified by the new group insertion
		auto new_group_it = std::ranges::lower_bound( _groups, new_group_id, {}, &group_t::id);
		QModelIndex new_group_index = groupIndex(new_group_it);
		if (new_group_it == _groups.end() || new_group_it->id != new_group_id) {
			// Add new group
			beginInsertRows({}, new_group_index.row(), new_group_index.row());
			new_group_it = _groups.insert(new_group_it, {new_group_id, {}});
			endInsertRows();
		}
		auto old_group_it = next(_groups.begin(), old_group_index.row());

		// Move row
		auto insert_pos = std::ranges::lower_bound(new_group_it->units, unit->id, {},
				[](Unit *unit){ return (*unit)->id; });
		auto insert_row = distance(new_group_it->units.begin(), insert_pos);
		beginMoveRows(old_group_index, unit_index.row(), unit_index.row(), new_group_index, insert_row);
		_unit_group[unit->id] = new_group_id;
		old_group_it->units.erase(next(old_group_it->units.begin(), unit_index.row()));
		new_group_it->units.insert(insert_pos, &unit);
		endMoveRows();

		// Update new group
		dataChanged(
			new_group_index.siblingAtColumn(0),
			new_group_index.siblingAtColumn(columnCount()-1));
		if (old_group_it->units.size() == 0) {
			// Remove empty old group
			beginRemoveRows({}, old_group_index.row(), old_group_index.row());
			_groups.erase(old_group_it);
			endRemoveRows();
		}
		else {
			// Update old group
			dataChanged(
				QModelIndex(old_group_index).siblingAtColumn(0),
				QModelIndex(old_group_index).siblingAtColumn(columnCount()-1));
		}
	}
	else {
		// group has not changed, only update data
		dataChanged(
			unit_index.siblingAtColumn(first_col),
			unit_index.siblingAtColumn(last_col));
		dataChanged(
			group_index.siblingAtColumn(first_col),
			group_index.siblingAtColumn(last_col));
	}
}

void GridViewModel::rebuildGroups()
{
	_groups.clear();
	_unit_group.clear();
	if (_group_by)
		for (int unit_index = 0; unit_index < _unit_filter.rowCount(); ++unit_index) {
			auto &unit = *_unit_filter.get(unit_index);
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
	if (_group_by) {
		for (std::size_t i = 0; i < _groups.size(); ++i) {
			auto group = index(i, 0);
			dataChanged(
				index(0, col->begin_column+first, group),
				index(rowCount(group), col->begin_column+last, group)
			);
		}
	}
}

void GridViewModel::columnBeginInsert(int first, int last)
{
	auto col = qobject_cast<AbstractColumn *>(sender());
	Q_ASSERT(col);
	auto offset = col->begin_column;
	beginInsertColumns({}, offset+first, offset+last);
	if (_group_by)
		for (std::size_t i = 0; i < _groups.size(); ++i)
			beginInsertColumns(index(i, 0), offset+first, offset+last);
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
	if (_group_by)
		for (std::size_t i = 0; i < _groups.size(); ++i)
			endInsertColumns();
}

void GridViewModel::columnBeginRemove(int first, int last)
{
	auto col = qobject_cast<AbstractColumn *>(sender());
	Q_ASSERT(col);
	auto offset = col->begin_column;
	beginRemoveColumns({}, offset+first, offset+last);
	if (_group_by)
		for (std::size_t i = 0; i < _groups.size(); ++i)
			beginRemoveColumns(index(i, 0), offset+first, offset+last);
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
		for (std::size_t i = 0; i < _groups.size(); ++i)
			endRemoveColumns();
}

void GridViewModel::columnBeginMove(int first, int last, int dest)
{
	auto col = qobject_cast<AbstractColumn *>(sender());
	Q_ASSERT(col);
	auto offset = col->begin_column;
	beginMoveColumns({}, offset+first, offset+last, {}, offset+dest);
	if (_group_by)
		for (std::size_t i = 0; i < _groups.size(); ++i) {
			auto group = index(i, 0);
			beginMoveColumns(group, offset+first, offset+last, group, offset+dest);
		}
}

void GridViewModel::columnEndMove(int first, int last, int dest)
{
	endMoveColumns();
	if (_group_by)
		for (std::size_t i = 0; i < _groups.size(); ++i)
			endMoveColumns();
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
