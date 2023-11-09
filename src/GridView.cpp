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

#include "GridViewDelegate.h"
#include "GridViewStyle.h"

GridView::GridView(QWidget *parent):
	QTreeView(parent),
	_style(std::make_unique<GridViewStyle>()),
	_delegate(std::make_unique<GridViewDelegate>())
{
	setStyle(_style.get());
	header()->setStyle(_style.get());

	setItemDelegate(_delegate.get());

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
	header()->setSectionResizeMode(QHeaderView::Fixed);
	header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
}
