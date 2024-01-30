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

#include "SkillModel.h"

#include "Unit.h"
#include "DataRole.h"

using namespace UnitDetails;

SkillModel::SkillModel(const DwarfFortressData &df, QObject *parent):
	UnitDataModel(df, parent)
{
}

SkillModel::~SkillModel()
{
}

int SkillModel::rowCount(const QModelIndex &parent) const
{
	if (_u && (*_u)->current_soul)
		return (*_u)->current_soul->skills.size();
	else
		return 0;
}

int SkillModel::columnCount(const QModelIndex &parent) const
{
	return static_cast<int>(Column::Count);
}

QVariant SkillModel::data(const QModelIndex &index, int role) const
{
	auto skill = (*_u)->current_soul->skills.at(index.row()).get();
	switch (static_cast<Column>(index.column())) {
	case Column::Skill:
		switch (role) {
		case Qt::DisplayRole:
		case DataRole::SortRole:
			return QString::fromLocal8Bit(caption_noun(skill->id));
		default:
			return {};
		}
	case Column::Level:
		switch (role) {
		case Qt::DisplayRole:
			if (skill->rusty > 0)
				return QString("%1 (%2)")
					.arg(skill->rating)
					.arg(-std::min(skill->rusty, int(skill->rating)));
			else
				return skill->rating;
		case DataRole::SortRole:
			return df::unit_skill::cumulated_experience(skill->rating) + skill->experience;
		case Qt::TextAlignmentRole:
			return Qt::AlignCenter;
		case Qt::ToolTipRole: {
			auto capped_rating = std::min(skill->rating, df::skill_rating::Legendary);
			auto rating_str = QString::fromLocal8Bit(caption(capped_rating));
			switch (skill->rustLevel()) {
			case df::unit_skill::NotRusty:
				return rating_str;
			case df::unit_skill::Rusty:
				return tr("%1 (rusty)").arg(rating_str);
			case df::unit_skill::VeryRusty:
				return tr("%1 (very rusty)").arg(rating_str);
			}
		}
		default:
			return {};
		}
	case Column::Progress:
		switch (role) {
		case Qt::DisplayRole:
		case DataRole::SortRole:
			return (100 * skill->experience)
				/ df::unit_skill::experience_for_next_level(skill->rating);
		case Qt::ToolTipRole:
			return QString("%1/%2")
				.arg(skill->experience)
				.arg(df::unit_skill::experience_for_next_level(skill->rating));
		default:
			return {};
		}
	default:
		return {};
	}
}

QVariant SkillModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal)
		return {};
	if (role != Qt::DisplayRole)
		return {};
	switch (static_cast<Column>(section)) {
	case Column::Skill:
		return tr("Skill");
	case Column::Level:
		return tr("Level");
	case Column::Progress:
		return tr("Progress");
	default:
		return {};
	}
}
