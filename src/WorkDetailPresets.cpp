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

#include "WorkDetailPresets.h"

#include "StandardPaths.h"
#include "Application.h"
#include "IconProvider.h"
#include "ModelMimeData.h"
#include "LogCategory.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMimeData>

#include <QInputDialog>

WorkDetailPresets::Preset::Preset(const QFileInfo &fi):
	id(fi.baseName()),
	file_info(fi)
{
	QFile file(fi.filePath());
	if (!file.open(QIODeviceBase::ReadOnly)) {
		throw std::runtime_error("Failed to open");
	}
	QJsonParseError error;
	auto doc = QJsonDocument::fromJson(file.readAll(), &error);
	if (error.error != QJsonParseError::NoError) {
		throw error;
	}
	if (doc.isArray()) {
		auto array = doc.array();
		properties.reserve(array.size());
		for (auto item: array)
			properties.push_back(WorkDetail::Properties::fromJson(item.toObject()));
	}
	else if (doc.isObject()) {
		properties.push_back(WorkDetail::Properties::fromJson(doc.object()));
	}
}

WorkDetailPresets::Preset::Preset(const QString &id, std::vector<WorkDetail::Properties> &&p):
	id(id),
	properties(std::move(p)),
	file_info(StandardPaths::writable_data_location() + "/workdetails/" + id + ".json")
{
}

void WorkDetailPresets::Preset::save() const
{
	auto dir = file_info.dir();
	if (!dir.exists()) {
		if (!dir.mkpath(".")) {
			qCCritical(WorkDetailLog) << "Failed to create directory" << dir.absolutePath();
			return;
		}
	}
	QFile file(file_info.filePath());
	if (!file.open(QIODeviceBase::WriteOnly | QIODeviceBase::Truncate)) {
		qCCritical(WorkDetailLog) << "Failed to open for writing" << file_info.absoluteFilePath();
		return;
	}
	QJsonDocument doc;
	if (properties.size() == 1)
		doc.setObject(properties.front().toJson());
	else {
		QJsonArray array;
		for (const auto &p: properties)
			array.append(p.toJson());
		doc.setArray(array);
	}
	file.write(doc.toJson());
	qCInfo(WorkDetailLog) << "saved" << file_info.filePath();
}

bool WorkDetailPresets::Preset::rename(const QString &new_id)
{
	QFileInfo new_file_info(file_info.dir(), new_id + ".json");
	if (!QFile::rename(file_info.filePath(), new_file_info.filePath()))
		return false;
	qCInfo(WorkDetailLog)
		<< "renamed" << file_info.filePath()
		<< "to" << new_file_info.filePath();
	id = new_id;
	file_info = new_file_info;
	return true;
}

WorkDetailPresets::WorkDetailPresets(QObject *parent):
	QAbstractItemModel(parent)
{
	QStringList name_filter = {"*.json"};
	for (QDir data_dir: StandardPaths::data_locations()) {
		QDir dir = data_dir.filePath("workdetails");
		for (const auto &fi: dir.entryInfoList(name_filter, QDir::Files)) {
			try {
				auto it = std::ranges::lower_bound(_presets, fi.baseName(), {}, &Preset::id);
				if (it != _presets.end() && (*it)->id == fi.baseName()) {
					qCInfo(WorkDetailLog)
						<< "Ignoring work detail preset" << fi.baseName()
						<< "from" << fi.absoluteFilePath();
				}
				else {
					emplacePreset(it, fi);
					qCInfo(WorkDetailLog)
						<< "Added work detail preset" << fi.baseName()
						<< "from" << fi.absoluteFilePath();
				}
			}
			catch (std::exception &e) {
				qCCritical(WorkDetailLog) << e.what() << fi.filePath();
			}
			catch (QJsonParseError &error) {
				qCCritical(WorkDetailLog)
					<< "Failed to parse json from"
					<< fi.filePath()
					<< ":"
					<< error.errorString();
			}
		}
	}
}

WorkDetailPresets::~WorkDetailPresets()
{
}

QModelIndex WorkDetailPresets::index(int row, int column, const QModelIndex &parent) const
{
	if (parent.isValid())
		return createIndex(row, column, _presets.at(parent.row()).get());
	else
		return createIndex(row, column, nullptr);
}

QModelIndex WorkDetailPresets::parent(const QModelIndex &index) const
{
	if (auto preset = static_cast<const Preset *>(index.internalPointer()))
		return createIndex(preset->row, 0, nullptr);
	else
		return {};
}

