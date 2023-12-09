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

#include "MainWindow.h"

#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QMetaEnum>
#include <QDockWidget>
#include <QProgressBar>
#include <QMessageBox>

#include <QCoroCore>

#include "Application.h"
#include "UnitDetailsDock.h"
#include "ObjectList.h"
#include "Unit.h"
#include "GridViewModel.h"
#include "PreferencesDialog.h"
#include "FilterBar.h"
#include "GroupBar.h"
#include "GridViewManager.h"
#include "GridView.h"

#include "ui_MainWindow.h"
#include "ui_AdvancedConnectionDialog.h"
#include "ui_AboutDialog.h"

struct MainWindow::StatusBarUi
{
	QLabel *dfversion;
	QLabel *connection_status;
	QProgressBar *loading_bar;
	QLabel *loading_label;

	void setupUi(QStatusBar *status_bar) {
		dfversion = new QLabel(status_bar);
		status_bar->addWidget(dfversion);
		connection_status = new QLabel(status_bar);
		status_bar->addPermanentWidget(connection_status);
		loading_bar = new QProgressBar(status_bar);
		loading_bar->setRange(0, 0);
		loading_bar->setValue(0);
		loading_bar->setVisible(false);
		status_bar->addPermanentWidget(loading_bar);
		loading_label = new QLabel(status_bar);
		loading_label->setVisible(false);
		status_bar->addPermanentWidget(loading_label);
	}
};


void foreachGridView (QTabWidget *tabs, std::invocable<GridView *> auto &&f) {
	for (int i = 0; i < tabs->count(); ++i) {
		auto view = qobject_cast<GridView *>(tabs->widget(i));
		Q_ASSERT(view);
		f(view);
	}
};

MainWindow::MainWindow(QWidget *parent):
	QMainWindow(parent),
	_ui(std::make_unique<Ui::MainWindow>()),
	_sb_ui(std::make_unique<StatusBarUi>()),
	_df(std::make_unique<DwarfFortress>())
{
	_ui->setupUi(this);
	_sb_ui->setupUi(_ui->statusbar);

	auto tabs = new QTabWidget(this);
	for (const auto &[name, params]: Application::gridviews().gridviews()) {
		auto title = params.title;
		auto view = new GridView(std::make_unique<GridViewModel>(params, *_df), tabs);
		tabs->addTab(view, title);
	}
	setCentralWidget(tabs);

	const auto &settings = Application::settings();

	// Setup tool bars
	addToolBarBreak();

	// Groups
	auto group_bar = new GroupBar(this);
	connect(group_bar, &GroupBar::groupChanged, this, [tabs](int index) {
		if (Application::settings().per_view_group_by()) {
			auto view = qobject_cast<GridView *>(tabs->currentWidget());
			Q_ASSERT(view);
			view->gridViewModel().setGroupBy(index);
		}
		else foreachGridView(tabs, [index](GridView *view) {
			view->gridViewModel().setGroupBy(index);
		});
	});
	connect(&settings.per_view_group_by, &SettingPropertyBase::valueChanged, this, [tabs, group_bar]() {
		if (!Application::settings().per_view_group_by()) {
			foreachGridView(tabs, [tabs, group_bar](GridView *view) {
				if (view != tabs->currentWidget())
					view->gridViewModel().setGroupBy(group_bar->groupIndex());
			});
		}
	});
	connect(tabs, &QTabWidget::currentChanged, this, [tabs, group_bar](int index) {
		if (Application::settings().per_view_group_by()) {
			auto view = qobject_cast<GridView *>(tabs->widget(index));
			Q_ASSERT(view);
			group_bar->setGroup(view->gridViewModel().groupIndex());
		}
	});
	foreachGridView(tabs, [](GridView *view) {
		view->gridViewModel().setGroupBy(0);
	});
	group_bar->setGroup(0);
	addToolBar(group_bar);

	// Filters
	auto filter_bar = new FilterBar(this);
	if (settings.per_view_filters()) {
		foreachGridView(tabs, [tabs, filter_bar](GridView *view) {
			if (view != tabs->currentWidget())
				view->gridViewModel().setUserFilters(std::make_shared<UserUnitFilters>());
			else
				view->gridViewModel().setUserFilters(filter_bar->filters());
		});
	}
	else {
		foreachGridView(tabs, [filter_bar](GridView *view) {
			view->gridViewModel().setUserFilters(filter_bar->filters());
		});
	}
	connect(&settings.per_view_filters, &SettingPropertyBase::valueChanged, this, [tabs, filter_bar]() {
		if (Application::settings().per_view_filters()) {
			foreachGridView(tabs, [tabs, filter_bar](GridView *view) {
				if (view != tabs->currentWidget())
					view->gridViewModel().setUserFilters(std::make_shared<UserUnitFilters>(*filter_bar->filters()));
			});
		}
		else {
			foreachGridView(tabs, [tabs, filter_bar](GridView *view) {
				if (view != tabs->currentWidget())
					view->gridViewModel().setUserFilters(filter_bar->filters());
			});
		}
	});
	connect(tabs, &QTabWidget::currentChanged, this, [tabs, filter_bar](int index) {
		if (Application::settings().per_view_filters()) {
			auto view = qobject_cast<GridView *>(tabs->widget(index));
			Q_ASSERT(view);
			filter_bar->setFilters(view->gridViewModel().userFilters());
		}
	});
	addToolBar(filter_bar);

	// DFHack connection
	connect(_df.get(), &DwarfFortress::error, this, [this](const QString &msg) {
		QMessageBox::critical(this, tr("Connection error"), msg);
	});
	connect(_df.get(), &DwarfFortress::connectionProgress, this, [this](const QString &msg) {
		_sb_ui->loading_label->setText(msg);
	});
	connect(_df.get(), &DwarfFortress::stateChanged, this, &MainWindow::onStateChanged);
	onStateChanged(_df->state());

	// Setup dock
	auto unit_details = new UnitDetailsDock(*_df, this);
	addDockWidget(Qt::LeftDockWidgetArea, unit_details);
	_ui->view_menu->addAction(unit_details->toggleViewAction());

	// Unit details shows current unit
	foreachGridView(tabs, [this, unit_details](GridView *view) {
		connect(view->selectionModel(), &QItemSelectionModel::currentChanged,
			this, [this, view, unit_details](const QModelIndex &current, const QModelIndex &prev) {
				auto source_index = view->sortModel().mapToSource(current);
				auto unit = view->gridViewModel().unit(source_index);
				auto unit_index = unit ? _df->units().find(*unit) : QModelIndex{};
				if (unit_index == _current_unit)
					return;
				_current_unit = unit_index;
				unit_details->setUnit(unit);
			});
	});

	// Connect to DFHack
	if (settings.autoconnect())
		on_connect_action_triggered();
}

