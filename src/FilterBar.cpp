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

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QToolButton>
#include <QWidgetAction>

#include "Application.h"
#include "ScriptManager.h"
#include "UserUnitFilters.h"
#include "Unit.h"

enum class FilterType {
	Simple,
	Regex,
	Script,
};

struct FilterBar::Ui
{
	QWidgetAction *filter_type_action;
	QComboBox *filter_type_cb;
	QWidgetAction *filter_text_action;
	QLineEdit *filter_text;
	std::vector<std::unique_ptr<QAction>> remove_filter_actions;
	QWidgetAction *add_filter_action;
	QMenu *add_filter_menu;

	void setupUi(QToolBar *parent)
	{
		parent->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

		auto title_action = new QWidgetAction(parent);
		title_action->setDefaultWidget(new QLabel(tr("Filters: ")));
		parent->addAction(title_action);

		filter_type_action = new QWidgetAction(parent);
		filter_type_cb = new QComboBox;
		filter_type_cb->addItem(tr("Simple"), QVariant::fromValue(FilterType::Simple));
		filter_type_cb->addItem(tr("Regex"), QVariant::fromValue(FilterType::Regex));
		filter_type_cb->addItem(tr("Script"), QVariant::fromValue(FilterType::Script));
		filter_type_action->setDefaultWidget(filter_type_cb);
		parent->addAction(filter_type_action);

		filter_text_action = new QWidgetAction(parent);
		filter_text = new QLineEdit;
		filter_text->setClearButtonEnabled(true);
		filter_text_action->setDefaultWidget(filter_text);
		parent->addAction(filter_text_action);

		add_filter_action = new QWidgetAction(parent);
		auto add_filter_button = new QToolButton;
		add_filter_menu = new QMenu(add_filter_button);
		add_filter_button->setMenu(add_filter_menu);
		add_filter_action->setText(tr("Add"));
		add_filter_action->setIcon(QIcon::fromTheme("list-add"));
		add_filter_button->setDefaultAction(add_filter_action);
		add_filter_button->setToolButtonStyle(Qt::ToolButtonIconOnly);
		add_filter_button->setPopupMode(QToolButton::InstantPopup);
		add_filter_action->setDefaultWidget(add_filter_button);
		parent->addAction(add_filter_action);
	}
};

FilterBar::FilterBar(QWidget *parent):
	QToolBar(tr("Filters"), parent),
	_ui(std::make_unique<FilterBar::Ui>()),
	_filters(std::make_shared<UserUnitFilters>())
{
	connect(_filters.get(), &QAbstractItemModel::rowsInserted, this, &FilterBar::filterInserted);
	connect(_filters.get(), &QAbstractItemModel::rowsAboutToBeRemoved, this, &FilterBar::filterRemoved);

	_ui->setupUi(this);
	connect(_ui->filter_type_cb, &QComboBox::currentIndexChanged, this, [this](int index) {
		updateFilterUi();
		updateTemporaryFilter();
	});
	connect(_ui->filter_text, &QLineEdit::textChanged, this, [this](const QString &text) {
		updateTemporaryFilter();
	});
	updateFilterUi();

	enum class BuiltinFilter {
		Workers
	};
	auto make_add_filter_action = [&, this](const QString &name, auto data) {
		auto action = new QAction(_ui->add_filter_menu);
		action->setText(name);
		action->setData(QVariant::fromValue(data));
		connect(action, &QAction::triggered, this, [this, action]() {
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
		_ui->add_filter_menu->addAction(action);
	};
	make_add_filter_action(tr("Workers"), BuiltinFilter::Workers);
	_ui->add_filter_menu->addSeparator();
	for (const auto &[name, filter]: Application::scripts().filters())
		make_add_filter_action(name, filter);
}

FilterBar::~FilterBar()
{
}

void FilterBar::setFilters(std::shared_ptr<UserUnitFilters> filters)
{
	Q_ASSERT(filters);

	// clean up old filters
	disconnect(_filters.get());
	_ui->remove_filter_actions.clear();

	_filters = std::move(filters);
	// connect and setup new filters
	connect(_filters.get(), &QAbstractItemModel::rowsInserted, this, &FilterBar::filterInserted);
	connect(_filters.get(), &QAbstractItemModel::rowsAboutToBeRemoved, this, &FilterBar::filterRemoved);
	insertFilterButtons(0, _filters->rowCount()-1);
}

void FilterBar::insertFilterButtons(int first, int last)
{
	QAction *before = unsigned(first) < _ui->remove_filter_actions.size()
		? _ui->remove_filter_actions[first].get()
		: _ui->add_filter_action;
	for (int i = first; i <= last; ++i) {
		auto index = _filters->index(i);
		auto &action = _ui->remove_filter_actions.emplace_back(std::make_unique<QAction>(this));
		action->setText(index.data().toString());
		action->setIcon(QIcon::fromTheme("edit-delete"));
		connect(action.get(), &QAction::triggered, this, [this, index = QPersistentModelIndex(index)]() {
			_filters->removeRows(index.row(), 1, index.parent());
		});
		insertAction(before, action.get());
	}
	// Move all the new elements added at the back to their proper position
	std::rotate(next(_ui->remove_filter_actions.begin(), first),
		prev(_ui->remove_filter_actions.end(), last-first+1),
		_ui->remove_filter_actions.end());
}

void FilterBar::filterInserted(const QModelIndex &parent, int first, int last)
{
	insertFilterButtons(first, last);
}

void FilterBar::filterRemoved(const QModelIndex &parent, int first, int last)
{
	_ui->remove_filter_actions.erase(
			next(_ui->remove_filter_actions.begin(), first),
			next(_ui->remove_filter_actions.begin(), last+1)
	);
}

void FilterBar::updateFilterUi()
{
	auto type = _ui->filter_type_cb->currentData().value<FilterType>();
	switch (type) {
	case FilterType::Simple:
		_ui->filter_text->setPlaceholderText(tr("Name filter"));
		break;
	case FilterType::Regex:
		_ui->filter_text->setPlaceholderText(tr("Name regex filter"));
		break;
	case FilterType::Script:
		_ui->filter_text->setPlaceholderText(tr("Script filter"));
		break;
	}
}

void FilterBar::updateTemporaryFilter()
{
	auto filter_type = _ui->filter_type_cb->currentData().value<FilterType>();
	auto text = _ui->filter_text->text();
	if (text.isEmpty())
		_filters->setTemporaryFilter(AllUnits{});
	else {
		switch (filter_type) {
		case FilterType::Simple:
			_filters->setTemporaryFilter(UnitNameFilter{
				text
			});
			break;
		case FilterType::Regex:
			_filters->setTemporaryFilter(UnitNameRegexFilter{
				QRegularExpression(text)
			});
			break;
		case FilterType::Script:
			_filters->setTemporaryFilter(ScriptedUnitFilter{
				Application::scripts().makeScript(text)
			});
			break;
		}
	}
}
