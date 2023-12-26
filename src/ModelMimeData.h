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

#ifndef MODEL_MIME_DATA_H
#define MODEL_MIME_DATA_H

#include <QMimeData>
#include <QAbstractItemModel>

class ModelMimeData: public QMimeData
{
	Q_OBJECT
public:
	ModelMimeData(const QAbstractItemModel *source_model, const QModelIndexList &indexes);
	~ModelMimeData() override;

	const QAbstractItemModel *sourceModel() const { return _source_model; }
	const auto &indexes() const { return _indexes; }

private:
	const QAbstractItemModel *_source_model;
	QList<QPersistentModelIndex> _indexes;
};

#endif
