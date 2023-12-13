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

#include "LaborModel.h"

static constexpr std::size_t categoryToIndex(df::unit_labor_category_t category)
{
	return -static_cast<int>(category)-1;
}

static constexpr df::unit_labor_category_t indexToCategory(int index)
{
	return static_cast<df::unit_labor_category_t>(-index-1);
}

LaborModel::LaborModel(QObject *parent):
	QAbstractItemModel(parent)
{
	for (int i = 0; i < df::unit_labor::Count; ++i) {
		auto labor = static_cast<df::unit_labor_t>(i);
		_categories.at(categoryToIndex(category(labor))).push_back(labor);
	}
}

LaborModel::~LaborModel()
{
}

static constexpr quintptr NoParent = -1;

QModelIndex LaborModel::index(int row, int column, const QModelIndex &parent) const
{
	if (parent.isValid())
		return createIndex(row, column, parent.row());
	else
		return createIndex(row, column, NoParent);
}

QModelIndex LaborModel::parent(const QModelIndex &index) const
{
	if (index.internalId() == NoParent)
		return {};
	else
		return createIndex(index.internalId(), 0, NoParent);
}

int LaborModel::rowCount(const QModelIndex &parent) const
{
	if (_group_by_category) {
		if (parent.isValid()) {
			if (parent.internalId() == NoParent)
				return _categories.at(parent.row()).size();
			else
				return 0;
		}
		else
			return _categories.size();
	}
	else {
		if (parent.isValid())
			return 0;
		else
			return _labors.size();
	}
}

int LaborModel::columnCount(const QModelIndex &) const
{
	return 1;
}

std::variant<df::unit_labor_t, df::unit_labor_category_t> LaborModel::parseIndex(const QModelIndex &index) const
{
	if (_group_by_category) {
		if (index.internalId() == NoParent)
			return indexToCategory(index.row());
		else
			return _categories.at(index.internalId()).at(index.row());
	}
	else
		return static_cast<df::unit_labor_t>(index.row());
}

QVariant LaborModel::data(df::unit_labor_t labor, int role) const
{
	switch (role) {
	case Qt::DisplayRole:
		if (auto name = caption(labor); !name.empty())
			return QString::fromLocal8Bit(name);
		else
			return QString::fromLocal8Bit(to_string(labor));
	case Qt::CheckStateRole:
		return _labors.at(labor) ? Qt::Checked : Qt::Unchecked;
	default:
		return {};
	}
}

QVariant LaborModel::data(df::unit_labor_category_t category, int role) const
{
	int index = categoryToIndex(category);
	auto is_enabled = [this](df::unit_labor_t labor) {
		return _labors.at(labor);
	};
	switch (role) {
	case Qt::DisplayRole:
		switch (category) {
		case df::unit_labor_category::None: return tr("No category");
		case df::unit_labor_category::Woodworking: return tr("Woodworking");
		case df::unit_labor_category::Stoneworking: return tr("Stoneworking");
		case df::unit_labor_category::Hunting: return tr("Hunting");
		case df::unit_labor_category::Healthcare: return tr("Healthcare");
		case df::unit_labor_category::Farming: return tr("Farming");
		case df::unit_labor_category::Fishing: return tr("Fishing");
		case df::unit_labor_category::Metalsmithing: return tr("Metalsmithing");
		case df::unit_labor_category::Jewelry: return tr("Jewelry");
		case df::unit_labor_category::Crafts: return tr("Crafts");
		case df::unit_labor_category::Engineering: return tr("Engineering");
		case df::unit_labor_category::Hauling: return tr("Hauling");
		case df::unit_labor_category::Other: return tr("Other");
		default: return {};
		}
	case Qt::CheckStateRole:
		if (std::ranges::all_of(_categories.at(index), is_enabled))
			return Qt::Checked;
		else if (std::ranges::none_of(_categories.at(index), is_enabled))
			return Qt::Unchecked;
		else
			return Qt::PartiallyChecked;
	default:
		return {};
	}
}

QVariant LaborModel::data(const QModelIndex &index, int role) const
{
	return visit([this, role](auto item) { return data(item, role); }, parseIndex(index));
}

bool LaborModel::setData(const QModelIndex &index, df::unit_labor_t labor, bool enable)
{
	_labors.at(labor) = enable;
	dataChanged(index, index);
	if (_group_by_category) {
		auto category_index = index.parent();
		dataChanged(category_index, category_index);
	}
	return true;
}

bool LaborModel::setData(const QModelIndex &index, df::unit_labor_category_t category, bool enable)
{
	const auto &category_labors = _categories.at(index.row());
	for (auto labor: category_labors)
		_labors.at(labor) = enable;
	dataChanged(index, index);
	dataChanged(createIndex(0, 0, index.row()), createIndex(category_labors.size()-1, 0, index.row()));
	return true;
}

bool LaborModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role != Qt::CheckStateRole)
		return false;
	return visit(
		[this, &index, enable = value.value<Qt::CheckState>() == Qt::Checked](auto item) {
			return setData(index, item, enable);
		},
		parseIndex(index));

}

Qt::ItemFlags LaborModel::flags(const QModelIndex &index) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
}

void LaborModel::setGroupByCategory(bool enabled)
{
	layoutAboutToBeChanged();
	_group_by_category = enabled;
	layoutChanged();
}

void LaborModel::setLabors(std::span<const bool, df::unit_labor::Count> labors)
{
	std::ranges::copy(labors, _labors.begin());
	if (_group_by_category)
		dataChanged(index(0), index(_categories.size()-1));
	else
		dataChanged(index(0), index(_labors.size()-1));
}
