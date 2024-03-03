/*
 * Copyright 2024 Clement Vuchener
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

#include "Model.h"

#include "df/types.h"
#include "Preference.h"
#include "ObjectList.h"
#include "Unit.h"

using namespace Preferences;

Model::Model(std::shared_ptr<const DwarfFortressData> df, QObject *parent):
	QAbstractTableModel(parent),
	_df(std::move(df))
{
	connect(_df.get(), &DwarfFortressData::gameDataUpdated, this, &Model::rebuild);
}

Model::~Model()
{
}

int Model::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;
	else
		return _preferences.size();
}

int Model::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;
	else
		return static_cast<int>(Columns::Count);
}

QVariant Model::data(const QModelIndex &index, int role) const
{
	const auto &[pref, count] = _preferences.at(index.row());
	switch (static_cast<Columns>(index.column())) {
	case Columns::Type:
		switch (role) {
		case Qt::DisplayRole:
			return Preference::toString(pref.type);
		default:
			return {};
		}
	case Columns::Name:
		switch (role) {
		case Qt::DisplayRole:
			return Preference::toString(*_df, pref);
		default:
			return {};
		}
	case Columns::UnitCount:
		switch (role) {
		case Qt::DisplayRole:
			return count;
		case Qt::TextAlignmentRole:
			return Qt::AlignRight;
		default:
			return {};
		}
	default:
		return {};
	}
}

QVariant Model::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal)
		return {};
	if (role != Qt::DisplayRole)
		return {};
	switch (static_cast<Columns>(section)) {
	case Columns::Type:
		return tr("Type");
	case Columns::Name:
		return tr("Name");
	case Columns::UnitCount:
		return tr("Count");
	default:
		return {};
	}
}

void Model::rebuild()
{
	beginResetModel();
	_preferences.clear();
	for (const auto &unit: *_df->units) {
		if (!unit.isFortControlled() || !unit->current_soul)
			continue;
		for (const auto &pref: unit->current_soul->preferences) {
			auto it = std::ranges::lower_bound(_preferences, *pref, {}, &decltype(_preferences)::value_type::first);
			if (it != _preferences.end() && it->first == *pref)
				++it->second;
			else
				_preferences.emplace(it, *pref, 1);
		}
	}
	endResetModel();
}
