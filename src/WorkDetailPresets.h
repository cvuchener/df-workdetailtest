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

#ifndef WORK_DETAIL_PRESETS_H
#define WORK_DETAIL_PRESETS_H

#include <QAbstractItemModel>

#include "WorkDetail.h"

#include <QFileInfo>

class ModelMimeData;

class WorkDetailPresets: public QAbstractItemModel
{
	Q_OBJECT
public:
	WorkDetailPresets(QObject *parent = nullptr);
	~WorkDetailPresets() override;

	QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	QModelIndex sibling(int row, int column, const QModelIndex &index) const override;
	int rowCount(const QModelIndex &parent = {}) const override;
	int columnCount(const QModelIndex &parent = {}) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::DisplayRole) override;

	QMimeData *mimeData(const QModelIndexList &indexes) const override;
	QStringList mimeTypes() const override;
	Qt::DropActions supportedDropActions() const override;
	bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
	bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

	struct Preset {
		QString id;
		std::vector<WorkDetail::Properties> properties;
		QFileInfo file_info;

		Preset(const QFileInfo &file);
		Preset(const QString &id, std::vector<WorkDetail::Properties> &&properties = {});

	private:
		friend class WorkDetailPresets;
		int row;
		void save() const;
		bool rename(const QString &new_id);
	};

	// If index is a preset, returns all contained properties
	// If index is a contained work detail, returns its properties (span of size 1)
	std::span<const WorkDetail::Properties> properties(const QModelIndex &index) const;
	// True if a contained workdetail, false if a preset group
	bool isWorkDetail(const QModelIndex &index) const;
	// If index is a work detail, returns its properties, nullptr otherwise
	const WorkDetail::Properties *workdetail(const QModelIndex &index) const;
	// If index is a a preset, returns its data, nullptr otherwise
	const Preset *preset(const QModelIndex &index) const;
	// True for editable/removable presets and their child work details
	bool isUserWritable(const QModelIndex &index) const;

	// Set properties of the work detail designated by index
	bool setProperties(const QModelIndex &index, WorkDetail::Properties &&properties);
	// Add work details to existing preset index
	bool add(std::vector<WorkDetail::Properties> &&properties, const QModelIndex &index);
	// Add a new preset with the given work details
	bool add(std::vector<WorkDetail::Properties> &&properties, const QString &new_preset_name);
	// Remove preset or work detail
	bool remove(const QModelIndex &index);

private:
	std::vector<std::unique_ptr<Preset>> _presets; // sorted by id

	bool copyMimeData(const QMimeData *data, int row, const QModelIndex &parent);
	bool moveMimeData(const ModelMimeData *data, int row, const QModelIndex &parent);

	std::pair<QString, decltype(_presets)::iterator> newPresetId();

	template <typename... Args>
	decltype(_presets)::iterator emplacePreset(decltype(_presets)::iterator pos, Args &&...args);
	void erasePreset(const QModelIndex &index);
};

#endif
