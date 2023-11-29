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
	QTreeView::setModel(model);
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

bool GridView::edit(const QModelIndex &index, EditTrigger trigger, QEvent *event)
{
	// Implement toggling the whole selection and toggling bt dragging the
	// mouse over cells.
	auto toggle = [this](const QModelIndex &index) {
		auto checked = index.data(Qt::CheckStateRole).value<Qt::CheckState>();
		model()->setData(index,
				checked == Qt::Checked
					? Qt::Unchecked
					: Qt::Checked,
				Qt::CheckStateRole);
	};
	if (auto mouse = dynamic_cast<QMouseEvent *>(event)) {
		bool enter = false;
		if (mouse->buttons() & Qt::LeftButton) {
			enter = _last_index != index;
			_last_index = index;
		}
		else {
			_last_index = QModelIndex{};
		}

		switch (mouse->type()) {
		case QEvent::MouseButtonPress:
			if (mouse->button() == Qt::LeftButton && index.flags() & Qt::ItemIsUserCheckable) {
				if (!selectionModel()->isRowSelected(index.row(), index.parent())) {
					toggle(index);
				}
				else {
					for (auto row: selectionModel()->selectedRows())
						toggle(row.siblingAtColumn(index.column()));
				}
				return false;
			}
			break;
		case QEvent::MouseButtonRelease:
			if (mouse->button() == Qt::LeftButton && index.flags() & Qt::ItemIsUserCheckable) {
				_last_index = QModelIndex{};
				return false;
			}
			break;
		case QEvent::MouseMove:
			if (enter && index.flags() & Qt::ItemIsUserCheckable) {
				if (!selectionModel()->isRowSelected(index.row(), index.parent())) {
					toggle(index);
				}
				else {
					for (auto row: selectionModel()->selectedRows())
						toggle(row.siblingAtColumn(index.column()));
				}
				return false;
			}
		default:
			break;
		}
	}
	return QTreeView::edit(index, trigger, event);
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
