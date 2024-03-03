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

#include "GridViewTabs.h"

#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include "Application.h"
#include "DwarfFortress.h"
#include "DwarfFortressData.h"
#include "FilterBar.h"
#include "GridView.h"
#include "GridViewManager.h"
#include "GridViewModel.h"
#include "GroupBar.h"
#include "ObjectList.h"
#include "Settings.h"
#include "StandardPaths.h"
#include "Unit.h"

void foreachGridView (QTabWidget *tabs, std::invocable<GridView *> auto &&f) {
	for (int i = 1; i < tabs->count(); ++i) {
		auto view = qobject_cast<GridView *>(tabs->widget(i));
		Q_ASSERT(view);
		f(view);
	}
};

static constexpr QStringView QSETTINGS_OPENED_GRIDVIEWS = u"opened_gridviews";
static constexpr QStringView QSETTINGS_OPENED_GRIDVIEWS_NAME = u"name";
static constexpr const char GRIDVIEW_NAME_PROPERTY[] = "gridview_name";

GridViewTabs::GridViewTabs(QWidget *parent):
	QTabWidget(parent)
{
	setMovable(true);

	const auto &settings = Application::settings();
	connect(&settings.per_view_group_by, &SettingPropertyBase::valueChanged, this, [this]() {
		if (!_group_bar)
			return;
		if (!Application::settings().per_view_group_by()) {
			foreachGridView(this, [this](GridView *view) {
				if (view != currentWidget())
					view->gridViewModel().setGroupBy(_group_bar->groupIndex());
			});
		}
	});
	connect(&settings.per_view_filters, &SettingPropertyBase::valueChanged, this, [this]() {
		if (!_filter_bar)
			return;
		if (Application::settings().per_view_filters()) {
			foreachGridView(this, [this](GridView *view) {
				if (view != currentWidget())
					view->gridViewModel().setUserFilters(std::make_shared<UserUnitFilters>(*_filter_bar->filters()));
			});
		}
		else {
			foreachGridView(this, [this](GridView *view) {
				if (view != currentWidget())
					view->gridViewModel().setUserFilters(_filter_bar->filters());
			});
		}
	});
	connect(this, &QTabWidget::currentChanged, this, &GridViewTabs::onCurrentTabChanged);
	connect(this, &QTabWidget::tabCloseRequested, this, [this](int index) {
		Q_ASSERT(index != 0);
		delete widget(index);
	});

	// Add button for adding new grid views
	auto add_button = new QToolButton(this);
	add_button->setText(tr("Add"));
	add_button->setIcon(QIcon::fromTheme("tab-new"));
	add_button->setToolButtonStyle(Qt::ToolButtonIconOnly);
	add_button->setPopupMode(QToolButton::InstantPopup);
	auto add_menu = new QMenu(this);
	for (const auto &[name, params]: Application::gridviews().gridviews()) {
		auto action = new QAction(add_menu);
		action->setText(params.title);
		connect(action, &QAction::triggered, this, [this, name]() { addView(name); });
		add_menu->addAction(action);
	}
	add_button->setMenu(add_menu);
	setCornerWidget(add_button);

	{ // Placeholder widget page, only visible when no other is opened
		auto widget = new QWidget(this);
		auto layout = new QVBoxLayout;
		widget->setLayout(layout);
		layout->addStretch();
		auto button = new QPushButton(widget);
		button->setText(tr("Add a grid view"));
		button->setMenu(add_menu);
		layout->addWidget(button, 0, Qt::AlignCenter);
		layout->addStretch();
		addTab(widget, tr("Add grid views"));
	}
}

GridViewTabs::~GridViewTabs()
{
	// Save opened gridviews;
	auto qsettings = StandardPaths::settings();
	qsettings.beginWriteArray(QSETTINGS_OPENED_GRIDVIEWS, count()-1);
	for (int i = 1; i < count(); ++i) {
		qsettings.setArrayIndex(i-1);
		qsettings.setValue(QSETTINGS_OPENED_GRIDVIEWS_NAME,
				widget(i)->property(GRIDVIEW_NAME_PROPERTY).toString());
	}
	qsettings.endArray();
}

