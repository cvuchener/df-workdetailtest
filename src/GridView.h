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

#ifndef GRID_VIEW_H
#define GRID_VIEW_H

#include <QTreeView>

class GridViewModel;
class QSortFilterProxyModel;

// GridView uses a GridViewModel and a QSortFilterProxyModel for sorting, index
// from QTreeView will be indexes from the sort model and must be mapped before
// used with the GridViewModel.
class GridView: public QTreeView
{
	Q_OBJECT
public:
	GridView(std::unique_ptr<GridViewModel> &&model, QWidget *parent = nullptr);
	~GridView() override;

	void setModel(QAbstractItemModel *model) override;

	GridViewModel &gridViewModel() { return *_model; };
	QSortFilterProxyModel &sortModel() { return *_sort_model; }

	// Map indexes and selections through both sort and grid view model
	// (between view indexes and DwarfFortressData::units indexes)
	QModelIndex mapToSource(const QModelIndex &index) const;
	QItemSelection mapSelectionToSource(const QItemSelection &selection) const;
	QModelIndex mapFromSource(const QModelIndex &index) const;
	QItemSelection mapSelectionFromSource(const QItemSelection &selection) const;

signals:

protected:
	void rowsInserted(const QModelIndex &index, int start, int end) override;

	// For cell painting
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;

private:
	std::unique_ptr<QStyle> _style;
	std::unique_ptr<GridViewModel> _model;
	std::unique_ptr<QSortFilterProxyModel> _sort_model;
	QPersistentModelIndex _last_index; // only valid when cell painting

	void toggleCells(const QModelIndex &index);
};

#endif
