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

#include "WorkDetailEditor.h"
#include "ui_WorkDetailEditor.h"

#include "Application.h"
#include "Settings.h"
#include "IconProvider.h"
#include "LaborModel.h"

WorkDetailEditor::WorkDetailEditor(QWidget *parent, Qt::WindowFlags f):
	QDialog(parent, f),
	_ui(std::make_unique<Ui::WorkDetailEditor>()),
	_labors(std::make_unique<LaborModel>())
{
	const auto &settings = Application::settings();
	_ui->setupUi(this);

	_ui->no_modify->setVisible(settings.bypass_work_detail_protection());
	connect(&settings.bypass_work_detail_protection, &SettingPropertyBase::valueChanged,
		this, [this]() { _ui->no_modify->setVisible(Application::settings().bypass_work_detail_protection()); });

	connect(_ui->cannot_be_everybody, &QCheckBox::toggled,
		this, &WorkDetailEditor::updateEverybodyDoesThis);


	_ui->mode->addItem(tr("Only selected does this"), df::work_detail_mode::OnlySelectedDoesThis);
	_ui->mode->addItem(tr("Nobody does this"), df::work_detail_mode::NobodyDoesThis);
	updateEverybodyDoesThis();

	_ui->icon->addItem(tr("None"), df::work_detail_icon::ICON_NONE);
	for (int i = 0; i < df::work_detail_icon::Count; ++i) {
		auto icon = static_cast<df::work_detail_icon_t>(i);
		auto name = QString::fromLocal8Bit(to_string(icon));
		_ui->icon->addItem(Application::icons().workdetail(icon), name, icon);
	}
	_labors->setGroupByCategory(true);
	_ui->labors->setModel(_labors.get());
}

WorkDetailEditor::~WorkDetailEditor()
{
}

QString WorkDetailEditor::name() const
{
	return _ui->name->text();
}

void WorkDetailEditor::setName(const QString &name)
{
	_ui->name->setText(name);
}

df::work_detail_mode_t WorkDetailEditor::mode() const
{
	return _ui->mode->currentData().value<df::work_detail_mode_t>();
}

void WorkDetailEditor::setMode(df::work_detail_mode_t mode)
{
	_ui->mode->setCurrentIndex(_ui->mode->findData(mode));
}

df::work_detail_icon_t WorkDetailEditor::icon() const
{
	return _ui->icon->currentData().value<df::work_detail_icon_t>();
}

void WorkDetailEditor::setIcon(df::work_detail_icon_t icon)
{
	_ui->icon->setCurrentIndex(_ui->icon->findData(icon));
}

bool WorkDetailEditor::noModify() const
{
	return _ui->no_modify->isChecked();
}

void WorkDetailEditor::setNoModify(bool enabled)
{
	_ui->no_modify->setChecked(enabled);
}

bool WorkDetailEditor::cannotBeEverybody() const
{
	return _ui->cannot_be_everybody->isChecked();
}

void WorkDetailEditor::setCannotBeEverybody(bool enabled)
{
	_ui->cannot_be_everybody->setChecked(enabled);
	updateEverybodyDoesThis();
}

void WorkDetailEditor::initFromWorkDetail(const WorkDetail &wd)
{
	setName(wd.displayName());
	setMode(static_cast<df::work_detail_mode_t>(wd->flags.bits.mode));
	setIcon(wd->icon);
	setNoModify(wd->flags.bits.no_modify);
	setCannotBeEverybody(wd->flags.bits.cannot_be_everybody);
	_labors->setLabors(wd->allowed_labors);
}

void WorkDetailEditor::initFromProperties(const WorkDetail::Properties &properties)
{
	setName(properties.name);
	if (properties.mode)
		setMode(*properties.mode);
	if (properties.icon)
		setIcon(*properties.icon);
	if (properties.no_modify)
		setNoModify(*properties.no_modify);
	if (properties.cannot_be_everybody)
		setCannotBeEverybody(*properties.cannot_be_everybody);
	std::array<bool, df::unit_labor::Count> labors = {false};
	for (const auto &[labor, enabled]: properties.labors)
		labors[labor] = enabled;
	_labors->setLabors(labors);
}

WorkDetail::Properties WorkDetailEditor::properties() const
{
	return {
		.name = name(),
		.mode = mode(),
		.icon = icon(),
		.no_modify = noModify(),
		.cannot_be_everybody = cannotBeEverybody(),
		.labors = WorkDetail::Properties::allLabors(_labors->labors()),
	};
}

void WorkDetailEditor::updateEverybodyDoesThis()
{
	auto index = _ui->mode->findData(df::work_detail_mode::EverybodyDoesThis);
	if (_ui->cannot_be_everybody->isChecked()) {
		if (index != -1)
			_ui->mode->removeItem(index);
	}
	else {
		if (index == -1)
			_ui->mode->insertItem(0,
					tr("Everybody does this"),
					df::work_detail_mode::EverybodyDoesThis);
	}
}