void GridViewTabs::init(GroupBar *group_bar, FilterBar *filter_bar, DwarfFortress *df)
{
	_group_bar = group_bar;
	connect(_group_bar, &GroupBar::groupChanged, this, [this](int index) {
		if (Application::settings().per_view_group_by()) {
			if (auto view = qobject_cast<GridView *>(currentWidget()))
				view->gridViewModel().setGroupBy(index);
		}
		else foreachGridView(this, [index](GridView *view) {
			view->gridViewModel().setGroupBy(index);
		});
	});

	_filter_bar = filter_bar;

	_df = df;
	_unit_selection.setModel(_df->data()->units.get());

	// Add previously saved grid views
	auto qsettings = StandardPaths::settings();
	int gridview_count = qsettings.beginReadArray(QSETTINGS_OPENED_GRIDVIEWS);
	for (int i = 0; i < gridview_count; ++i) {
		qsettings.setArrayIndex(i);
		addView(qsettings.value(QSETTINGS_OPENED_GRIDVIEWS_NAME).toString());
	}
	qsettings.endArray();

	setTabsClosable(gridview_count > 0);
	setTabVisible(0, gridview_count == 0);
}

void GridViewTabs::addView(const QString &name)
{
	Q_ASSERT(_df);
	try {
		const auto &settings = Application::settings();
		const auto &params = Application::gridviews().find(name);
		auto view = new GridView(std::make_unique<GridViewModel>(params, _df->data()), this);
		view->setProperty(GRIDVIEW_NAME_PROPERTY, name);
		if (!settings.per_view_group_by())
			view->gridViewModel().setGroupBy(_group_bar ? _group_bar->groupIndex() : 0);
		if (settings.per_view_filters() || !_filter_bar)
			view->gridViewModel().setUserFilters(std::make_shared<UserUnitFilters>());
		else
			view->gridViewModel().setUserFilters(_filter_bar->filters());
		connect(view->selectionModel(), &QItemSelectionModel::currentChanged,
			this, [this, view](const QModelIndex &current, const QModelIndex &prev) {
				onCurrentUnitChanged(view, current, prev);
			});
		connect(view->selectionModel(), &QItemSelectionModel::selectionChanged,
			this, [this, view](const QItemSelection &selected, const QItemSelection &deselected) {
				onSelectionChanged(view, selected, deselected);
			});
		addTab(view, params.title);
		setTabVisible(0, false); // Hide placeholder tab
		setTabsClosable(true);
	}
	catch (std::exception &e) {
		qCritical() << "Cannot add view" << name << e.what();
	}
}

void GridViewTabs::tabRemoved(int index)
{
	Q_ASSERT(index > 0); // placeholder tab should not be removable
	if (count() == 1) { // last tab is placeholder
		setTabsClosable(false);
		setTabVisible(0, true);
	}
}

void GridViewTabs::onCurrentTabChanged(int index)
{
	if (auto view = qobject_cast<GridView *>(widget(index))) {
		const auto &settings = Application::settings();
		if (settings.per_view_group_by() && _group_bar)
			_group_bar->setGroup(view->gridViewModel().groupIndex());
		if (settings.per_view_filters() && _filter_bar)
			_filter_bar->setFilters(view->gridViewModel().userFilters());
		if (settings.sync_selection()) {
			auto current_index = view->mapFromSource(_current_unit);
			auto current_selection = view->mapSelectionFromSource(_unit_selection.selection());
			view->selectionModel()->select(current_selection,
					QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
			view->selectionModel()->setCurrentIndex(current_index,
					current_selection.isEmpty()
						? QItemSelectionModel::Select
						: QItemSelectionModel::NoUpdate);
		}
		else {
			auto current_index = view->mapToSource(view->currentIndex());
			if (_current_unit != current_index) {
				_current_unit = current_index;
				currentUnitChanged(current_index);
			}
		}
		// Now that current index and selection are set, let the view
		// change the saved selection and index.
		_controlling_view = view;

	}
}

void GridViewTabs::onCurrentUnitChanged(GridView *view, const QModelIndex &current, const QModelIndex &prev)
{
	if (_controlling_view != view)
		return;
	auto unit_index = view->mapToSource(current);
	if (unit_index != _current_unit) {
		_current_unit = unit_index;
		currentUnitChanged(unit_index);
	}
}

void GridViewTabs::onSelectionChanged(GridView *view, const QItemSelection &selected, const QItemSelection &deselected)
{
	if (_controlling_view != view)
		return;
	_unit_selection.select(
			view->mapSelectionToSource(view->selectionModel()->selection()),
			QItemSelectionModel::ClearAndSelect);
}

UserUnitFilters *GridViewTabs::currentFilters()
{
	return _filter_bar->filters().get();
}
