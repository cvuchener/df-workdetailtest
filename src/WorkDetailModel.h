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

#ifndef WORK_DETAIL_MODEL_H
#define WORK_DETAIL_MODEL_H

#include "ObjectList.h"
#include "WorkDetail.h"

class WorkDetailModel: public ObjectList<WorkDetail>
{
public:
	WorkDetailModel(DwarfFortressData &df, QPointer<DFHack::Client> dfhack, QObject *parent = nullptr);
	~WorkDetailModel() override;

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	QMimeData *mimeData(const QModelIndexList &indexes) const override;
	QStringList mimeTypes() const override;
	Qt::DropActions supportedDropActions() const override;
	bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

	QCoro::Task<> add(WorkDetail::Properties properties, int row = -1);
	QCoro::Task<> remove(QList<QPersistentModelIndex> indexes);
	QCoro::Task<> move(QList<QPersistentModelIndex> indexes, int row);

private:
	DwarfFortressData &_df;
	QPointer<DFHack::Client> _dfhack;
};

#endif
