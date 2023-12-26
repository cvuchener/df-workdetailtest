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

#ifndef WORK_DETAIL_EDITOR_H
#define WORK_DETAIL_EDITOR_H

#include <QDialog>

namespace Ui { class WorkDetailEditor; }

#include "df_enums.h"
#include "WorkDetail.h"

class LaborModel;

class WorkDetailEditor: public QDialog
{
	Q_OBJECT
public:
	WorkDetailEditor(QWidget *parent = nullptr, Qt::WindowFlags f = {});
	~WorkDetailEditor() override;

	QString name() const;
	void setName(const QString &);
	df::work_detail_mode_t mode() const;
	void setMode(df::work_detail_mode_t mode);
	df::work_detail_icon_t icon() const;
	void setIcon(df::work_detail_icon_t icon);
	LaborModel &labors() { return *_labors; }
	const LaborModel &labors() const { return *_labors; }

	void initFromWorkDetail(const WorkDetail &wd);
	void initFromProperties(const WorkDetail::Properties &properties);
	WorkDetail::Properties properties() const;

private:
	std::unique_ptr<Ui::WorkDetailEditor> _ui;
	std::unique_ptr<LaborModel> _labors;
};

#endif
