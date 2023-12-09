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

#ifndef USER_UNIT_FILTERS_H
#define USER_UNIT_FILTERS_H

#include <QAbstractListModel>
#include <QRegularExpression>
#include <QJSValue>

class Unit;

using UnitFilter = std::function<bool(const Unit &)>;

struct AllUnits
{
	bool operator()(const Unit &) const noexcept { return true; }
};

struct UnitNameFilter
{
	QString text;
	bool operator()(const Unit &) const;
};

struct UnitNameRegexFilter
{
	QRegularExpression regex;
	bool operator()(const Unit &) const;
};

struct ScriptedUnitFilter
{
	QJSValue script;
	bool operator()(const Unit &) const;
};

extern const std::vector<std::pair<const char *, UnitFilter>> BuiltinUnitFilters;

class UserUnitFilters: public QAbstractListModel
{
	Q_OBJECT
public:
	UserUnitFilters(QObject *parent = nullptr);
	UserUnitFilters(const UserUnitFilters &, QObject *parent = nullptr);
	~UserUnitFilters() override;

	int rowCount(const QModelIndex &parent = {}) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	bool removeRows(int row, int count, const QModelIndex &parent = {}) override;

	void addFilter(const QString &name, UnitFilter filter);
	void clear();

	enum class TemporaryType {
		Simple,
		Regex,
		Script,
	};

	std::pair<TemporaryType, QString> temporaryFilter() const {
		return {_temporary_type, _temporary_text};
	}
	void setTemporaryFilter(TemporaryType type, const QString &text);

	bool operator()(const Unit &) const;

signals:
	void invalidated();

private:
	std::vector<std::pair<QString, UnitFilter>> _filters;
	UnitFilter _temporary_filter;
	TemporaryType _temporary_type;
	QString _temporary_text;
};

#endif
