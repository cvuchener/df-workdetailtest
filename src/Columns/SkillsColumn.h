/*
 * Copyright 2024 Clement Vuchener
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

#ifndef COLUMNS_SKILLS_COLUMN_H
#define COLUMNS_SKILLS_COLUMN_H

#include "AbstractColumn.h"
#include "Columns/Factory.h"
#include "Columns/SortOptions.h"
#include "df_enums.h"

class DwarfFortressData;

namespace Columns {

class SkillsColumn: public AbstractColumn
{
	Q_OBJECT
public:
	SkillsColumn(std::span<const df::job_skill_t> skills, QObject *parent = nullptr);
	~SkillsColumn() override;

	int count() const override;
	QVariant headerData(int section, int role = Qt::DisplayRole) const override;
	QVariant unitData(int section, const Unit &unit, int role = Qt::DisplayRole) const override;

	void makeHeaderMenu(int section, QMenu *menu, QWidget *parent) override;

	static Factory makeFactory(const QJsonObject &);

private:
	std::vector<df::job_skill_t> _skills;
	enum class SortBy {
		Rating,
		RatingWithRust,
		Experience,
	};
	SortOptions<SkillsColumn, SortBy> _sort;
};

}

#endif
