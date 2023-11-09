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
#include "IconProvider.h"

WorkDetailEditor::WorkDetailEditor(QWidget *parent, Qt::WindowFlags f):
	QDialog(parent, f),
	_ui(std::make_unique<Ui::WorkDetailEditor>())
{
	_ui->setupUi(this);
	_ui->mode->addItem(tr("Everybody does this"), df::work_detail_mode::EverybodyDoesThis);
	_ui->mode->addItem(tr("Nobody does this"), df::work_detail_mode::NobodyDoesThis);
	_ui->mode->addItem(tr("Only selected does this"), df::work_detail_mode::OnlySelectedDoesThis);
	_ui->icon->addItem(tr("None"), df::work_detail_icon::ICON_NONE);
	for (int i = 0; i < df::work_detail_icon::Count; ++i) {
		auto icon = static_cast<df::work_detail_icon_t>(i);
		auto name = QString::fromLocal8Bit(to_string(icon));
		_ui->icon->addItem(Application::icons().workdetail(icon), name, icon);
	}
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
