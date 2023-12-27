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
#include "Application.h"
#include "Settings.h"
#include "DwarfFortressData.h"
#include "WorkDetailModel.h"
#include "WorkDetailPresets.h"
#include "WorkDetailEditor.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QMenu>
#include <QDragEnterEvent>
#include <QShortcut>
#include <QClipboard>

template <typename T>
ExternalCopyView<T>::ExternalCopyView(QWidget *parent):
	T(parent)
{
}

template <typename T>
ExternalCopyView<T>::~ExternalCopyView()
{
}

// Hack around protected access for changing default action for drop events
class HackedDropEvent: public QDropEvent
{
public:
	void setProposedAction(Qt::DropAction action)
	{
		m_defaultAction = action;
		m_dropAction = action;
	}
};

template <std::derived_from<QDropEvent> T>
void forceAction(T *event, Qt::DropAction action)
{
	static_cast<HackedDropEvent *>(static_cast<QDropEvent *>(event))->setProposedAction(action);
}

template <typename T>
void ExternalCopyView<T>::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->source() != this)
		forceAction(event, Qt::CopyAction);
	T::dragEnterEvent(event);
}

template <typename T>
void ExternalCopyView<T>::dragMoveEvent(QDragMoveEvent *event)
{
	if (event->source() != this)
		forceAction(event, Qt::CopyAction);
	T::dragMoveEvent(event);
}

template <typename T>
void ExternalCopyView<T>::dropEvent(QDropEvent *event)
{
	if (event->source() != this)
		forceAction(event, Qt::CopyAction);
	T::dropEvent(event);
}

WorkDetailView::WorkDetailView(QWidget *parent):
	ExternalCopyView<QListView>(parent)
{
}

WorkDetailView::~WorkDetailView()
{
}

bool WorkDetailView::edit(const QModelIndex &index, QAbstractItemView::EditTrigger trigger, QEvent *event)
{
	switch (trigger) {
	case QAbstractItemView::DoubleClicked:
	case QAbstractItemView::EditKeyPressed: {
		auto wd_model = qobject_cast<WorkDetailModel *>(model());
		Q_ASSERT(wd_model);
		auto wd = wd_model->get(index.row());
		Q_ASSERT(wd);
		if (!Application::settings().bypass_work_detail_protection() && (*wd)->flags.bits.no_modify)
			return false;
		WorkDetailEditor editor(this);
		editor.initFromWorkDetail(*wd);
		if (editor.exec() == QDialog::Accepted)
			wd->edit(editor.properties());
		return false;
	}
	default:
		return QListView::edit(index, trigger, event);
	}
}

WorkDetailPresetView::WorkDetailPresetView(QWidget *parent):
	ExternalCopyView<QTreeView>(parent)
{
}

WorkDetailPresetView::~WorkDetailPresetView()
{
}

bool WorkDetailPresetView::edit(const QModelIndex &index, QAbstractItemView::EditTrigger trigger, QEvent *event)
{
	auto presets = qobject_cast<WorkDetailPresets *>(model());
	Q_ASSERT(presets);
	switch (trigger) {
	case QAbstractItemView::DoubleClicked:
	case QAbstractItemView::EditKeyPressed:
		if (presets->isUserWritable(index)) {
			if (auto properties = presets->workdetail(index)) {
				WorkDetailEditor editor(this);
				editor.initFromProperties(*properties);
				if (editor.exec() == QDialog::Accepted)
					presets->setProperties(index, editor.properties());
				return false;
			}
		}
		[[fallthrough]];
	default:
		return QTreeView::edit(index, trigger, event);
	}
}

struct WorkDetailManager::Shortcuts
{
	QShortcut *delete_preset;
	QShortcut *new_preset;
	QShortcut *copy_preset;
	QShortcut *cut_preset;
	QShortcut *paste_preset;
	QShortcut *delete_work_detail;
	QShortcut *new_work_detail;
	QShortcut *copy_work_detail;
	QShortcut *paste_work_detail;
};

