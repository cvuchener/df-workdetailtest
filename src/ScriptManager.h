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
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(ScriptLog);

class Unit;

class ScriptManager
{
public:
	ScriptManager();
	~ScriptManager();

	const std::vector<std::pair<QString, QJSValue>> &filters() const { return _scripts; }

	QJSValue makeUnit(const Unit &unit);
	QJSValue makeScript(const QString &expression);

private:
	QJSEngine _js;
	QJSValue _test_dummy;
	std::vector<std::pair<QString, QJSValue>> _scripts;
};

#endif
