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

#include "SkillsColumn.h"

#include "DwarfFortressData.h"
#include "Unit.h"
#include "DataRole.h"
#include "df/utils.h"
#include "LogCategory.h"

#include <QColor>
#include <QJsonObject>
#include <QJsonArray>


using namespace Columns;

SkillsColumn::SkillsColumn(std::span<const df::job_skill_t> skills, QObject *parent):
	AbstractColumn(parent),
	_sort{*this, SortBy::Rating, {
		{SortBy::Rating, tr("rating")},
		{SortBy::RatingWithRust, tr("rating with rust")},
		{SortBy::Experience, tr("experience")}}}
{
	_skills.resize(skills.size());
	std::ranges::copy(skills, _skills.begin());
}

SkillsColumn::~SkillsColumn()
{
}

int SkillsColumn::count() const
{
	return _skills.size();
}

QVariant SkillsColumn::headerData(int section, int role) const
{
	switch (role) {
	case Qt::DisplayRole:
		return QString::fromLocal8Bit(caption(_skills.at(section)));
	default:
		return {};
	}
}

QVariant SkillsColumn::unitData(int section, const Unit &unit, int role) const
{
	auto skill_id = _skills.at(section);
	const df::unit_skill *skill = nullptr;
	if (unit->current_soul)
		skill = df::find(unit->current_soul->skills, skill_id);
	switch (role) {
	case Qt::DisplayRole:
		if (skill)
			return static_cast<int>(skill->rating);
		return {};
	case DataRole::RatingRole:
		if (skill)
			return (1+skill->rating)/16.0;
		return {};
	case Qt::ToolTipRole:
		if (skill) {
			auto tooltip = QString("<h3>%1</h3>").arg(unit.displayName());
			tooltip += tr("<p>%1 %2 (%3)</p>")
				.arg(QString::fromLocal8Bit(caption(std::min(skill->rating, df::skill_rating::Legendary))))
				.arg(QString::fromLocal8Bit(caption_noun(skill->id)))
				.arg(static_cast<int>(skill->rating));
			tooltip += tr("<p>Experience: %1/%2</p>")
				.arg(skill->experience)
				.arg(df::unit_skill::experience_for_next_level(skill->rating));
			if (skill->rusty > 0)
				tooltip += tr("<p>Rust: %1 (%2)</p>")
					.arg(skill->rusty)
					.arg(QString::fromLocal8Bit(caption(static_cast<df::skill_rating_t>(std::clamp(skill->rating-skill->rusty, 0, 15)))));
			return tooltip;
		}
		return {};
	case DataRole::BorderRole:
		switch (skill ? skill->rustLevel() : df::unit_skill::NotRusty) {
		case df::unit_skill::Rusty:
			return QColor(255, 128, 0);
		case df::unit_skill::VeryRusty:
			return QColor(255, 0, 0);
		default:
			return {};
		}
	case DataRole::SortRole:
		if (skill) switch (_sort.option) {
			case SortBy::Rating:
				return skill->rating;
			case SortBy::RatingWithRust:
				return std::max(skill->rating - skill->rusty, 0);
			case SortBy::Experience:
				return df::unit_skill::cumulated_experience(skill->rating) + skill->experience;
		}
		else
			return -1;
	default:
		return {};
	}
}

void SkillsColumn::makeHeaderMenu(int section, QMenu *menu, QWidget *parent)
{
	_sort.makeSortMenu(menu);
}

Factory SkillsColumn::makeFactory(const QJsonObject &json)
{
	std::vector<df::job_skill_t> skills;
	for (auto value: json["skills"].toArray()) {
		auto skill_name = value.toString().toLocal8Bit();
		std::string_view skill_view(skill_name.data(), skill_name.size());
		if (auto skill = df::job_skill::from_string(skill_view))
			skills.push_back(*skill);
		else
			qCWarning(GridViewLog) << "Invalid skill value for Skills column" << skill_name;
	}
	return [skills = std::move(skills)](DwarfFortressData &df) {
		return std::make_unique<SkillsColumn>(skills);
	};
}