WorkDetailManager::WorkDetailManager(std::shared_ptr<DwarfFortressData> df, QWidget *parent, Qt::WindowFlags flags):
	QDialog(parent, flags),
	_ui(std::make_unique<Ui::WorkDetailManager>()),
	_df(std::move(df)),
	_presets(std::make_unique<WorkDetailPresets>(this)),
	_shorcuts(std::make_unique<Shortcuts>())
{
	_ui->setupUi(this);
	_ui->workdetails_view->setModel(_df->work_details.get());
	_ui->presets_view->setModel(_presets.get());

	_shorcuts->delete_preset = new QShortcut(QKeySequence::Delete, _ui->presets_view,
			this, &WorkDetailManager::removeSelectedPresets, Qt::WidgetShortcut);
	_shorcuts->new_preset = new QShortcut(QKeySequence::New, _ui->presets_view,
			this, &WorkDetailManager::addPreset, Qt::WidgetShortcut);
	_shorcuts->copy_preset = new QShortcut(QKeySequence::Copy, _ui->presets_view,
			this, &WorkDetailManager::copyPresets, Qt::WidgetShortcut);
	_shorcuts->cut_preset = new QShortcut(QKeySequence::Cut, _ui->presets_view,
			this, &WorkDetailManager::cutPresets, Qt::WidgetShortcut);
	_shorcuts->paste_preset = new QShortcut(QKeySequence::Paste, _ui->presets_view,
			this, &WorkDetailManager::pastePresets, Qt::WidgetShortcut);
	_shorcuts->delete_work_detail = new QShortcut(QKeySequence::Delete, _ui->workdetails_view,
			this, &WorkDetailManager::removeSelectedWorkDetails, Qt::WidgetShortcut);
	_shorcuts->new_work_detail = new QShortcut(QKeySequence::New, _ui->workdetails_view,
			this, &WorkDetailManager::addWorkDetail, Qt::WidgetShortcut);
	_shorcuts->copy_work_detail = new QShortcut(QKeySequence::Copy, _ui->workdetails_view,
			this, &WorkDetailManager::copyWorkDetails, Qt::WidgetShortcut);
	_shorcuts->paste_work_detail = new QShortcut(QKeySequence::Paste, _ui->workdetails_view,
			this, &WorkDetailManager::pasteWorkDetails, Qt::WidgetShortcut);

	connect(_ui->move_top_button, &QAbstractButton::clicked,
		this, &WorkDetailManager::moveTop);
	connect(_ui->move_up_button, &QAbstractButton::clicked,
		this, &WorkDetailManager::moveUp);
	connect(_ui->move_down_button, &QAbstractButton::clicked,
		this, &WorkDetailManager::moveDown);
	connect(_ui->move_bottom_button, &QAbstractButton::clicked,
		this, &WorkDetailManager::moveBottom);
	connect(_ui->add_workdetail_button, &QAbstractButton::clicked,
		this, &WorkDetailManager::addWorkDetail);
	connect(_ui->remove_workdetails_button, &QAbstractButton::clicked,
		this, &WorkDetailManager::removeSelectedWorkDetails);
	connect(_ui->workdetails_view->selectionModel(), &QItemSelectionModel::selectionChanged,
		this, [this]() {
			bool has_selection = _ui->workdetails_view->selectionModel()->hasSelection();
			const auto &selection = _ui->workdetails_view->selectionModel()->selectedRows();
			auto editable = Application::settings().bypass_work_detail_protection() ||
					std::ranges::none_of(selection, [this](const auto &index) {
						auto wd = _df->work_details->get(index.row());
						return (*wd)->flags.bits.no_modify;
					});
			_ui->move_top_button->setEnabled(has_selection);
			_ui->move_up_button->setEnabled(has_selection);
			_ui->move_down_button->setEnabled(has_selection);
			_ui->move_bottom_button->setEnabled(has_selection);
			_ui->remove_workdetails_button->setEnabled(has_selection && editable);
			_shorcuts->delete_work_detail->setEnabled(has_selection && editable);
		});
}

WorkDetailManager::~WorkDetailManager()
{
}

static QPersistentModelIndex make_persistent(const QModelIndex &index)
{
	return index;
}

static QList<QPersistentModelIndex> make_persistent(const QModelIndexList &indexes)
{
	QList<QPersistentModelIndex> out;
	out.reserve(indexes.size());
	for (const auto &index: indexes)
		out.push_back(index);
	return out;
}

