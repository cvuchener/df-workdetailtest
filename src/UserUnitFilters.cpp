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

#include "UserUnitFilters.h"

#include "Application.h"
#include "ScriptManager.h"
#include "Unit.h"
#include "UnitScriptWrapper.h"

bool UnitNameFilter::operator()(const Unit &unit) const
{
	return unit.displayName().contains(text);
}

bool UnitNameRegexFilter::operator()(const Unit &unit) const
{
	return regex.match(unit.displayName()).hasMatch();
}

bool ScriptedUnitFilter::operator()(const Unit &unit) const
{
	auto result = script.call({Application::scripts().makeUnit(unit)});
	if (result.isError()) {
		qCCritical(ScriptLog) << "Filter script failed:" << result.property("message").toString();
		return false;
	}
	return result.toBool();
}

const std::vector<std::pair<const char *, UnitFilter>> BuiltinUnitFilters = {
	{QT_TRANSLATE_NOOP("BuiltinUnitFilters", "FortControlled"), &Unit::isFortControlled},
	{QT_TRANSLATE_NOOP("BuiltinUnitFilters", "Workers"), &Unit::canAssignWork},
};

UserUnitFilters::UserUnitFilters(QObject *parent):
	QAbstractListModel(parent)
{
}

UserUnitFilters::~UserUnitFilters()
{
}

int UserUnitFilters::rowCount(const QModelIndex &parent) const
{
	return _filters.size();
}

QVariant UserUnitFilters::data(const QModelIndex &index, int role) const
{
	switch (role) {
	case Qt::DisplayRole:
		return _filters[index.row()].first;
	default:
		return {};
	}
}

bool UserUnitFilters::removeRows(int row, int count, const QModelIndex &parent)
{
	beginRemoveRows(parent, row, row+count-1);
	_filters.erase(next(_filters.begin(), row), next(_filters.begin(), row+count));
	endRemoveRows();
	invalidated();
	return true;
}

void UserUnitFilters::addFilter(const QString &name, UnitFilter filter)
{
	beginInsertRows({}, _filters.size(), _filters.size());
	_filters.emplace_back(name, std::move(filter));
	endInsertRows();
	invalidated();
}

void UserUnitFilters::clear()
{
	beginRemoveRows({}, 0, _filters.size()-1);
	_filters.clear();
	endRemoveRows();
	invalidated();
}

bool UserUnitFilters::operator()(const Unit &unit) const
{
	for (const auto &[name, filter]: _filters)
		if (filter && !filter(unit))
			return false;
	return !_temporary_filter || _temporary_filter(unit);
}
