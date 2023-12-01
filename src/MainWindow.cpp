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
#include <QMenu>
#include <QProgressBar>
#include <QHeaderView>
#include <QMessageBox>
#include <QSortFilterProxyModel>

#include <QCoroCore>

#include "Application.h"
#include "UnitDetailsDock.h"
#include "ObjectList.h"
#include "Unit.h"
#include "GridViewModel.h"
#include "PreferencesDialog.h"

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

MainWindow::MainWindow(QWidget *parent):
	QMainWindow(parent),
	_ui(std::make_unique<Ui::MainWindow>()),
	_sb_ui(std::make_unique<StatusBarUi>()),
	_df(std::make_unique<DwarfFortress>()),
	_model(std::make_unique<GridViewModel>(*_df)),
	_sort_model(std::make_unique<QSortFilterProxyModel>())
{
	_ui->setupUi(this);
	_sb_ui->setupUi(_ui->statusbar);

	const auto &settings = Application::settings();

	_sort_model->setSourceModel(_model.get());
	_ui->view->setModel(_sort_model.get());
	connect(_ui->view, &GridView::contextMenuRequestedForHeader, this, [this](int section, const QPoint &pos) {
		QMenu menu;
		_model->makeColumnMenu(section, &menu, this);
		if (!menu.isEmpty())
			menu.exec(pos);
	});
	connect(_ui->view, &GridView::contextMenuRequestedForCell, this, [this](const QModelIndex &index, const QPoint &pos) {
		QMenu menu;
		_model->makeCellMenu(_sort_model->mapToSource(index), &menu, this);
		if (!menu.isEmpty())
			menu.exec(pos);
	});

	_ui->filter_cb->addItem(tr("Fort controlled"),
			QVariant::fromValue(GridViewModel::BaseFilter::FortControlled));
	_ui->filter_cb->addItem(tr("Worker"),
			QVariant::fromValue(GridViewModel::BaseFilter::Worker));
	connect(_ui->filter_cb, &QComboBox::currentIndexChanged, this, [this](int index) {
		_model->setFilter(_ui->filter_cb->itemData(index).value<GridViewModel::BaseFilter>());
	});
	_model->setFilter(GridViewModel::BaseFilter::FortControlled);

	_ui->group_by_cb->addItem(tr("No group"),
			QVariant::fromValue(GridViewModel::Group::NoGroup));
	_ui->group_by_cb->addItem(tr("Creature"),
			QVariant::fromValue(GridViewModel::Group::Creature));
	connect(_ui->group_by_cb, &QComboBox::currentIndexChanged, this, [this](int index) {
		_model->setGroupBy(_ui->group_by_cb->itemData(index).value<GridViewModel::Group>());
	});
	_model->setGroupBy(GridViewModel::Group::NoGroup);

	connect(_df.get(), &DwarfFortress::error, this, [this](const QString &msg) {
		QMessageBox::critical(this, tr("Connection error"), msg);
	});
	connect(_df.get(), &DwarfFortress::connectionProgress, this, [this](const QString &msg) {
		_sb_ui->loading_label->setText(msg);
	});
	connect(_df.get(), &DwarfFortress::stateChanged, this, &MainWindow::onStateChanged);
	onStateChanged(_df->state());

	auto unit_details = new UnitDetailsDock(*_df, this);
	addDockWidget(Qt::LeftDockWidgetArea, unit_details);
	_ui->view_menu->addAction(unit_details->toggleViewAction());

	connect(_ui->view->selectionModel(), &QItemSelectionModel::currentChanged, this, [this, unit_details](const QModelIndex &current, const QModelIndex &prev) {
		auto unit_index = _model->sourceUnitIndex(_sort_model->mapToSource(current));
		if (unit_index == _current_unit)
			return;
		_current_unit = unit_index;
		if (_current_unit.isValid())
			unit_details->setUnit(_df->units().get(_current_unit.row()));
		else
			unit_details->setUnit(nullptr);
	});

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
