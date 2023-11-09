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

#ifndef GRID_VIEW_MODEL_H
#define GRID_VIEW_MODEL_H

#include <QAbstractItemModel>

class QMenu;
class DwarfFortress;
class AbstractColumn;
class UnitFilterProxyModel;

class GridViewModel: public QAbstractItemModel
{
	Q_OBJECT
public:
	GridViewModel(DwarfFortress &df, QObject *parent = nullptr);
	~GridViewModel() override;

	QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	int rowCount(const QModelIndex &parent = {}) const override;
	int columnCount(const QModelIndex &parent = {}) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	QModelIndex sourceUnitIndex(const QModelIndex &index) const;

	void makeColumnMenu(int section, QMenu *menu, QWidget *parent);
	void makeCellMenu(const QModelIndex &index, QMenu *menu, QWidget *parent);

private slots:
	void unitDataChanged(const QModelIndex &first, const QModelIndex &last, const QList<int> &roles);
	void cellDataChanged(int first, int last, int unit_id);
	void columnDataChanged(int first, int last);
	void columnBeginInsert(int first, int last);
	void columnBeginRemove(int first, int last);
	void columnEndReset();
private:
	DwarfFortress &_df;
	std::unique_ptr<UnitFilterProxyModel> _unit_filter;
	std::vector<std::unique_ptr<AbstractColumn>> _columns;

	std::pair<const AbstractColumn *, int> getColumn(int col) const;
	std::pair<AbstractColumn *, int> getColumn(int col);
};

#endif