QModelIndex WorkDetailPresets::sibling(int row, int column, const QModelIndex &index) const
{
	return createIndex(row, column, index.internalPointer());
}

int WorkDetailPresets::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid()) {
		if (parent.internalPointer())
			return 0;
		else
			return _presets.at(parent.row())->properties.size();
	}
	else
		return _presets.size();
}

int WorkDetailPresets::columnCount(const QModelIndex &) const
{
	return 1;
}

QVariant WorkDetailPresets::data(const QModelIndex &index, int role) const
{
	if (auto parent = static_cast<const Preset *>(index.internalPointer())) { // work detail leaf
		const auto &properties = parent->properties.at(index.row());
		switch (role) {
		case Qt::DisplayRole:
			return properties.name;
		case Qt::DecorationRole:
			if (properties.icon && *properties.icon != df::work_detail_icon::ICON_NONE)
				return Application::icons().workdetail(*properties.icon);
			else
				return {};
		case Qt::ToolTipRole: {
			QString tip = "<h3>" + properties.name + "</h3><ul>";
			if (properties.mode) {
				tip += "<p>";
				switch (*properties.mode) {
				case df::work_detail_mode::EverybodyDoesThis:
					tip += tr("Everybody does this");
					break;
				case df::work_detail_mode::OnlySelectedDoesThis:
					tip += tr("Only selected does this");
					break;
				case df::work_detail_mode::NobodyDoesThis:
					tip += tr("Nobody does this");
					break;
				default:
					break;
				}
				tip += "</p>";
			}
			for (const auto &[labor, enabled]: properties.labors)
				if (enabled)
					tip += "<li>" + QString::fromLocal8Bit(caption(labor)) + "</li>";
			tip += "</ul>";
			return tip;
		}
		default:
			return {};
		}
	}
	else { // preset node
		const auto &preset = *_presets.at(index.row());
		switch (role) {
		case Qt::DisplayRole:
		case Qt::EditRole:
			return preset.id;
		case Qt::DecorationRole:
			if (!preset.file_info.isWritable())
				return QIcon::fromTheme("object-locked");
			else
				return {};
		case Qt::ToolTipRole: {
			QString tip = "<h3>" + preset.id + "</h3><ul>";
			tip += "<p>" + preset.file_info.absoluteFilePath() + "</p>";
			for (const auto &properties: preset.properties)
				tip += "<li>" + properties.name + "</li>";
			tip += "</ul>";
			return tip;
		}
		default:
			return {};
		}
	}
}

Qt::ItemFlags WorkDetailPresets::flags(const QModelIndex &index) const
{
	auto flags = QAbstractItemModel::flags(index);
	if (!index.isValid()) { // Root
		flags |= Qt::ItemIsDropEnabled;
	}
	else if (index.internalPointer() == nullptr) { // Presets groups/files
		flags |= Qt::ItemIsDragEnabled;
		const auto &preset = *_presets.at(index.row());
		if (preset.file_info.isWritable()) {
			flags |= Qt::ItemIsDropEnabled;
			flags |= Qt::ItemIsEditable;
		}
	}
	else { // Work details
		flags |= Qt::ItemIsDragEnabled;
		flags |= Qt::ItemNeverHasChildren;
	}
	return flags;
}

bool WorkDetailPresets::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid() || index.internalPointer() != nullptr)
		return false;
	auto &preset = *_presets.at(index.row());
	if (!preset.file_info.isWritable())
		return false;
	switch (role) {
	case Qt::EditRole: {
		QString new_id = value.toString();
		if (new_id == preset.id)
			return true;
		auto new_pos = std::ranges::lower_bound(_presets, new_id, {}, &Preset::id);
		if (new_pos != _presets.end() && (*new_pos)->id == new_id)
			return false;
		if (!preset.rename(new_id)) {
			qCCritical(WorkDetailLog) << "Cannot rename" << preset.id << "to" << new_id;
			return false;
		}
		int new_row = distance(_presets.begin(), new_pos);
		if (new_row != index.row() && new_row != index.row()+1) {
			beginMoveRows({}, index.row(), index.row(), {}, new_row);
			auto old_pos = _presets.begin()+index.row();
			(*old_pos)->row = new_row;
			if (new_row > index.row()) {
				for (auto it = old_pos; it != new_pos; ++it)
					--(*it)->row;
				std::rotate(old_pos, next(old_pos), new_pos);
			}
			else {
				for (auto it = new_pos; it != old_pos; ++it)
					++(*it)->row;
				std::rotate(new_pos, old_pos, next(old_pos));
			}
			endMoveRows();
		}
		else
			dataChanged(index, index);
		return true;
	}
	default:
		return false;
	}
}

