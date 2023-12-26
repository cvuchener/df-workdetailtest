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

#ifndef WORK_DETAIL_MANAGER_H
#define WORK_DETAIL_MANAGER_H

#include <QDialog>

namespace Ui { class WorkDetailManager; }
class DwarfFortressData;
class WorkDetailPresets;

// Force copy action when dragging/dropping from other widgets
template <typename T>
class ExternalCopyView: public T
{
public:
	ExternalCopyView(QWidget *parent);
	~ExternalCopyView() override;

protected:
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dropEvent(QDropEvent *event) override;
};

#include <QListView>

class WorkDetailView: public ExternalCopyView<QListView>
{
	Q_OBJECT
public:
	WorkDetailView(QWidget *parent = nullptr);
	~WorkDetailView() override;

protected:
	bool edit(const QModelIndex &index, QAbstractItemView::EditTrigger trigger, QEvent *event) override;
};

#include <QTreeView>

class WorkDetailPresetView: public ExternalCopyView<QTreeView>
{
	Q_OBJECT
public:
	WorkDetailPresetView(QWidget *parent = nullptr);
	~WorkDetailPresetView() override;

protected:
	bool edit(const QModelIndex &index, QAbstractItemView::EditTrigger trigger, QEvent *event) override;
};

class WorkDetailManager: public QDialog
{
	Q_OBJECT
public:
	WorkDetailManager(std::shared_ptr<DwarfFortressData> df, QWidget *parent = nullptr, Qt::WindowFlags flags = {});
	~WorkDetailManager() override;

private slots:
	void on_workdetails_view_customContextMenuRequested(const QPoint &);
	void on_presets_view_customContextMenuRequested(const QPoint &);

private:
	void addWorkDetail();
	void removeSelectedWorkDetails();
	void removeWorkDetail(const QPersistentModelIndex &index);
	void removeWorkDetails(const QList<QPersistentModelIndex> &indexes);
	void exportWorkDetails(const QList<QPersistentModelIndex> &indexes, const QModelIndex &dest);
	void copyWorkDetails() const;
	void pasteWorkDetails();
	void moveTop();
	void moveUp();
	void moveDown();
	void moveBottom();
	void addPreset();
	void addPresetWorkDetail(const QPersistentModelIndex &index);
	void removeSelectedPresets();
	void removePreset(const QPersistentModelIndex &index);
	void removePresets(const QList<QPersistentModelIndex> &indexes);
	void copyPresets() const;
	void cutPresets();
	void pastePresets();
	std::unique_ptr<Ui::WorkDetailManager> _ui;
	std::shared_ptr<DwarfFortressData> _df;
	std::unique_ptr<WorkDetailPresets> _presets;
	struct Shortcuts;
	std::unique_ptr<Shortcuts> _shorcuts;
};

#endif
