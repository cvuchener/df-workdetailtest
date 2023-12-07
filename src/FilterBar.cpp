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

#include "FilterBar.h"

#include <QToolButton>
#include <QMenu>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>

#include "Application.h"
#include "ScriptManager.h"
#include "UnitFilterProxyModel.h"
#include "Unit.h"

LabelAction::LabelAction(const QString &text, QObject *parent):
	QWidgetAction(parent)
{
	setText(text);
}

LabelAction::~LabelAction()
{
}

QWidget *LabelAction::createWidget(QWidget *parent)
{
	return new QLabel(text(), parent);
}

FilterTypeAction::FilterTypeAction(QObject *parent):
	QWidgetAction(parent),
	_type(FilterType::Simple)
{
}

FilterTypeAction::~FilterTypeAction()
{
}

QWidget *FilterTypeAction::createWidget(QWidget *parent)
{
	auto cb = new QComboBox(parent);
	cb->addItem(tr("Simple"), QVariant::fromValue(FilterType::Simple));
	cb->addItem(tr("Regex"), QVariant::fromValue(FilterType::Regex));
	cb->addItem(tr("Script"), QVariant::fromValue(FilterType::Script));
	connect(cb, &QComboBox::currentIndexChanged, this, [this, cb](int index) {
		_type = cb->itemData(index).value<FilterType>();
		currentChanged(_type);
	});
	cb->setCurrentIndex(cb->findData(QVariant::fromValue(_type)));
	return cb;
}

FilterTextAction::FilterTextAction(QObject *parent):
	QWidgetAction(parent)
{
}

FilterTextAction::~FilterTextAction()
{
}

QWidget *FilterTextAction::createWidget(QWidget *parent)
{
	auto line = new QLineEdit(parent);
	connect(line, &QLineEdit::textChanged, this, [this](const QString &text) {
		_text = text;
		textChanged(text);
	});
	line->setText(_text);
	return line;
}

RemoveFilterAction::RemoveFilterAction(const QString &text, QObject *parent):
	QWidgetAction(parent)
{
	setText(text);
	setIcon(QIcon::fromTheme("edit-delete"));
}

RemoveFilterAction::~RemoveFilterAction()
{
}

QWidget *RemoveFilterAction::createWidget(QWidget *parent)
{
	auto button = new QToolButton(parent);
	button->setDefaultAction(this);
	button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	return button;
}

FilterMenuAction::FilterMenuAction(QObject *parent):
	QWidgetAction(parent)
{
	setText(tr("Add"));
	setIcon(QIcon::fromTheme("list-add"));
}

FilterMenuAction::~FilterMenuAction()
{
}

QWidget *FilterMenuAction::createWidget(QWidget *parent)
{
	auto button = new QToolButton(parent);
	button->setDefaultAction(this);
	auto menu = new QMenu(button);
	auto make_add_filter_action = [&, this](const QString &name, auto data) {
		auto action = new QAction(menu);
		action->setText(name);
		action->setData(QVariant::fromValue(data));
		connect(action, &QAction::triggered, this, [this, action]() {
				triggered(action);
		});
		menu->addAction(action);
	};
	make_add_filter_action(tr("Workers"), BuiltinFilter::Workers);
	menu->addSeparator();
	for (const auto &[name, filter]: Application::scripts().filters())
		make_add_filter_action(name, filter);
	button->setMenu(menu);
	button->setPopupMode(QToolButton::InstantPopup);
	return button;
}


FilterBar::FilterBar(QWidget *parent):
	QToolBar(tr("Filters"), parent),
	_filters(nullptr)
{
	addAction(new LabelAction(tr("Filters: "), this));
	_filter_type = new FilterTypeAction(this);
	addAction(_filter_type);
	connect(_filter_type, &FilterTypeAction::currentChanged, this, [this](FilterType type) {
		filterChanged(type, _filter_text->text());
	});
	_filter_text = new FilterTextAction(this);
	addAction(_filter_text);
	connect(_filter_text, &FilterTextAction::textChanged, this, [this](const QString &text) {
		filterChanged(_filter_type->currentType(), text);
	});
	_add_filter_menu = new FilterMenuAction(this);
	_add_filter_menu->setEnabled(_filters != nullptr);
	addAction(_add_filter_menu);
	connect(_add_filter_menu, &FilterMenuAction::triggered, this, [this](const QAction *action) {
		Q_ASSERT(_filters);
		if (action->data().metaType() == QMetaType::fromType<QJSValue>()) {
			_filters->addFilter(action->text(), ScriptedUnitFilter{action->data().value<QJSValue>()});
		}
		else if (action->data().metaType() == QMetaType::fromType<BuiltinFilter>()) {
			switch (action->data().value<BuiltinFilter>()) {
			case BuiltinFilter::Workers:
				_filters->addFilter(action->text(), &Unit::canAssignWork);
			}
		}
		else
			qFatal() << "Invalid filter";
	});
}

FilterBar::~FilterBar()
{
}

void FilterBar::setFilterModel(UnitFilterList *model)
{
	if (_filters)
		disconnect(_filters);
	_remove_filter_actions.clear();
	_filters = model;
	_add_filter_menu->setEnabled(_filters != nullptr);
	if (_filters) {
		connect(_filters, &QAbstractItemModel::rowsInserted, this, &FilterBar::filterInserted);
		connect(_filters, &QAbstractItemModel::rowsAboutToBeRemoved, this, &FilterBar::filterRemoved);
		insertFilterButtons(0, _filters->rowCount()-1);
	}
}

void FilterBar::insertFilterButtons(int first, int last)
{
	QAction *before = unsigned(first) < _remove_filter_actions.size()
		? _remove_filter_actions[first].get()
		: _add_filter_menu;
	for (int i = first; i <= last; ++i) {
		auto index = _filters->index(i);
		auto &action = _remove_filter_actions.emplace_back(std::make_unique<RemoveFilterAction>(index.data().toString()));
		connect(action.get(), &QAction::triggered, this, [this, index = QPersistentModelIndex(index)]() {
			_filters->removeRows(index.row(), 1, index.parent());
		});
		insertAction(before, action.get());
	}
	// Move all the new elements added at the back to their proper position
	std::rotate(next(_remove_filter_actions.begin(), first),
		prev(_remove_filter_actions.end(), last-first+1),
		_remove_filter_actions.end());
}

void FilterBar::filterInserted(const QModelIndex &parent, int first, int last)
{
	insertFilterButtons(first, last);
}

void FilterBar::filterRemoved(const QModelIndex &parent, int first, int last)
{
	_remove_filter_actions.erase(
			next(_remove_filter_actions.begin(), first),
			next(_remove_filter_actions.begin(), last+1)
	);
}
