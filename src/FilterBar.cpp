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
#include <QCompleter>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QToolButton>
#include <QWidgetAction>
#include <QMainWindow>
#include <QStatusBar>

#include "Application.h"
#include "ScriptManager.h"
#include "UserUnitFilters.h"
#include "Unit.h"

static QStatusBar *findStatusBar(QWidget *widget)
{
	for (; widget; widget = widget->parentWidget())
		if (auto main_window = qobject_cast<QMainWindow *>(widget))
			return main_window->statusBar();
	return nullptr;
}

struct FilterBar::Ui
{
	QWidgetAction *filter_type_action;
	QComboBox *filter_type_cb;
	QWidgetAction *filter_text_action;
	QLineEdit *filter_text;
	QCompleter *filter_script_completer;
	std::vector<std::unique_ptr<QAction>> remove_filter_actions;
	QWidgetAction *add_filter_action;
	QMenu *add_filter_menu;

	void setupUi(QToolBar *toolbar)
	{
		toolbar->setObjectName("FilterBar");
		toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

		auto title_action = new QWidgetAction(toolbar);
		title_action->setDefaultWidget(new QLabel(tr("Filters: ")));
		toolbar->addAction(title_action);

		filter_type_action = new QWidgetAction(toolbar);
		filter_type_cb = new QComboBox;
		filter_type_cb->addItem(tr("Simple"),
				QVariant::fromValue(UserUnitFilters::TemporaryType::Simple));
		filter_type_cb->addItem(tr("Regex"),
				QVariant::fromValue(UserUnitFilters::TemporaryType::Regex));
		filter_type_cb->addItem(tr("Script"),
				QVariant::fromValue(UserUnitFilters::TemporaryType::Script));
		filter_type_action->setDefaultWidget(filter_type_cb);
		toolbar->addAction(filter_type_action);

		filter_text_action = new QWidgetAction(toolbar);
		filter_text = new QLineEdit;
		filter_text->setClearButtonEnabled(true);
		filter_text_action->setDefaultWidget(filter_text);
		toolbar->addAction(filter_text_action);

		filter_script_completer = new ScriptPropertiesCompleter(toolbar);
		filter_script_completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
		filter_script_completer->setWidget(filter_text);

		add_filter_action = new QWidgetAction(toolbar);
		auto add_filter_button = new QToolButton;
		add_filter_menu = new QMenu(add_filter_button);
		add_filter_button->setMenu(add_filter_menu);
		add_filter_action->setText(tr("Add"));
		add_filter_action->setIcon(QIcon::fromTheme("list-add"));
		add_filter_button->setDefaultAction(add_filter_action);
		add_filter_button->setToolButtonStyle(Qt::ToolButtonIconOnly);
		add_filter_button->setPopupMode(QToolButton::InstantPopup);
		add_filter_action->setDefaultWidget(add_filter_button);
		toolbar->addAction(add_filter_action);
	}
};

FilterBar::FilterBar(QWidget *parent):
	QToolBar(tr("Filters"), parent),
	_ui(std::make_unique<FilterBar::Ui>()),
	_filters(std::make_shared<UserUnitFilters>())
{
	_ui->setupUi(this);
	connect(_ui->filter_type_cb, &QComboBox::currentIndexChanged, this, [this](int index) {
		updateFilterUi();
		updateTemporaryFilter();
	});
	connect(_ui->filter_text, &QLineEdit::textChanged,
		this, &FilterBar::updateTemporaryFilter);
	connect(_ui->filter_text, &QLineEdit::textEdited,
		this, &FilterBar::filterEditChanged);
	connect(_ui->filter_text, &QLineEdit::cursorPositionChanged,
		this, &FilterBar::filterEditChanged);
	connect(_ui->filter_text, &QLineEdit::selectionChanged,
		this, &FilterBar::filterEditChanged);
	connect(_ui->filter_script_completer, qOverload<const QString &>(&QCompleter::activated),
		this, &FilterBar::completionActivated);
	connect(_ui->filter_script_completer, qOverload<const QModelIndex &>(&QCompleter::highlighted),
		this, &FilterBar::completionHighlighted);

	auto make_add_filter_action = [&, this](const QString &name, auto data) {
		auto action = new QAction(_ui->add_filter_menu);
		action->setText(name);
		action->setData(QVariant::fromValue(data));
		connect(action, &QAction::triggered, this, [this, action]() {
			Q_ASSERT(_filters);
			if (action->data().metaType() == QMetaType::fromType<QJSValue>()) {
				_filters->addFilter(action->text(), ScriptedUnitFilter{action->data().value<QJSValue>()});
			}
			else if (action->data().metaType() == QMetaType::fromType<std::size_t>()) {
				const auto &[name, filter] = BuiltinUnitFilters[action->data().value<std::size_t>()];
				_filters->addFilter(action->text(), filter);
			}
			else
				qFatal() << "Invalid filter";
		});
		_ui->add_filter_menu->addAction(action);
	};
	for (std::size_t i = 0; i < BuiltinUnitFilters.size(); ++i) {
		const auto &[name, filter] = BuiltinUnitFilters[i];
		make_add_filter_action(QCoreApplication::translate("BuiltinUnitFilters", name), i);
	}
	_ui->add_filter_menu->addSeparator();
	for (const auto &[name, filter]: Application::scripts().filters())
		make_add_filter_action(name, filter);

	setupFilters();
}

