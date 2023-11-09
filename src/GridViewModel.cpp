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
#include "WorkDetailColumn.h"
#include "SpecialistColumn.h"
#include "CP437.h"

GridViewModel::GridViewModel(DwarfFortress &df, QObject *parent):
	QAbstractItemModel(parent),
	_df(df),
	_unit_filter(std::make_unique<UnitFilterProxyModel>())
{
	_columns.push_back(std::make_unique<WorkDetailColumn>(df));
	_columns.push_back(std::make_unique<SpecialistColumn>(df));

	_unit_filter->setUnitFilter(&Unit::canAssignWork);
	_unit_filter->setSourceModel(&_df.units());
	connect(_unit_filter.get(), &QAbstractItemModel::modelAboutToBeReset,
		this, [this]() {
			beginResetModel();
		});
	connect(_unit_filter.get(), &QAbstractItemModel::modelReset,
		this, [this]() {
			endResetModel();
		});
	connect(_unit_filter.get(), &QAbstractItemModel::dataChanged,
		this, &GridViewModel::unitDataChanged);
	connect(_unit_filter.get(), &QAbstractItemModel::rowsAboutToBeInserted,
		this, [this](const QModelIndex &, int first, int last) {
			beginInsertRows({}, first, last);
		});
	connect(_unit_filter.get(), &QAbstractItemModel::rowsInserted,
		this, [this](const QModelIndex &, int, int) {
			endInsertRows();
		});
	connect(_unit_filter.get(), &QAbstractItemModel::rowsAboutToBeRemoved,
		this, [this](const QModelIndex &, int first, int last) {
			beginRemoveRows({}, first, last);
		});
	connect(_unit_filter.get(), &QAbstractItemModel::rowsRemoved,
		this, [this](const QModelIndex &, int, int) {
			endRemoveRows();
		});
	int count = 1;
	for (auto &col: _columns) {
		col->begin_column = count;
		col->end_column = (count += col->count());
		connect(col.get(), &AbstractColumn::columnsAboutToBeReset,
			this, [this]() {
				beginResetModel();
			});
		connect(col.get(), &AbstractColumn::columnsReset,
			this, &GridViewModel::columnEndReset);
		connect(col.get(), &AbstractColumn::unitDataChanged,
			this, &GridViewModel::cellDataChanged);
		connect(col.get(), &AbstractColumn::columnDataChanged,
			this, &GridViewModel::columnDataChanged);
		connect(col.get(), &AbstractColumn::columnsAboutToBeInserted,
			this, &GridViewModel::columnBeginInsert);
		connect(col.get(), &AbstractColumn::columnsInserted,
			this, [this](int first, int last) {
				endInsertColumns();
			});
		connect(col.get(), &AbstractColumn::columnsAboutToBeRemoved,
			this, &GridViewModel::columnBeginRemove);
		connect(col.get(), &AbstractColumn::columnsRemoved,
			this, [this](int first, int last) {
				endRemoveColumns();
			});
	}
}

GridViewModel::~GridViewModel()
{
}

QModelIndex GridViewModel::index(int row, int column, const QModelIndex &parent) const
{
	if (parent.isValid())
		return {};
	return createIndex(row, column);
}

QModelIndex GridViewModel::parent(const QModelIndex &index) const
{
	return {};
}

int GridViewModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;
	else
		return _unit_filter->rowCount();
}

int GridViewModel::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;
	else {
		return _columns.back()->end_column;
	}
}

QVariant GridViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal)
		return {};
	if (section == 0) {
		switch (role) {
		case Qt::DisplayRole:
			return tr("Name");
		default:
			return {};
		}
	}
	else {
		auto [col, s] = getColumn(section);
		return col->headerData(s, role);
	}
}

QVariant GridViewModel::data(const QModelIndex &index, int role) const
{
	auto unit = _unit_filter->get(index.row());
	Q_ASSERT(unit);
	if (index.column() == 0) {
		switch (role) {
		case Qt::DisplayRole:
			return unit->displayName();
		case Qt::EditRole:
			return fromCP437((*unit)->name.nickname);
		default:
			return {};
		}
	}
	else {
		auto [col, section] = getColumn(index.column());
		return col->unitData(section, *unit, role);
	}
}

bool GridViewModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	auto unit = _unit_filter->get(index.row());
	Q_ASSERT(unit);
	if (index.column() == 0) {
		if (role != Qt::EditRole)
			return false;
		unit->edit({ .nickname = value.toString() });
		return true;
	}
	else {
		auto [col, section] = getColumn(index.column());
		return col->setUnitData(section, *unit, value, role);
	}
}

Qt::ItemFlags GridViewModel::flags(const QModelIndex &index) const
{
	auto unit = _unit_filter->get(index.row());
	Q_ASSERT(unit);
	if (index.column() == 0) {
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemNeverHasChildren;
	}
	else {
		auto [col, section] = getColumn(index.column());
		return col->flags(section, *unit) | Qt::ItemNeverHasChildren;
	}
}

QModelIndex GridViewModel::sourceUnitIndex(const QModelIndex &index) const
{
	return _unit_filter->mapToSource(_unit_filter->index(index.row(), 0));
}

#include <QMenu>
#include <QInputDialog>

void GridViewModel::makeColumnMenu(int section, QMenu *menu, QWidget *parent)
{
	if (section != 0) {
		auto [col, idx] = getColumn(section);
		col->makeHeaderMenu(idx, menu, parent);
	}
}

void GridViewModel::makeCellMenu(const QModelIndex &index, QMenu *menu, QWidget *parent)
{
	auto unit = _unit_filter->get(index.row());
	if (index.column() == 0) {
		auto nickname_action = new QAction(tr("Edit %1 nickname...").arg(unit->displayName()), menu);
		connect(nickname_action, &QAction::triggered, [unit, parent]() {
			bool ok;
			auto new_nickname = QInputDialog::getText(parent,
					tr("Edit nickname"),
					tr("Choose a new nickname for %1:").arg(unit->displayName()),
					QLineEdit::Normal,
					fromCP437((*unit)->name.nickname),
					&ok);
			if (ok)
				unit->edit({ .nickname = new_nickname });
		});
		menu->addAction(nickname_action);
	}
	else {
		auto [col, sec] = getColumn(index.column());
		col->makeUnitMenu(sec, *unit, menu, parent);
	}
}

void GridViewModel::unitDataChanged(const QModelIndex &first, const QModelIndex &last, const QList<int> &roles)
{
	dataChanged(index(first.row(), 0), index(last.row(), columnCount()-1));
}

void GridViewModel::cellDataChanged(int first, int last, int unit_id)
{
	auto col = qobject_cast<AbstractColumn *>(sender());
	Q_ASSERT(col);
	auto unit_idx = _unit_filter->mapFromSource(_df.units().find(unit_id));
	if (unit_idx.isValid())
		dataChanged(
			index(unit_idx.row(), col->begin_column+first),
			index(unit_idx.row(), col->begin_column+last)
		);
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

void GridViewModel::columnBeginRemove(int first, int last)
{
	auto col = qobject_cast<AbstractColumn *>(sender());
	Q_ASSERT(col);
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
