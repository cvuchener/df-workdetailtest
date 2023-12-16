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

#include "ScriptManager.h"

#include "StandardPaths.h"
#include "UnitScriptWrapper.h"

Q_LOGGING_CATEGORY(ScriptLog, "script");

static void printError(const QJSValue &error)
{
	qCCritical(ScriptLog) << QString("%1:%2: %3: %4")
		.arg(error.property("fileName").toString())
		.arg(error.property("lineNumber").toInt())
		.arg(error.property("name").toString())
		.arg(error.property("message").toString());
}

ScriptManager::ScriptManager():
	_test_dummy(_js.newQObject(new UnitScriptWrapper))
{
	_js.installExtensions(QJSEngine::ConsoleExtension);
	addEnumValues("profession", df::profession::AllValues);
	QStringList name_filter = {"*.js"};
	for (QDir data_dir: StandardPaths::data_locations()) {
		QDir dir = data_dir.filePath("unit_filters");
		for (const auto &fi: dir.entryInfoList(name_filter, QDir::Files)) {
			QFile file(fi.filePath());
			if (!file.open(QIODeviceBase::ReadOnly)) {
				qCCritical(ScriptLog) << "Failed to open" << fi.filePath();
				continue;
			}
			QTextStream stream(&file);
			auto result = _js.evaluate(stream.readAll(), fi.filePath());
			if (result.isError()) {
				printError(result);
				continue;
			}
			if (!result.isCallable()) {
				qCCritical(ScriptLog) << "Script" << fi.filePath() << "is not callable";
				continue;
			}
			auto test_result = result.call({_test_dummy});
			if (test_result.isError()) {
				printError(test_result);
				continue;
			}
			qCInfo(ScriptLog) << "Added script" << fi.baseName()
				<< "from" << fi.absoluteFilePath();
			_scripts.emplace_back(fi.baseName(), std::move(result));
		}
	}
}

ScriptManager::~ScriptManager()
{
}

QJSValue ScriptManager::makeUnit(const Unit &unit)
{
	return _js.newQObject(new UnitScriptWrapper(unit));
}

QJSValue ScriptManager::makeScript(const QString &expression)
{
	auto script = _js.evaluate(QString("(u) => Boolean(%1)").arg(expression));
	if (script.isError())
		return script;
	auto test_result = script.call({_test_dummy});
	if (test_result.isError())
		return test_result;
	return script;
}

template <std::ranges::input_range R>
void ScriptManager::addEnumValues(const QString &name, R &&values)
{
	auto object = _js.newObject();
	for (auto value: values)
		object.setProperty(QString::fromLocal8Bit(to_string(value)), value);
	_js.globalObject().setProperty(name, object);
}
