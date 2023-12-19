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

class WorkDetailManager: public QDialog
{
	Q_OBJECT
public:
	WorkDetailManager(std::shared_ptr<DwarfFortressData> df, QWidget *parent = nullptr, Qt::WindowFlags flags = {});
	~WorkDetailManager() override;

private slots:
	void on_workdetails_view_customContextMenuRequested(const QPoint &);

private:
	void addWorkDetail();
	void removeWorkDetail(const QPersistentModelIndex &index);
	void removeWorkDetails(const QList<QPersistentModelIndex> &indexes);
	std::unique_ptr<Ui::WorkDetailManager> _ui;
	std::shared_ptr<DwarfFortressData> _df;
};

#endif
