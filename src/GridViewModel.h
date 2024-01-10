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
#include <QLoggingCategory>

#include "UnitFilterProxyModel.h"
#include "Columns/Factory.h"

Q_DECLARE_LOGGING_CATEGORY(GridViewLog);

class QMenu;
class DwarfFortressData;
class AbstractColumn;
class GroupBy;
class Unit;

class GridViewModel: public QAbstractItemModel
{
	Q_OBJECT
public:
	struct Parameters {
		QString title;
		UnitFilter filter;
		std::vector<Columns::Factory> columns;

		static Parameters fromJson(const QJsonDocument &);
	};
	GridViewModel(const Parameters &parameters, std::shared_ptr<DwarfFortressData> df, QObject *parent = nullptr);
	~GridViewModel() override;

	const QString &title() const { return _title; }

	const std::shared_ptr<UserUnitFilters> &userFilters() const { return _user_filters; }
	void setUserFilters(std::shared_ptr<UserUnitFilters> user_filters);

	void setGroupBy(int index);
	int groupIndex() const { return _group_index; }

	QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	QModelIndex sibling(int row, int column, const QModelIndex &index) const override;
	int rowCount(const QModelIndex &parent = {}) const override;
	int columnCount(const QModelIndex &parent = {}) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	const Unit *unit(const QModelIndex &index) const;

	void makeColumnMenu(int section, QMenu *menu, QWidget *parent);
	void makeCellMenu(const QModelIndex &index, QMenu *menu, QWidget *parent);

	void toggleCells(const QModelIndexList &indexes);

private slots:
	void cellDataChanged(int first, int last, int unit_id);

	void unitDataChanged(const QModelIndex &first, const QModelIndex &last, const QList<int> &roles);

	void unitBeginInsert(const QModelIndex &, int first, int last);
	void unitEndInsert(const QModelIndex &, int first, int last);

	void unitBeginRemove(const QModelIndex &, int first, int last);
	void unitEndRemove(const QModelIndex &, int first, int last);

	void columnDataChanged(int first, int last);

	void columnBeginInsert(int first, int last);
	void columnEndInsert(int first, int last);

	void columnBeginRemove(int first, int last);
	void columnEndRemove(int first, int last);

	void columnBeginMove(int first, int last, int dest);
	void columnEndMove(int first, int last, int dest);

private:
	std::shared_ptr<DwarfFortressData> _df;
	QString _title;
	UnitFilterProxyModel _unit_filter;
	std::shared_ptr<UserUnitFilters> _user_filters;
	std::vector<std::unique_ptr<AbstractColumn>> _columns;
	std::unique_ptr<GroupBy> _group_by;
	int _group_index;
	struct group_t {
		quint64 id;
		std::vector<Unit *> units; // sorted by id
	};
	std::vector<group_t> _groups; // sorted by id
	std::map<int, quint64> _unit_group; // unit id -> group id
	auto findGroup(quint64 id) {
		auto it = std::ranges::lower_bound(_groups, id, {}, &group_t::id);
		Q_ASSERT(it != _groups.end() && it->id == id);
		return it;
	}
	auto findGroup(quint64 id) const {
		auto it = std::ranges::lower_bound(_groups, id, {}, &group_t::id);
		Q_ASSERT(it != _groups.end() && it->id == id);
		return it;
	}

	QModelIndex groupIndex(decltype(_groups)::const_iterator it) const;
	QModelIndex unitIndex(int unit_id) const;
	void addUnitToGroup(Unit &unit, quint64 group_id, bool reseting = false);
	void removeFromGroup(const QModelIndex &index);
	void updateGroupedUnit(Unit &unit, int first_col, int last_col);
	void rebuildGroups();

	template <typename Model, typename UnitAction, typename GroupAction, typename... Args>
	static auto applyToIndex(Model &&model, const QModelIndex &index, UnitAction &&unit_action, GroupAction &&group_action, Args &&...args);

	std::pair<const AbstractColumn *, int> getColumn(int col) const;
	std::pair<AbstractColumn *, int> getColumn(int col);
};

#endif