MainWindow::~MainWindow()
{
}

void MainWindow::onStateChanged(DwarfFortress::State state)
{
	_sb_ui->connection_status->setText(QMetaEnum::fromType<decltype(state)>().valueToKey(state));
	switch (state) {
	case DwarfFortress::State::Disconnected:
		_ui->connect_action->setEnabled(true);
		_ui->advanced_connection_action->setEnabled(true);
		_ui->disconnect_action->setEnabled(false);
		_ui->update_action->setEnabled(false);
		_sb_ui->loading_label->setVisible(false);
		_sb_ui->loading_bar->setVisible(false);
		_sb_ui->dfversion->setText({});
		break;
	case DwarfFortress::State::Connecting:
	case DwarfFortress::State::Updating:
		_ui->connect_action->setEnabled(false);
		_ui->advanced_connection_action->setEnabled(false);
		_ui->disconnect_action->setEnabled(false);
		_ui->update_action->setEnabled(false);
		_sb_ui->loading_label->setVisible(true);
		_sb_ui->loading_bar->setVisible(true);
		break;
	case DwarfFortress::State::Connected:
		_ui->connect_action->setEnabled(false);
		_ui->advanced_connection_action->setEnabled(false);
		_ui->disconnect_action->setEnabled(true);
		_ui->update_action->setEnabled(true);
		_sb_ui->loading_label->setVisible(false);
		_sb_ui->loading_bar->setVisible(false);
		_sb_ui->dfversion->setText(tr("DF %1 – DFHack %2")
				.arg(_df->getDFVersion())
				.arg(_df->getDFHackVersion()));
		break;
	}
}

void MainWindow::on_connect_action_triggered()
{
	const auto &settings = Application::settings();
	_df->connectToDF(settings.host_address(), settings.host_port());
}

void MainWindow::on_advanced_connection_action_triggered()
{
	const auto &settings = Application::settings();
	QDialog dialog;
	Ui::AdvancedConnectionDialog ui;
	ui.setupUi(&dialog);
	ui.address->setText(settings.host_address());
	ui.port->setValidator(settings.host_port.validator());
	ui.port->setText(QString::number(settings.host_port()));
	if (dialog.exec() == QDialog::Accepted)
		_df->connectToDF(ui.address->text(), ui.port->text().toShort());
}

void MainWindow::on_disconnect_action_triggered()
{
	_df->disconnectFromDF();
}

void MainWindow::on_update_action_triggered()
{
	_df->update();
}

void MainWindow::on_preferences_action_triggered()
{
	PreferencesDialog dialog(this);
	dialog.loadSettings();
	if (dialog.exec() == QDialog::Accepted)
		dialog.saveSettings();
}

void MainWindow::on_about_action_triggered()
{
	QDialog dialog(this);
	Ui::AboutDialog ui;
	ui.setupUi(&dialog);
	ui.version->setText(tr("Version %1").arg(Application::instance()->applicationVersion()));
	ui.qt_version->setText(tr("Build with Qt %1, running with Qt %2")
			.arg(QT_VERSION_STR)
			.arg(qVersion()));
	dialog.exec();
}
