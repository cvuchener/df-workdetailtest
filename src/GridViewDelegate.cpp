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

#include "GridViewDelegate.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include "PainterSaver.h"
#include "DataRole.h"

GridViewDelegate::GridViewDelegate(QObject *parent):
	QStyledItemDelegate(parent)
{
}

GridViewDelegate::~GridViewDelegate()
{
}

bool GridViewDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &opt, const QModelIndex &index)
{
	bool enter = false;
	if (event->type() == QEvent::MouseMove) {
		enter = _last_index != index;
		_last_index = index;
	}

	auto option = opt;
	initStyleOption(&option, index);
	if (option.features & QStyleOptionViewItem::HasCheckIndicator) {
		if (event->type() == QEvent::MouseButtonPress) {
			auto mouse = static_cast<QMouseEvent *>(event);
			if (mouse->button() == Qt::LeftButton && option.rect.contains(mouse->pos())) {
				auto checked = index.data(Qt::CheckStateRole).value<Qt::CheckState>();
				model->setData(index,
						checked == Qt::Checked ? Qt::Unchecked : Qt::Checked,
						Qt::CheckStateRole);
			}
		}
		else if (enter) {
			auto mouse = static_cast<QMouseEvent *>(event);
			if (mouse->buttons() & Qt::LeftButton) {
				auto checked = index.data(Qt::CheckStateRole).value<Qt::CheckState>();
				model->setData(index,
						checked == Qt::Checked ? Qt::Unchecked : Qt::Checked,
						Qt::CheckStateRole);
			}
		}
		return false;
	}
	else {
		return QStyledItemDelegate::editorEvent(event, model, option, index);
	}
}