QMimeData *WorkDetailPresets::mimeData(const QModelIndexList &indexes) const
{
	QJsonDocument doc;
	std::size_t count = 0;
	for (const auto &index: indexes) {
		if (index.internalPointer() == nullptr)
			count += _presets.at(index.row())->properties.size();
		else
			++count;
	}
	if (count == 1) {
		for (const auto &index: indexes) {
			if (auto parent = static_cast<const Preset *>(index.internalPointer())) {
				const auto &properties = parent->properties.at(index.row());
				doc.setObject(properties.toJson());
			}
			else {
				const auto &preset = *_presets.at(index.row());
				if (!preset.properties.empty()) {
					doc.setObject(preset.properties.front().toJson());
				}
			}
		}
	}
	else {
		QJsonArray array;
		for (const auto &index: indexes) {
			if (auto parent = static_cast<const Preset *>(index.internalPointer())) {
				const auto &properties = parent->properties.at(index.row());
				array.append(properties.toJson());
			}
			else {
				const auto &preset = *_presets.at(index.row());
				for (const auto &properties: preset.properties) {
					array.append(properties.toJson());
				}
			}
		}
		doc.setArray(array);
	}
	auto data = new ModelMimeData(this, indexes);
	data->setData("application/json", doc.toJson(QJsonDocument::Compact));
	data->setText(doc.toJson(QJsonDocument::Indented));
	return data;
}

QStringList WorkDetailPresets::mimeTypes() const
{
	return {"application/json", "text/plain"};
}

Qt::DropActions WorkDetailPresets::supportedDropActions() const
{
	return Qt::CopyAction | Qt::MoveAction;
}

bool WorkDetailPresets::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
	switch (action) {
	case Qt::CopyAction:
		if (parent.isValid() && parent.internalPointer() == nullptr) {
			auto &preset = *_presets.at(parent.row());
			if (!preset.file_info.isWritable())
				return false; // Don't copy to a read-only preset
		}
		break;
	case Qt::MoveAction:
		if (auto model_data = qobject_cast<const ModelMimeData *>(data)) {
			if (model_data->sourceModel() != this)
				return false; // Don't move from other model
			if (parent.isValid() && parent.internalPointer() == nullptr) {
				auto &preset = *_presets.at(parent.row());
				if (!preset.file_info.isWritable())
					return false; // Don't move to read-only preset
			}
			for (const auto &index: model_data->indexes()) {
				if (index.isValid() && !isUserWritable(index))
					return false; // Don't move from read-only preset
			}
			if (!parent.isValid()
					&& model_data->indexes().size() == 1
					&& model_data->indexes().front().internalPointer() == nullptr)
				return false; // Don't move a single preset to a new preset

		}
		else
			return false;
		break;
	default:
		break;
	}
	return QAbstractItemModel::canDropMimeData(data, action, row, column, parent);
}

bool WorkDetailPresets::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	switch (action) {
	case Qt::CopyAction:
		return copyMimeData(data, row, parent);
	case Qt::MoveAction:
		if (auto model_data = qobject_cast<const ModelMimeData *>(data))
			return moveMimeData(model_data, row, parent);
		[[fallthrough]];
	default:
		return false;
	}
}

bool WorkDetailPresets::copyMimeData(const QMimeData *data, int row, const QModelIndex &parent)
{
	// Parse json data
	QJsonDocument doc;
	QJsonParseError error;
	if (data->hasFormat("application/json"))
		doc = QJsonDocument::fromJson(data->data("application/json"), &error);
	else if (data->hasFormat("text/plain"))
		doc = QJsonDocument::fromJson(data->data("text/plain"), &error);
	else
		return false;
	if (error.error != QJsonParseError::NoError) {
		qCWarning(WorkDetailLog) << "Invalid dropped json" << error.errorString();
		return false;
	}
	std::vector<WorkDetail::Properties> properties;
	if (doc.isObject())
		properties.push_back(WorkDetail::Properties::fromJson(doc.object()));
	else if (doc.isArray()) {
		auto array = doc.array();
		properties.reserve(array.size());
		for (auto item: array)
			properties.push_back(WorkDetail::Properties::fromJson(item.toObject()));
	}
	if (properties.empty())
		return false;
	if (parent.isValid()) {
		Q_ASSERT(parent.internalPointer() == nullptr);
		// Insert work details in existing preset
		auto &preset = *_presets.at(parent.row());
		if (!preset.file_info.isWritable())
			return false;
		auto insert_pos = row < 0
			? preset.properties.end()
			: preset.properties.begin() + row;
		row = distance(preset.properties.begin(), insert_pos);
		beginInsertRows(parent, row, row);
		preset.properties.insert(
				insert_pos,
				std::move_iterator(properties.begin()),
				std::move_iterator(properties.end()));
		endInsertRows();
		preset.save();
		dataChanged(parent, parent);
	}
	else {
		// Add new preset
		auto [new_id, it] = newPresetId();
		int row = distance(_presets.begin(), it);
		beginInsertRows({}, row, row);
		it = emplacePreset(it, new_id, std::move(properties));
		endInsertRows();
		(*it)->save();
	}
	return true;
}

