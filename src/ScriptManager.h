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

#ifndef SCRIPT_MANAGER_H
#define SCRIPT_MANAGER_H

#include <QJSEngine>
#include <QStandardItemModel>
#include <QCompleter>

class Unit;

class ScriptManager
{
public:
	ScriptManager();
	~ScriptManager();

	const std::vector<std::pair<QString, QJSValue>> &filters() const { return _scripts; }

	QJSValue makeUnit(const Unit &unit);
	QJSValue makeScript(const QString &expression);

	QAbstractItemModel *propertiesModel() { return &_properties_model; }

private:
	template <std::ranges::input_range R>
	void addEnumValues(const QString &name, R &&values);

	QJSEngine _js;
	QJSValue _test_dummy;
	std::vector<std::pair<QString, QJSValue>> _scripts;
	QStandardItemModel _properties_model;
};

class ScriptPropertiesCompleter: public QCompleter
{
	Q_OBJECT
public:
	ScriptPropertiesCompleter(QObject *parent = nullptr);
	~ScriptPropertiesCompleter() override;

	QString pathFromIndex(const QModelIndex &index) const override;
	QStringList splitPath(const QString &path) const override;
};

#endif