void WorkDetailManager::on_workdetails_view_customContextMenuRequested(const QPoint &pos)
{
	const auto &settings = Application::settings();
	QMenu menu(this);
	auto index = _ui->workdetails_view->indexAt(pos);
	if (index.isValid()) {
		auto wd = _df->work_details->get(index.row());
		// Edit action
		auto edit_action = new QAction(QIcon::fromTheme("document-edit"), tr("Edit %1...").arg(index.data().toString()), &menu);
		connect(edit_action, &QAction::triggered, this, [this, index = make_persistent(index)]() {
			WorkDetailEditor editor(this);
			if (!index.isValid())
				return;
			auto wd = _df->work_details->get(index.row());
			if (!wd)
				return;
			editor.initFromWorkDetail(*wd);
			if (editor.exec() == QDialog::Accepted)
				wd->edit(editor.properties());
		});
		edit_action->setEnabled(settings.bypass_work_detail_protection() || !(*wd)->flags.bits.no_modify);
		menu.addAction(edit_action);
	}
	auto selection = _ui->workdetails_view->selectionModel()->selectedRows();
	if (!selection.isEmpty()) {
		// Remove Selection action
		auto remove_action = new QAction(&menu);
		remove_action->setIcon(QIcon::fromTheme("edit-delete"));
		if (selection.size() == 1) {
			auto index = selection.front();
			remove_action->setText(tr("Remove %1").arg(index.data().toString()));
			connect(remove_action, &QAction::triggered, this,
				[this, index = make_persistent(index)]() {
					removeWorkDetail(index);
				});
		}
		else {
			remove_action->setText(tr("Remove selected work details"));
			connect(remove_action, &QAction::triggered, this,
				[this, indexes = make_persistent(selection)]() {
					removeWorkDetails(indexes);
				});
		}
		remove_action->setEnabled(settings.bypass_work_detail_protection() ||
			std::ranges::none_of(selection, [this](const auto &index) {
				auto wd = _df->work_details->get(index.row());
				return (*wd)->flags.bits.no_modify;
			}));
		menu.addAction(remove_action);
		// Export menu
		auto export_menu = new QMenu(tr("Export"), &menu);
		export_menu->setIcon(QIcon::fromTheme("document-export"));
		for (int i = 0; i < _presets->rowCount(); ++i) {
			auto index = _presets->index(i, 0);
			if (index.flags() & Qt::ItemIsDropEnabled) {
				auto action = new QAction(
						tr("Add to %1").arg(index.data().toString()),
						export_menu);
				connect(action, &QAction::triggered, this,
					[this, dest = make_persistent(index), indexes = make_persistent(selection)]() {
						exportWorkDetails(indexes, dest);
					});
				export_menu->addAction(action);
			}
		}
		auto new_preset_action = new QAction(tr("Create new preset..."), export_menu);
		connect(new_preset_action, &QAction::triggered, this,
			[this, indexes = make_persistent(selection)]() {
				exportWorkDetails(indexes, {});
			});
		export_menu->addAction(new_preset_action);
		menu.addMenu(export_menu);
	}
	// Add new action
	auto add_action = new QAction(
			QIcon::fromTheme("document-new"),
			tr("Add new work detail"),
			&menu);
	connect(add_action, &QAction::triggered, this, &WorkDetailManager::addWorkDetail);
	menu.addAction(add_action);
	// Show menu
	menu.exec(_ui->workdetails_view->mapToGlobal(pos));
}

void WorkDetailManager::on_presets_view_customContextMenuRequested(const QPoint &pos)
{
	QMenu menu(this);
	auto index = _ui->presets_view->indexAt(pos);
	if (index.isValid()) {
		if (_presets->isWorkDetail(index)) {
			// Edit action
			auto edit_action = new QAction(QIcon::fromTheme("document-edit"), tr("Edit %1...").arg(index.data().toString()), &menu);
			connect(edit_action, &QAction::triggered, this, [this, index = make_persistent(index)]() {
				WorkDetailEditor editor(this);
				if (!index.isValid())
					return;
				if (auto properties = _presets->workdetail(index)) {
					editor.initFromProperties(*properties);
					if (editor.exec() == QDialog::Accepted)
						_presets->setProperties(index, editor.properties());
				}
			});
			edit_action->setEnabled(_presets->isUserWritable(index));
			menu.addAction(edit_action);
		}
		else {
			// Add new action
			auto add_action = new QAction(
					QIcon::fromTheme("document-new"),
					tr("Add new work detail in %1")
						.arg(index.data().toString()),
					&menu);
			connect(add_action, &QAction::triggered, this, [this, index = make_persistent(index)]() {
				addPresetWorkDetail(index);
			});
			add_action->setEnabled(_presets->isUserWritable(index));
			menu.addAction(add_action);
		}
	}
	auto selection = _ui->presets_view->selectionModel()->selectedRows();
	if (!selection.isEmpty()) {
		// Remove selection action
		auto remove_action = new QAction(&menu);
		remove_action->setIcon(QIcon::fromTheme("edit-delete"));
		if (selection.size() == 1) {
			auto index = selection.front();
			remove_action->setText(tr("Remove %1").arg(index.data().toString()));
			connect(remove_action, &QAction::triggered, this,
				[this, index = make_persistent(index)]() {
					removePreset(index);
				});
		}
		else {
			remove_action->setText(tr("Remove selected presets"));
			connect(remove_action, &QAction::triggered, this,
				[this, indexes = make_persistent(selection)]() {
					removePresets(indexes);
				});
		}
		remove_action->setEnabled(std::ranges::all_of(selection,
				[this](const auto &index) {
					return _presets->isUserWritable(index);
				}));
		menu.addAction(remove_action);
		// Import selection action
		auto import_action = new QAction(
				QIcon::fromTheme("document-import"),
				tr("Import"),
				&menu);
		connect(import_action, &QAction::triggered, this,
			[this, indexes = make_persistent(selection)] {
				for (const auto &index: indexes) {
					for (const auto &properties: _presets->properties(index))
						_df->work_details->add(properties);
				}
			});
		menu.addAction(import_action);
	}
	// Add new action
	auto add_action = new QAction(
			QIcon::fromTheme("document-new"),
			tr("Add new preset"),
			&menu);
	connect(add_action, &QAction::triggered, this, &WorkDetailManager::addPreset);
	menu.addAction(add_action);
	// Show menu
	if (!menu.isEmpty())
		menu.exec(_ui->presets_view->mapToGlobal(pos));
}