bool WorkDetailPresets::moveMimeData(const ModelMimeData *data, int row, const QModelIndex &parent)
{
	// Validate data
	if (data->sourceModel() != this)
		return false;
	if (parent.isValid()) {
		// Destination is writable
		if (parent.internalPointer() != nullptr)
			return false;
		auto &preset = *_presets.at(parent.row());
		if (!preset.file_info.isWritable())
			return false;
	}
	for (const auto &index: data->indexes()) {
		// Source is writable
		if (index.isValid() && !isUserWritable(index))
			return false;
	}
	// Find destination preset
	QPersistentModelIndex dest_preset;
	decltype(Preset::properties)::iterator insert_pos;
	if (!parent.isValid()) {
		// Add new preset
		auto [new_id, it] = newPresetId();
		int dest_row = distance(_presets.begin(), it);
		beginInsertRows({}, dest_row, dest_row);
		it = emplacePreset(it, new_id);
		endInsertRows();
		dest_preset = createIndex(dest_row, 0, nullptr);
		insert_pos = (*it)->properties.begin();
	}
	else {
		// Insert in existing preset
		dest_preset = parent;
		auto &dest = *_presets.at(parent.row());
		if (row >= 0)
			insert_pos = dest.properties.begin() + row;
		else
			insert_pos = dest.properties.end();
	}
	auto &dest = *_presets.at(dest_preset.row());
	// Move presets
	for (const auto &index: data->indexes()) {
		if (!index.isValid())
			continue;
		int insert_row = distance(dest.properties.begin(), insert_pos);
		if (auto src_preset = static_cast<Preset *>(index.internalPointer())) {
			// Move single work detail
			if (src_preset == &dest) {
				// Move from same preset
				beginMoveRows(dest_preset, index.row(), index.row(),
						dest_preset, insert_row);
				auto moved_wd = dest.properties.begin() + index.row();
				if (insert_row > index.row())
					std::rotate(moved_wd, next(moved_wd), insert_pos);
				else {
					std::rotate(insert_pos, moved_wd, next(moved_wd));
					++insert_pos;
				}
				endMoveRows();
			}
			else {
				// Move from different preset
				beginMoveRows(index.parent(), index.row(), index.row(),
						dest_preset, insert_row);
				auto moved_wd = src_preset->properties.begin() + index.row();
				insert_pos = dest.properties.insert(insert_pos, std::move(*moved_wd));
				++insert_pos;
				src_preset->properties.erase(moved_wd);
				src_preset->save();
				endMoveRows();
			}
		}
		else {
			auto &moved_preset = *_presets.at(index.row());
			auto wd_count = moved_preset.properties.size();
			// Move all preset work details
			beginMoveRows(index, 0, wd_count-1,
					dest_preset, insert_row);
			insert_pos = dest.properties.insert(insert_pos,
					std::move_iterator(moved_preset.properties.begin()),
					std::move_iterator(moved_preset.properties.end()));
			insert_pos += wd_count;
			moved_preset.properties.clear();
			endMoveRows();
			// Remove empty preset
			beginRemoveRows({}, index.row(), index.row());
			erasePreset(index);
			endRemoveRows();
		}
	}
	dest.save();
	dataChanged(dest_preset, dest_preset);
	return true;
}

std::span<const WorkDetail::Properties> WorkDetailPresets::properties(const QModelIndex &index) const
{
	if (!index.isValid())
		return {};
	else if (auto preset = static_cast<const Preset *>(index.internalPointer()))
		return {&preset->properties.at(index.row()), 1};
	else
		return _presets.at(index.row())->properties;
}

bool WorkDetailPresets::isWorkDetail(const QModelIndex &index) const
{
	return index.isValid() && index.internalPointer() != nullptr;
}

