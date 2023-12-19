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

#include "WorkDetailManager.h"

#include "ui_WorkDetailManager.h"
#include "DwarfFortressData.h"
#include "WorkDetailModel.h"
#include "WorkDetailEditor.h"
#include "LaborModel.h"

#include <QMessageBox>
#include <QMenu>

WorkDetailManager::WorkDetailManager(std::shared_ptr<DwarfFortressData> df, QWidget *parent, Qt::WindowFlags flags):
	QDialog(parent, flags),
	_ui(std::make_unique<Ui::WorkDetailManager>()),
	_df(std::move(df))
{
	_ui->setupUi(this);
	_ui->workdetails_view->setModel(_df->work_details.get());
}

WorkDetailManager::~WorkDetailManager()
{
}

void WorkDetailManager::on_workdetails_view_customContextMenuRequested(const QPoint &pos)
{
	QMenu menu(this);
	auto selection = _ui->workdetails_view->selectionModel()->selectedRows();
	if (!selection.isEmpty()) {
		// Remove Selection action
		auto action = new QAction(&menu);
		action->setIcon(QIcon::fromTheme("edit-delete"));
		if (selection.size() == 1) {
			auto index = selection.front();
			action->setText(tr("Remove %1").arg(index.data().toString()));
			connect(action, &QAction::triggered, this, [this, index = QPersistentModelIndex(index)]() {
				removeWorkDetail(index);
			});
		}
		else {
			action->setText(tr("Remove selected"));
			auto make_persistent = [](const QModelIndexList &indexes) {
				QList<QPersistentModelIndex> out;
				out.reserve(indexes.size());
				for (const auto &index: indexes)
					out.push_back(index);
				return out;
			};
			connect(action, &QAction::triggered, this, [this, indexes = make_persistent(selection)]() {
				removeWorkDetails(indexes);
			});
		}
		menu.addAction(action);
	}
	// Add new action
	auto add_action = new QAction(QIcon::fromTheme("document-new"), tr("Add new work detail"), &menu);
	connect(add_action, &QAction::triggered, this, &WorkDetailManager::addWorkDetail);
	menu.addAction(add_action);
	// Show menu
	menu.exec(_ui->workdetails_view->mapToGlobal(pos));
}

void WorkDetailManager::addWorkDetail()
{
	WorkDetailEditor editor(this);
	editor.setName(tr("New work detail"));
	editor.setMode(df::work_detail_mode::EverybodyDoesThis);
	editor.setIcon(df::work_detail_icon::ICON_NONE);
	if (QDialog::Accepted == editor.exec()) {
		_df->work_details->add({
			.name = editor.name(),
			.mode = editor.mode(),
			.icon = editor.icon(),
			.labors = WorkDetail::Properties::allLabors(editor.labors().labors()),
		});
	}
}

void WorkDetailManager::removeWorkDetail(const QPersistentModelIndex &index)
{
	QMessageBox question(this);
	question.setIcon(QMessageBox::Question);
	question.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	question.setWindowTitle(tr("Removing %1").arg(index.data().toString()));
	question.setText(tr("Are you sure you want to remove the work detail \"%1\"?")
				.arg(index.data().toString()));
	if (question.exec() == QMessageBox::Yes)
		_df->work_details->remove({index});
}

void WorkDetailManager::removeWorkDetails(const QList<QPersistentModelIndex> &indexes)
{
	if (indexes.size() == 1) {
		removeWorkDetail(indexes.front());
		return;
	}
	QMessageBox question(this);
	question.setIcon(QMessageBox::Question);
	question.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	question.setWindowTitle(tr("Removing multiple work details"));
	question.setText(tr("Are you sure you want to remove the following work details?"));
	QStringList names;
	names.reserve(indexes.size());
	for (auto index: indexes)
		names.push_back(index.data().toString());
	question.setInformativeText(names.join("\n"));
	if (question.exec() == QMessageBox::Yes)
		_df->work_details->remove(indexes);
}
