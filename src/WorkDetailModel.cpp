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

#include "WorkDetailModel.h"

#include "Application.h"
#include "IconProvider.h"
#include "DwarfFortressData.h"

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(DFHackLog);

#include <QCoroFuture>
#include "workdetailtest.pb.h"
#include <dfhack-client-qt/Function.h>

static const DFHack::Function<
	dfproto::workdetailtest::AddWorkDetail,
	dfproto::workdetailtest::WorkDetailResult
> AddWorkDetail = {"workdetailtest", "AddWorkDetail"};
static const DFHack::Function<
	dfproto::workdetailtest::RemoveWorkDetail,
	dfproto::workdetailtest::Result
> RemoveWorkDetail = {"workdetailtest", "RemoveWorkDetail"};
static const DFHack::Function<
	dfproto::workdetailtest::MoveWorkDetail,
	dfproto::workdetailtest::Result
> MoveWorkDetail = {"workdetailtest", "MoveWorkDetail"};

#include <QMimeData>

class IndexedMimeData: public QMimeData
{
	QList<QPersistentModelIndex> _indexes;
public:
	IndexedMimeData(const QModelIndexList &indexes)
	{
		for (const auto &index: indexes)
			_indexes.push_back(index);
	}
	~IndexedMimeData() override
	{
	}

	const auto &indexes() const { return _indexes; }
};

WorkDetailModel::WorkDetailModel(DwarfFortressData &df, QPointer<DFHack::Client> dfhack, QObject *parent):
	ObjectList<WorkDetail>(parent),
	_df(df),
	_dfhack(dfhack)
{
}

WorkDetailModel::~WorkDetailModel()
{
}

QVariant WorkDetailModel::data(const QModelIndex &index, int role) const
{
	auto wd = get(index.row());
	switch (role) {
	case Qt::DisplayRole:
		return wd->displayName();
	case Qt::DecorationRole:
		return Application::icons().workdetail((*wd)->icon);
	default:
		return {};
	}
}

Qt::ItemFlags WorkDetailModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::ItemIsDropEnabled;
	else
		return Qt::ItemIsEnabled
			| Qt::ItemIsSelectable
			| Qt::ItemIsDragEnabled;
}

QMimeData *WorkDetailModel::mimeData(const QModelIndexList &indexes) const
{
	auto data = new IndexedMimeData(indexes);
	data->setData("text/plain", "");
	return data;
}

QStringList WorkDetailModel::mimeTypes() const
{
	return {"text/plain"};
}

Qt::DropActions WorkDetailModel::supportedDropActions() const
{
	return Qt::MoveAction;
}

bool WorkDetailModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	if (auto indexed_data = dynamic_cast<const IndexedMimeData *>(data)) {
		move(indexed_data->indexes(), row);
		return true;
	}
	else
		return false;
}

QCoro::Task<> WorkDetailModel::add(WorkDetail::Properties properties, int row)
{
	auto df = _df.shared_from_this(); // make sure the object live for the whole coroutine
	Q_ASSERT(df);
	// Prepare arguments
	dfproto::workdetailtest::AddWorkDetail args;
	if (row >= 0)
		args.set_position(row);
	properties.setArgs(*args.mutable_properties());
	// Call
	if (!_dfhack) {
		qCWarning(DFHackLog) << "DFHack client was deleted";
		co_return;
	}
	auto r = co_await AddWorkDetail(*_dfhack, args).first;
	// Check results
	if (!r) {
		qCWarning(DFHackLog) << "AddWorkDetail failed" << make_error_code(r.cr).message();
		co_return;
	}
	const auto &wd_result = r->work_detail();
	if (!wd_result.success()) {
		qCWarning(DFHackLog) << "AddWorkDetail failed" << wd_result.error();
		co_return;
	}
	// Apply changes
	auto wd = std::make_shared<WorkDetail>(std::make_unique<df::work_detail>(), _df, *_dfhack);
	wd->setProperties(properties, *r);
	if (row < 0) {
		beginInsertRows({}, _objects.size(), _objects.size());
		_objects.push_back(std::move(wd));
		endInsertRows();
	}
	else {
		beginInsertRows({}, row, row);
		_objects.insert(_objects.begin()+row, std::move(wd));
		endInsertRows();
	}
}

QCoro::Task<> WorkDetailModel::remove(QList<QPersistentModelIndex> indexes)
{
	auto df = _df.shared_from_this(); // make sure the object live for the whole coroutine
	Q_ASSERT(df);
	for (const auto &index: indexes) {
		auto wd = get(index.row());
		dfproto::workdetailtest::RemoveWorkDetail args;
		args.mutable_id()->set_index(index.row());
		args.mutable_id()->set_name((*wd)->name);
		// Call
		if (!_dfhack) {
			qCWarning(DFHackLog) << "DFHack client was deleted";
			co_return;
		}
		auto r = co_await RemoveWorkDetail(*_dfhack, args).first;
		// Check results
		if (!r) {
			qCWarning(DFHackLog) << "RemoveWorkDetail failed" << make_error_code(r.cr).message();
			co_return;
		}
		if (!r->success()) {
			qCWarning(DFHackLog) << "RemoveWorkDetail failed" << r->error();
			co_return;
		}
		// Apply changes
		beginRemoveRows({}, index.row(), index.row());
		_objects.erase(_objects.begin()+index.row());
		endRemoveRows();
	}
}

QCoro::Task<> WorkDetailModel::move(QList<QPersistentModelIndex> indexes, int row)
{
	auto df = _df.shared_from_this(); // make sure the object live for the whole coroutine
	Q_ASSERT(df);
	if (row < 0)
		row = _objects.size();
	for (const auto &index: indexes) {
		auto old_row = index.row();
		if (row < old_row || row > old_row+1) {
			auto wd = _objects.begin()+index.row();
			dfproto::workdetailtest::MoveWorkDetail args;
			(*wd)->setId(*args.mutable_id());
			args.set_new_position(row);
			if (!_dfhack)
				break;
			auto r = co_await MoveWorkDetail(*_dfhack, args).first;
			if (!r) {
				qCWarning(DFHackLog) << "MoveWorkDetail failed" << make_error_code(r.cr).message();
				co_return;
			}
			if (!r->success()) {
				qCWarning(DFHackLog) << "MoveWorkDetail failed" << r->error();
				co_return;
			}
			beginMoveRows({}, old_row, old_row, {}, row);
			auto destination = _objects.begin()+row;
			if (old_row > row)
				std::rotate(destination, wd, next(wd));
			else
				std::rotate(wd, next(wd), destination);
			endMoveRows();
		}
		if (old_row >= row)
			++row;
	}
}
