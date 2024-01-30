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

#include "Application.h"
#include "StandardPaths.h"
#include "UnitScriptWrapper.h"
#include "LogCategory.h"

#include <QMetaProperty>
#include <QMetaMethod>

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
	{ // Add unit properties to properties model
		auto item = new QStandardItem("u");
		const auto &meta_unit = UnitScriptWrapper::staticMetaObject;
		for (int i = meta_unit.propertyOffset(); i < meta_unit.propertyCount(); ++i) {
			auto prop = meta_unit.property(i);
			auto prop_item = new QStandardItem();
			prop_item->setData(prop.name(), Qt::EditRole);
			int doc = meta_unit.indexOfClassInfo((QByteArray("doc:") + prop.name()).data());
			if (doc == -1) {
				prop_item->setData(QString("property %1: %2")
						.arg(prop.name())
						.arg(prop.typeName()),
						Qt::StatusTipRole);
			}
			else {
				prop_item->setData(QString("property %1: %2 – %3")
						.arg(prop.name())
						.arg(prop.typeName())
						.arg(meta_unit.classInfo(doc).value()),
						Qt::StatusTipRole);
			}
			item->appendRow(prop_item);
		}
		for (int i = meta_unit.methodOffset(); i < meta_unit.methodCount(); ++i) {
			auto method = meta_unit.method(i);
			if (method.access() != QMetaMethod::Public)
				continue;
			if (method.methodType() == QMetaMethod::Constructor
					|| method.methodType() == QMetaMethod::Signal)
				continue;
			auto method_item = new QStandardItem();
			method_item->setData(method.name(), Qt::EditRole);
			QStringList params_doc;
			for (int j = 0; j < method.parameterCount(); ++j)
				params_doc.push_back(QString("%1: %2")
						.arg(method.parameterNames().at(j))
						.arg(method.parameterTypeName(j)));
			int doc = meta_unit.indexOfClassInfo((QByteArray("doc:") + method.name()).data());
			if (doc == -1) {
				method_item->setData(QString("function %1(%2): %3")
						.arg(method.name())
						.arg(params_doc.join(", "))
						.arg(method.returnMetaType().name()),
						Qt::StatusTipRole);
			}
			else {
				method_item->setData(QString("function %1(%2): %3 – %4")
						.arg(method.name())
						.arg(params_doc.join(", "))
						.arg(method.returnMetaType().name())
						.arg(meta_unit.classInfo(doc).value()),
						Qt::StatusTipRole);
			}
			item->appendRow(method_item);
		}
		_properties_model.appendRow(item);
	}
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
	_properties_model.setSortRole(Qt::EditRole);
	_properties_model.sort(0);
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
	auto item = new QStandardItem(name);
	for (auto value: values) {
		auto value_name = QString::fromLocal8Bit(to_string(value));
		object.setProperty(value_name, value);
		item->appendRow(new QStandardItem(value_name));
	}
	_js.globalObject().setProperty(name, object);
	_properties_model.appendRow(item);
}

ScriptPropertiesCompleter::ScriptPropertiesCompleter(QObject *parent):
	QCompleter(Application::scripts().propertiesModel(), parent)
{
	setModelSorting(QCompleter::CaseInsensitivelySortedModel);
}

ScriptPropertiesCompleter::~ScriptPropertiesCompleter()
{
}

QString ScriptPropertiesCompleter::pathFromIndex(const QModelIndex &index) const
{
	QString path = index.data().toString();
	for (auto parent = index.parent(); parent.isValid(); parent = parent.parent()) {
		path.prepend('.');
		path.prepend(parent.data().toString());
	}
	return path;
}

QStringList ScriptPropertiesCompleter::splitPath(const QString &path) const
{
	return path.split('.');
}
