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

#include "PreferencesDialog.h"

#include "ui_PreferencesDialog.h"

#include "Application.h"

PreferencesDialog::PreferencesDialog(QWidget *parent):
	QDialog(parent),
	_ui(std::make_unique<Ui::PreferencesDialog>())
{
	_ui->setupUi(this);
	const auto &settings = Application::settings();
	_ui->host_port->setValidator(settings.host_port.validator());
	connect(_ui->reset_defaults_button, &QAbstractButton::clicked,
		this, &PreferencesDialog::loadDefaultSettings);
}

PreferencesDialog::~PreferencesDialog()
{
}

void PreferencesDialog::loadSettings()
{
	const auto &settings = Application::settings();
	_ui->host_address->setText(settings.host_address());
	_ui->host_port->setText(QString::number(settings.host_port()));
	_ui->host_autoconnect->setChecked(settings.autoconnect());
	_ui->autorefresh_enable->setChecked(settings.autorefresh_enabled());
	_ui->autorefresh_interval->setValue(settings.autorefresh_interval());
	_ui->use_native_process->setChecked(settings.use_native_process());
	_ui->bypass_work_detail_protection->setChecked(settings.bypass_work_detail_protection());
	_ui->gridview_perview_groups->setChecked(settings.per_view_group_by());
	_ui->gridview_perview_filters->setChecked(settings.per_view_filters());
}

void PreferencesDialog::loadDefaultSettings()
{
	const auto &settings = Application::settings();
	_ui->host_address->setText(settings.host_address.defaultValue());
	_ui->host_port->setText(QString::number(settings.host_port.defaultValue()));
	_ui->host_autoconnect->setChecked(settings.autoconnect.defaultValue());
	_ui->autorefresh_enable->setChecked(settings.autorefresh_enabled.defaultValue());
	_ui->autorefresh_interval->setValue(settings.autorefresh_interval.defaultValue());
	_ui->use_native_process->setChecked(settings.use_native_process.defaultValue());
	_ui->bypass_work_detail_protection->setChecked(settings.bypass_work_detail_protection.defaultValue());
	_ui->gridview_perview_groups->setChecked(settings.per_view_group_by.defaultValue());
	_ui->gridview_perview_filters->setChecked(settings.per_view_filters.defaultValue());
}

void PreferencesDialog::saveSettings() const
{
	auto &settings = Application::settings();
	settings.host_address = _ui->host_address->text();
	settings.host_port = _ui->host_port->text().toInt();
	settings.autoconnect = _ui->host_autoconnect->isChecked();
	settings.autorefresh_enabled = _ui->autorefresh_enable->isChecked();
	settings.autorefresh_interval = _ui->autorefresh_interval->value();
	settings.use_native_process = _ui->use_native_process->isChecked();
	settings.bypass_work_detail_protection = _ui->bypass_work_detail_protection->isChecked();
	settings.per_view_group_by = _ui->gridview_perview_groups->isChecked();
	settings.per_view_filters = _ui->gridview_perview_filters->isChecked();
}
