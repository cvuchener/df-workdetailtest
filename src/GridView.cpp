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

#include "GridViewModel.h"
#include "GridViewStyle.h"

GridView::GridView(QWidget *parent):
	QTreeView(parent),
	_style(std::make_unique<GridViewStyle>())
{
	setStyle(_style.get());
	header()->setStyle(_style.get());

	header()->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(header(), &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
		auto idx = header()->logicalIndexAt(pos);
		if (idx != -1)
			contextMenuRequestedForHeader(idx, header()->mapToGlobal(pos));
	});
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
		auto idx = indexAt(pos);
		if (idx.isValid())
			contextMenuRequestedForCell(idx, viewport()->mapToGlobal(pos));
	});
}

GridView::~GridView()
{
}

void GridView::setModel(QAbstractItemModel *model)
{
	if (auto old_model = GridView::model())
		disconnect(old_model);
	QTreeView::setModel(model);
	connect(model, &QAbstractItemModel::layoutChanged, this, [this](const QList<QPersistentModelIndex> &parents, QAbstractItemModel::LayoutChangeHint hint) {
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
	Q_ASSERT(model->columnCount() > 0);
	header()->setSectionResizeMode(QHeaderView::Fixed);
	header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
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
	auto toggle_cell = [this](const QModelIndex &index) {
		auto checked = index.data(Qt::CheckStateRole).value<Qt::CheckState>();
		model()->setData(index,
				checked == Qt::Checked
					? Qt::Unchecked
					: Qt::Checked,
				Qt::CheckStateRole);
	};
	auto selection = selectionModel();
	if (selection->selectedRows().size() <= 1
			|| !selection->isRowSelected(index.row(), index.parent())) {
		// If clicking outside the selection or single
		// selection, only toggle the cell under the cursor
		toggle_cell(index);
	}
	else { // Toggle all cells from this column in the selection
		// Find the GridViewModel
		std::vector<QAbstractProxyModel *> proxies;
		GridViewModel *gv_model = nullptr;
		auto m = model();
		while (!(gv_model = qobject_cast<GridViewModel *>(m))) {
			if (auto proxy = qobject_cast<QAbstractProxyModel *>(m)) {
				proxies.push_back(proxy);
				m = proxy->sourceModel();
			}
			else
				break;
		}
		if (gv_model) {
			// Toggle all at once if possible
			QModelIndexList indexes;
			for (auto row: selection->selectedRows()) {
				auto idx = row.siblingAtColumn(index.column());
				for (auto proxy: proxies)
					idx = proxy->mapToSource(idx);
				indexes.push_back(idx);
			}
			gv_model->toggleCells(indexes);
		}
		else for (auto row: selection->selectedRows()) // ...or individually
			toggle_cell(row.siblingAtColumn(index.column()));
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
	QTreeView::mouseMoveEvent(event);
}

QItemSelectionModel::SelectionFlags GridView::selectionCommand(const QModelIndex &index, const QEvent *event) const
{
	// Do not update selection when toggling cells
	if (index.isValid()
			&& index.flags() & Qt::ItemIsUserCheckable
			&& selectionModel()->isRowSelected(index.row(), index.parent()))
		return QItemSelectionModel::NoUpdate;
	else
		return QTreeView::selectionCommand(index, event);
}