FilterBar::~FilterBar()
{
}

void FilterBar::setFilters(std::shared_ptr<UserUnitFilters> filters)
{
	Q_ASSERT(filters);

	if (filters == _filters) // same filters, no change
		return;

	// clean up old filters
	disconnect(_inserted_signal);
	disconnect(_removed_signal);
	_ui->remove_filter_actions.clear();

	_filters = std::move(filters);
	setupFilters();
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

void FilterBar::setupFilters()
{
	// connect and setup new filters
	_inserted_signal = connect(
			_filters.get(), &QAbstractItemModel::rowsInserted,
			this, &FilterBar::filterInserted);
	_removed_signal = connect(
			_filters.get(), &QAbstractItemModel::rowsAboutToBeRemoved,
			this, &FilterBar::filterRemoved);
	if (_filters->rowCount() > 0)
		insertFilterButtons(0, _filters->rowCount()-1);
	// setup temporary filter ui
	auto [type, text] = _filters->temporaryFilter();
	auto cb_index = _ui->filter_type_cb->findData(QVariant::fromValue(type));
	_ui->filter_type_cb->setCurrentIndex(cb_index);
	_ui->filter_text->setText(text);
	updateFilterUi();
}

void FilterBar::updateFilterUi()
{
	auto type = _ui->filter_type_cb->currentData().value<UserUnitFilters::TemporaryType>();
	switch (type) {
	case UserUnitFilters::TemporaryType::Simple:
		_ui->filter_text->setPlaceholderText(tr("Name filter"));
		break;
	case UserUnitFilters::TemporaryType::Regex:
		_ui->filter_text->setPlaceholderText(tr("Name regex filter"));
		break;
	case UserUnitFilters::TemporaryType::Script:
		_ui->filter_text->setPlaceholderText(tr("Script filter"));
		break;
	}
}

void FilterBar::updateTemporaryFilter()
{
	_filters->setTemporaryFilter(
		_ui->filter_type_cb->currentData().value<UserUnitFilters::TemporaryType>(),
		_ui->filter_text->text()
	);
}

static QRegularExpressionMatch findCurrentJSIdentifer(const QString &text, int cursor, bool full = false)
{
	// match partial identifier.member
	static const QRegularExpression no_anchor("\\p{ID_Start}\\p{ID_Continue}*(?:\\.\\p{ID_Start}\\p{ID_Continue}*)*\\.?");
	static const QRegularExpression anchor_at_end(no_anchor.pattern() + "\\z");
	// Match before the cursor
	auto match = anchor_at_end.matchView(QStringView(text).first(cursor));
	if (full && match.hasMatch()) { // Also match after the cursor
		match = no_anchor.match(text,
				match.capturedStart(),
				QRegularExpression::NormalMatch,
				QRegularExpression::AnchorAtOffsetMatchOption);
	}
	return match;
}

void FilterBar::filterEditChanged()
{
	if (_ui->filter_type_cb->currentData().value<UserUnitFilters::TemporaryType>() != UserUnitFilters::TemporaryType::Script)
		return;
	auto filter_text  = _ui->filter_text->text();
	auto cursor_pos = _ui->filter_text->selectionStart() != -1
		? _ui->filter_text->selectionStart()
		: _ui->filter_text->cursorPosition();
	auto match = findCurrentJSIdentifer(filter_text, cursor_pos);
	_ui->filter_script_completer->setCompletionPrefix(match.captured());
	_ui->filter_script_completer->complete();
}

void FilterBar::completionActivated(const QString &text)
{
	auto filter_text = _ui->filter_text->text();
	auto cursor_pos = _ui->filter_text->selectionStart() != -1
		? _ui->filter_text->selectionStart()
		: _ui->filter_text->cursorPosition();
	auto match = findCurrentJSIdentifer(filter_text, cursor_pos, true);
	if (match.hasMatch()) {
		filter_text.replace(match.capturedStart(), match.capturedLength(), text);
		_ui->filter_text->setText(filter_text);
		_ui->filter_text->setCursorPosition(match.capturedStart() + text.size());
	}
	else {
		filter_text.insert(cursor_pos, text);
		_ui->filter_text->setText(filter_text);
		_ui->filter_text->setCursorPosition(cursor_pos + text.size());
	}
}

void FilterBar::completionHighlighted(const QModelIndex &index)
{
	if (auto status_bar = findStatusBar(this))
		status_bar->showMessage(index.data(Qt::StatusTipRole).toString());
}
