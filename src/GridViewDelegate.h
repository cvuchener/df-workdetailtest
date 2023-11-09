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

#ifndef GRID_VIEW_DELEGATE_H
#define GRID_VIEW_DELEGATE_H

#include <QStyledItemDelegate>

class GridViewDelegate: public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit GridViewDelegate(QObject *parent = nullptr);
    ~GridViewDelegate() override;

protected:
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;

private:
    QPersistentModelIndex _last_index;
};

#endif