void WorkDetailManager::addWorkDetail()
{
	WorkDetailEditor editor(this);
	editor.setName(tr("New work detail"));
	editor.setMode(df::work_detail_mode::EverybodyDoesThis);
	editor.setIcon(df::work_detail_icon::ICON_NONE);
	if (QDialog::Accepted == editor.exec()) {
		_df->work_details->add(editor.properties());
	}
}

void WorkDetailManager::removeSelectedWorkDetails()
{
	removeWorkDetails(make_persistent(_ui->workdetails_view->selectionModel()->selectedRows()));
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

void WorkDetailManager::exportWorkDetails(const QList<QPersistentModelIndex> &indexes, const QModelIndex &dest)
{
	QString name;
	if (dest.isValid()) {
		name = dest.data().toString();
	}
	else {
		name = QInputDialog::getText(this,
				tr("New work detail preset"),
				tr("New work detail preset name:"));
		if (name.isEmpty())
			return;
	}
	std::vector<WorkDetail::Properties> properties;
	for (const auto &wd_idx: indexes) {
		auto wd = _df->work_details->get(wd_idx.row());
		properties.push_back(WorkDetail::Properties::fromWorkDetail(**wd));
	}
	bool success = dest.isValid()
		? _presets->add(std::move(properties), dest)
		: _presets->add(std::move(properties), name);
	if (!success) {
		QMessageBox::critical(this,
				tr("Export as work detail preset"),
				tr("Failed to add work details to %1").arg(name));
	}
}

void WorkDetailManager::copyWorkDetails() const
{
	auto clipboard = QGuiApplication::clipboard();
	auto selection = _ui->workdetails_view->selectionModel()->selectedRows();
	if (!selection.isEmpty())
		clipboard->setMimeData(_df->work_details->mimeData(selection));
}

void WorkDetailManager::pasteWorkDetails()
{
	auto clipboard = QGuiApplication::clipboard();
	auto current = _ui->workdetails_view->currentIndex();
	_df->work_details->dropMimeData(clipboard->mimeData(), Qt::CopyAction, current.row(), 0, {});
}

void WorkDetailManager::moveTop()
{
	auto selection = _ui->workdetails_view->selectionModel()->selectedRows();
	_df->work_details->move(make_persistent(selection), 0);
}

void WorkDetailManager::moveUp()
{
	auto selection = _ui->workdetails_view->selectionModel()->selectedRows();
	std::ranges::sort(selection, {}, &QModelIndex::row); // Is this necessary?
	int row = 0;
	for (const auto &index: selection)
		if (index.row() != row++)
			_df->work_details->move({index}, index.row()-1);
}

void WorkDetailManager::moveDown()
{
	auto selection = _ui->workdetails_view->selectionModel()->selectedRows();
	std::ranges::sort(selection, std::greater{}, &QModelIndex::row);
	int row = _df->work_details->rowCount()-1;
	for (const auto &index: selection)
		if (index.row() != row--)
			_df->work_details->move({index}, index.row()+2);
}

void WorkDetailManager::moveBottom()
{
	auto selection = _ui->workdetails_view->selectionModel()->selectedRows();
	_df->work_details->move(make_persistent(selection), -1);
}

void WorkDetailManager::addPreset()
{
	auto name = QInputDialog::getText(this,
			tr("New work detail preset"),
			tr("New work detail preset name:"));
	if (!name.isEmpty())
		_presets->add({}, name);
}

void WorkDetailManager::addPresetWorkDetail(const QPersistentModelIndex &index)
{
	WorkDetailEditor editor(this);
	editor.setName(tr("New work detail"));
	editor.setMode(df::work_detail_mode::EverybodyDoesThis);
	editor.setIcon(df::work_detail_icon::ICON_NONE);
	if (QDialog::Accepted == editor.exec()) {
		_presets->add({editor.properties()}, index);
	}
}

void WorkDetailManager::removeSelectedPresets()
{
	removePresets(make_persistent(_ui->presets_view->selectionModel()->selectedRows()));
}

void WorkDetailManager::removePreset(const QPersistentModelIndex &index)
{
	if (!index.isValid())
		return;
	QMessageBox question(this);
	question.setIcon(QMessageBox::Question);
	question.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	if (_presets->isWorkDetail(index)) {
		auto preset = _presets->preset(index.parent());
		Q_ASSERT(preset);
		question.setWindowTitle(tr("Removing %1").arg(index.data().toString()));
		question.setText(tr("Are you sure you want to remove the work detail \"%1\"?")
				.arg(index.data().toString()));
		question.setInformativeText(tr("This work detail belongs to the preset \"%1\" from \"%2\".")
				.arg(preset->id)
				.arg(preset->file_info.absoluteFilePath()));
	}
	else { // Preset file
		auto preset = _presets->preset(index);
		Q_ASSERT(preset);
		question.setWindowTitle(tr("Removing %1").arg(preset->id));
		question.setText(tr("Are you sure you want to remove the preset \"%1\"?")
					.arg(index.data().toString()));
		question.setInformativeText(tr("File \"%1\" will be removed")
				.arg(preset->file_info.absoluteFilePath()));
	}
	if (question.exec() == QMessageBox::Yes)
		if (index.isValid())
			_presets->remove(index);
}

void WorkDetailManager::removePresets(const QList<QPersistentModelIndex> &indexes)
{
	if (indexes.size() == 1) {
		removePreset(indexes.front());
		return;
	}
	QMessageBox question(this);
	question.setIcon(QMessageBox::Question);
	question.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	question.setWindowTitle(tr("Removing multiple work detail presets"));
	question.setText(tr("Are you sure you want to remove the following work detail presets?"));
	QStringList info;
	info.reserve(indexes.size());
	for (auto index: indexes) {
		if (!index.isValid())
			continue;
		if (_presets->isWorkDetail(index)) {
			auto preset = _presets->preset(index.parent());
			Q_ASSERT(preset);
			info.push_back(tr("Work detail %1 from %2 (%3)")
					.arg(index.data().toString())
					.arg(preset->id)
					.arg(preset->file_info.absoluteFilePath()));
		}
		else {
			auto preset = _presets->preset(index);
			Q_ASSERT(preset);
			info.push_back(tr("Preset %1 (%2)")
					.arg(preset->id)
					.arg(preset->file_info.absoluteFilePath()));
		}
	}
	question.setInformativeText(info.join("\n"));
	if (question.exec() == QMessageBox::Yes)
		for (const auto &index: indexes)
			if (index.isValid())
				_presets->remove(index);
}

void WorkDetailManager::copyPresets() const
{
	auto clipboard = QGuiApplication::clipboard();
	auto selection = _ui->presets_view->selectionModel()->selectedRows();
	if (!selection.isEmpty())
		clipboard->setMimeData(_presets->mimeData(selection));
}

void WorkDetailManager::cutPresets()
{
	auto clipboard = QGuiApplication::clipboard();
	auto selection = _ui->presets_view->selectionModel()->selectedRows();
	if (!selection.isEmpty()) {
		clipboard->setMimeData(_presets->mimeData(selection));
		for (const auto &index: selection)
			_presets->remove(index);
	}
}

void WorkDetailManager::pastePresets()
{
	auto clipboard = QGuiApplication::clipboard();
	auto current = _ui->presets_view->currentIndex();
	QModelIndex parent;
	int row;
	if (_presets->isWorkDetail(current)) {
		parent = current.parent();
		row = current.row();
	}
	else {
		parent = current;
		row = -1;
	}
	_presets->dropMimeData(clipboard->mimeData(), Qt::CopyAction, row, 0, parent);
}
