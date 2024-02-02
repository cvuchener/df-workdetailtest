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

#include "GridView.h"

#include <QHeaderView>
#include <QMouseEvent>
#include <QAbstractProxyModel>
#include <QMenu>

#include "GridViewModel.h"
#include "GridViewStyle.h"
#include "DataRole.h"

GridView::GridView(std::unique_ptr<GridViewModel> &&model, QWidget *parent):
	QTreeView(parent),
	_style(std::make_unique<GridViewStyle>()),
	_model(std::move(model)),
	_sort_model(std::make_unique<QSortFilterProxyModel>())
{
	setStyle(_style.get());
	header()->setStyle(_style.get());

	setMouseTracking(true);
	setSelectionMode(QAbstractItemView::ExtendedSelection);
	setSortingEnabled(true);
	header()->setStretchLastSection(false);

	Q_ASSERT(_model);
	_sort_model->setSourceModel(_model.get());
	_sort_model->setSortRole(DataRole::SortRole);
	QTreeView::setModel(_sort_model.get());
	connect(_model.get(), &QAbstractItemModel::layoutChanged, this, [this](const QList<QPersistentModelIndex> &parents, QAbstractItemModel::LayoutChangeHint hint) {
		if (hint == QAbstractItemModel::NoLayoutChangeHint) { // ignore sorting changes
			if (!parents.empty()) {
				for(auto parent: parents)
					expandRecursively(parent);
			}
			else
				expandAll();
		}
		// stop painting cells
		_last_index = QModelIndex{};
	});
	header()->setSectionResizeMode(QHeaderView::Fixed);
	header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

	header()->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(header(), &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
		auto idx = header()->logicalIndexAt(pos);
		if (idx != -1) {
			QMenu menu;
			_model->makeColumnMenu(idx, &menu, this);
			if (!menu.isEmpty())
				menu.exec(header()->mapToGlobal(pos));
		}
	});
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
		auto idx = indexAt(pos);
		if (idx.isValid()) {
			QMenu menu;
			_model->makeCellMenu(_sort_model->mapToSource(idx), &menu, this);
			if (!menu.isEmpty())
				menu.exec(viewport()->mapToGlobal(pos));
		}
	});
}

GridView::~GridView()
{
}

void GridView::setModel(QAbstractItemModel *model)
{
	qFatal("GridView's model should only be set through the constructor");
}

QModelIndex GridView::mapToSource(const QModelIndex &index) const
{
	return _model->mapToSource(_sort_model->mapToSource(index));
}

QItemSelection GridView::mapSelectionToSource(const QItemSelection &selection) const
{
	return _model->mapSelectionToSource(_sort_model->mapSelectionToSource(selection));
}

QModelIndex GridView::mapFromSource(const QModelIndex &index) const
{
	return _sort_model->mapFromSource(_model->mapFromSource(index));
}

QItemSelection GridView::mapSelectionFromSource(const QItemSelection &selection) const
{
	return _sort_model->mapSelectionFromSource(_model->mapSelectionFromSource(selection));
}

void GridView::rowsInserted(const QModelIndex &index, int start, int end)
{
	QTreeView::rowsInserted(index, start, end);
	if (model())
		for (int i = start; i <= end; ++i)
			expand(model()->index(i, 0, index));
}

void GridView::toggleCells(const QModelIndex &index)
{
	auto selection = selectionModel();
	if (selection->selectedRows().size() <= 1
			|| !selection->isRowSelected(index.row(), index.parent())) {
		// If clicking outside the selection or single
		// selection, only toggle the cell under the cursor
		auto checked = index.data(Qt::CheckStateRole).value<Qt::CheckState>();
		model()->setData(index,
				checked == Qt::Checked
					? Qt::Unchecked
					: Qt::Checked,
				Qt::CheckStateRole);
	}
	else { // Toggle all cells from this column in the selection
		QModelIndexList indexes;
		for (auto row: selection->selectedRows()) {
			auto idx = row.siblingAtColumn(index.column());
			indexes.push_back(_sort_model->mapToSource(idx));
		}
		_model->toggleCells(indexes);
	}
}

void GridView::mousePressEvent(QMouseEvent *event)
{
	auto index = indexAt(event->pos());
	if (event->button() == Qt::LeftButton && index.flags() & Qt::ItemIsUserCheckable) {
		_last_index = index; // start painting cells
		toggleCells(index);
	}
	else
		QTreeView::mousePressEvent(event);
}

void GridView::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton && _last_index.isValid())
		_last_index = QModelIndex{}; // stop painting cells
	else
		QTreeView::mouseReleaseEvent(event);
}

void GridView::mouseMoveEvent(QMouseEvent *event)
{
	if (_last_index.isValid()) { // painting cells
		if (event->buttons() & Qt::LeftButton) {
			auto index = indexAt(event->pos());
			if (_last_index != index) { // entered new cell
				_last_index = index;
				if (index.flags() & Qt::ItemIsUserCheckable) {
					toggleCells(index);
				}
			}
		}
		else {
			_last_index = QModelIndex{};
		}
	}
	else
		QTreeView::mouseMoveEvent(event);
}