const WorkDetail::Properties *WorkDetailPresets::workdetail(const QModelIndex &index) const
{
	if (!index.isValid())
		return nullptr;
	else if (auto preset = static_cast<const Preset *>(index.internalPointer()))
		return &preset->properties.at(index.row());
	else
		return nullptr;
}

const WorkDetailPresets::Preset *WorkDetailPresets::preset(const QModelIndex &index) const
{
	if (!index.isValid() || index.internalPointer() != nullptr)
		return nullptr;
	return _presets.at(index.row()).get();
}

bool WorkDetailPresets::isUserWritable(const QModelIndex &index) const
{
	if (!index.isValid())
		return false;
	auto preset = index.internalPointer() != nullptr
		? static_cast<const Preset *>(index.internalPointer())
		: _presets.at(index.row()).get();
	return preset->file_info.isWritable();
}


bool WorkDetailPresets::setProperties(const QModelIndex &index, WorkDetail::Properties &&properties)
{
	if (!index.isValid())
		return false;
	if (auto preset = static_cast<Preset *>(index.internalPointer())) {
		if (!preset->file_info.isWritable())
			return false;
		preset->properties.at(index.row()) = std::move(properties);
		preset->save();
		dataChanged(index, index);
		return true;
	}
	else
		return false;
}

bool WorkDetailPresets::add(std::vector<WorkDetail::Properties> &&properties, const QModelIndex &index)
{
	if (!index.isValid() || index.internalPointer() != nullptr)
		return false;
	auto preset = _presets.at(index.row()).get();
	if (!preset->file_info.isWritable())
		return false;
	beginInsertRows(index, preset->properties.size(), preset->properties.size()+properties.size()-1);
	preset->properties.insert(preset->properties.end(),
			std::move_iterator(properties.begin()),
			std::move_iterator(properties.end()));
	preset->save();
	endInsertRows();
	return true;
}

bool WorkDetailPresets::add(std::vector<WorkDetail::Properties> &&properties, const QString &name)
{
	auto it = std::ranges::lower_bound(_presets, name, {}, &Preset::id);
	if (it != _presets.end() && (*it)->id == name)
		return false;
	int row = distance(_presets.begin(), it);
	beginInsertRows({}, row, row);
	it = emplacePreset(it, name, std::move(properties));
	(*it)->save();
	endInsertRows();
	return true;
}

bool WorkDetailPresets::remove(const QModelIndex &index)
{
	if (!index.isValid())
		return false;
	if (auto parent = static_cast<Preset *>(index.internalPointer())) {
		if (!parent->file_info.isWritable())
			return false;
		beginRemoveRows(index.parent(), index.row(), index.row());
		parent->properties.erase(parent->properties.begin()+index.row());
		parent->save();
		endRemoveRows();
	}
	else {
		auto &preset = *_presets.at(index.row());
		if (!preset.file_info.isWritable())
			return false;
		beginRemoveRows(index.parent(), index.row(), index.row());
		erasePreset(index);
		endRemoveRows();
	}
	return false;
}

std::pair<QString, std::vector<std::unique_ptr<WorkDetailPresets::Preset>>::iterator> WorkDetailPresets::newPresetId()
{
	QString base_name = tr("New preset");
	QString new_id = base_name;
	int count = 1;
	auto it = std::ranges::lower_bound(_presets, new_id, {}, &Preset::id);
	while (it != _presets.end() && (*it)->id == new_id) {
		new_id = QString("%1 %2")
			.arg(base_name)
			.arg(++count);
		it = std::ranges::lower_bound(_presets, new_id, {}, &Preset::id);
	}
	return {new_id, it};
}

template <typename... Args>
std::vector<std::unique_ptr<WorkDetailPresets::Preset>>::iterator WorkDetailPresets::emplacePreset(decltype(_presets)::iterator pos, Args &&...args)
{
	pos = _presets.emplace(pos, std::make_unique<Preset>(std::forward<Args>(args)...));
	// Update rows
	(*pos)->row = distance(_presets.begin(), pos);
	for (auto it = next(pos); it != _presets.end(); ++it)
		++(*it)->row;
	return pos;
}

void WorkDetailPresets::erasePreset(const QModelIndex &index)
{
	auto it = _presets.begin() + index.row();
	if (!QFile::remove((*it)->file_info.filePath()))
		qCCritical(WorkDetailLog) << "Failed to remove"
			<< (*it)->file_info.absoluteFilePath();
	else
		qCInfo(WorkDetailLog) << "Removed preset"
			<< (*it)->file_info.absoluteFilePath();
	it = _presets.erase(it);
	// Update rows
	for (; it != _presets.end(); ++it)
		--(*it)->row;
}
