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

#include "PreferencesModel.h"

#include "Unit.h"
#include "DwarfFortressData.h"
#include "DataRole.h"
#include "Preference.h"

using namespace UnitDetails;

PreferencesModel::PreferencesModel(const DwarfFortressData &df, QObject *parent):
	UnitDataModel(df, parent)
{
}

PreferencesModel::~PreferencesModel()
{
}

int PreferencesModel::rowCount(const QModelIndex &parent) const
{
	if (_u)
		if (auto s = (*_u)->current_soul.get())
			return s->preferences.size();
	return 0;
}

int PreferencesModel::columnCount(const QModelIndex &parent) const
{
	return static_cast<int>(Column::Count);
}

QVariant PreferencesModel::data(const QModelIndex &index, int role) const
{
	auto pref = (*_u)->current_soul->preferences.at(index.row()).get();
	switch (static_cast<Column>(index.column())) {
	case Column::Description:
		switch (role) {
		case Qt::DisplayRole:
		case DataRole::SortRole:
			return Preference::toString(_df, *pref);
		default:
			return {};
		}
	case Column::Category:
		switch (role) {
		case Qt::DisplayRole:
		case DataRole::SortRole:
			return Preference::toString(pref->type);
		default:
			return {};
		}
	default:
		return {};
	}
}

QVariant PreferencesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal)
		return {};
	if (role != Qt::DisplayRole)
		return {};
	switch (static_cast<Column>(section)) {
	case Column::Description:
		return tr("Description");
	case Column::Category:
		return tr("Category");
	default:
		return {};
	}
}
