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

#include "Unit.h"

#include "DwarfFortress.h"
#include "CP437.h"
#include <QCoroFuture>
#include "DFUtils.h"

#include <dfhack-client-qt/Function.h>

static const DFHack::Function<
	dfproto::workdetailtest::UnitProperties,
	dfproto::workdetailtest::Result
> EditUnit = {"workdetailtest", "EditUnit"};

Unit::Unit(std::unique_ptr<df::unit> &&unit, DwarfFortress &df, QObject *parent):
	QObject(parent),
	_u(std::move(unit)),
	_df(df)
{
	refresh();
}

Unit::~Unit()
{
}

void Unit::update(std::unique_ptr<df::unit> &&unit)
{
	aboutToBeUpdated();
	_u = std::move(unit);
	refresh();
	updated();
}

void Unit::refresh()
{

	auto hf = _df.findHistoricalFigure(_u->hist_figure_id);
	auto identity = hf && hf->info && hf->info->reputation
		? _df.findIdentity(hf->info->reputation->cur_identity)
		: nullptr;
	if (identity)
		_display_name = _df.raws().language.translate_name(identity->name);
	else
		_display_name = _df.raws().language.translate_name(_u->name);
	if (_display_name.isEmpty()) {
		const auto &caste = caste_raw();
		if (isBaby())
			_display_name = fromCP437(caste.baby_name[0]);
		else if (isChild())
			_display_name = fromCP437(caste.child_name[0]);
		else
			_display_name = fromCP437(caste.caste_name[0]);
	}
	if (_display_name.isEmpty()) {
		const auto &creature = creature_raw();
		if (isBaby())
			_display_name = fromCP437(creature.general_baby_name[0]);
		else if (isChild())
			_display_name = fromCP437(creature.general_child_name[0]);
		else
			_display_name = fromCP437(creature.name[0]);
	}
}

const df::creature_raw &Unit::creature_raw() const
{
	return *_df.raws().creatures.all.at(_u->race);
}

const df::caste_raw &Unit::caste_raw() const
{
	return *creature_raw().caste.at(_u->caste);
}

bool Unit::isFortControlled() const
{
	// if (gamemode != DWARF) return false;
	if (_u->mood == df::mood_type::Berserk
			|| isCrazed()
			|| isOpposedToLife()
			|| _u->undead
			|| _u->flags3.bits.ghostly)
		return false;
	if (_u->flags1.bits.marauder
			|| _u->flags1.bits.invader_origin
			|| _u->flags1.bits.active_invader
			|| _u->flags1.bits.forest
			|| _u->flags1.bits.merchant
			|| _u->flags1.bits.diplomat)
		return false;
	if (_u->flags1.bits.tame)
		return true;
	if (_u->flags2.bits.visitor
			|| _u->flags2.bits.visitor_uninvited
			|| _u->flags2.bits.underworld
			|| _u->flags2.bits.resident
			|| _u->flags4.bits.agitated_wilderness_creature)
		return false;
	return _u->civ_id != -1 && _u->civ_id == _df.currentCivId();
}

bool Unit::isCrazed() const
{
	if (_u->flags3.bits.scuttle)
		return false;
	if (_u->curse.rem_tags1.bits.CRAZED)
		return false;
	if (_u->curse.add_tags1.bits.CRAZED)
		return true;
	return caste_raw().flags.isSet(df::caste_raw_flags::CRAZED);
}

bool Unit::isOpposedToLife() const
{
	if (_u->curse.rem_tags1.bits.OPPOSED_TO_LIFE)
		return false;
	if (_u->curse.add_tags1.bits.OPPOSED_TO_LIFE)
		return true;
	return caste_raw().flags.isSet(df::caste_raw_flags::OPPOSED_TO_LIFE);
}

bool Unit::canLearn() const
{
	if (_u->curse.rem_tags1.bits.CAN_LEARN)
		return false;
	if (_u->curse.add_tags1.bits.CAN_LEARN)
		return true;
	return caste_raw().flags.isSet(df::caste_raw_flags::CAN_LEARN);
}

bool Unit::isOwnGroup() const
{
	if (auto hf = _df.findHistoricalFigure(_u->hist_figure_id))
		return std::ranges::any_of(hf->entity_links, [this](const auto &link) {
			return link->entity_id == _df.currentGroupId() &&
				link->type() == df::histfig_entity_link_type::MEMBER;
		});
	else
		return false;
}

bool Unit::canAssignWork() const
{
	if (_u->flags1.bits.inactive)
		return false;
	if (!isFortControlled())
		return false;
	if (isTamable())
		return false;
	if (hasMenialWorkExemption())
		return false;
	if (!isAdult())
		return false;
	if (_u->undead)
		return false;
	if (!canLearn())
		return false;
	if (!isOwnGroup() && std::ranges::any_of(_u->occupations, [](const auto &occ) {
				using namespace df::occupation_type;
				return occ->type == PERFORMER || occ->type == SCHOLAR;
			}))
		return false;
	if (std::ranges::any_of(_u->occupations, [](const auto &occ) {
				using namespace df::occupation_type;
				return occ->type == MERCENARY || occ->type == MONSTER_SLAYER;
			}))
		return false;
	return true;
}

bool Unit::isBaby() const
{
	return _u->profession == df::profession::BABY;
}

bool Unit::isChild() const
{
	return _u->profession == df::profession::CHILD;
}

bool Unit::isAdult() const
{
	return _u->profession != df::profession::BABY
		&& _u->profession != df::profession::CHILD;
}

bool Unit::isTamable() const
{
	const auto &caste_flags = caste_raw().flags;
	return caste_flags.isSet(df::caste_raw_flags::PET)
		|| caste_flags.isSet(df::caste_raw_flags::PET_EXOTIC);
}

bool Unit::hasMenialWorkExemption() const
{
	auto match_position = [](const DwarfFortress &df, const df::historical_figure &hf, df::entity_position_flags_t flag) {
		for (const auto &link: hf.entity_links) {
			auto el_pos = dynamic_cast<const df::histfig_entity_link_position *>(link.get());
			if (!el_pos)
				continue;
			auto entity = df.findHistoricalEntity(el_pos->entity_id);
			if (!entity || entity->id != df.currentGroupId())
				continue;
			auto assignment = df::find(entity->positions.assignments, el_pos->assignment_id);
			if (!assignment)
				continue;
			auto position = df::find(entity->positions.own, assignment->position_id);
			if (!position || !position->flags.isSet(flag))
				continue;
			return true;
		}
		return false;
	};
	if (auto hf = _df.findHistoricalFigure(_u->hist_figure_id)) {
		using namespace df::entity_position_flags;
		if (match_position(_df, *hf, MENIAL_WORK_EXEMPTION))
			return true;
		for (const auto &link: hf->histfig_links) {
			if (link->type() != df::histfig_hf_link_type::SPOUSE)
				continue;
			auto spouse_hf = _df.findHistoricalFigure(link->target);
			if (!spouse_hf)
				continue;
			if (match_position(_df, *spouse_hf, MENIAL_WORK_EXEMPTION_SPOUSE))
				return true;
		}
	}
	return false;
}

QCoro::Task<> Unit::edit(Properties changes)
{
	auto thisptr = shared_from_this(); // make sure the object live for the whole coroutine
	Q_ASSERT(thisptr);
	dfproto::workdetailtest::UnitProperties args;
	args.set_id(_u->id);
	if (changes.nickname) {
		args.set_nickname(toCP437(*changes.nickname));
	}
	if (changes.only_do_assigned_jobs) {
		args.set_only_do_assigned_jobs(*changes.only_do_assigned_jobs);
	}
	auto r = co_await EditUnit(_df.dfhack(), args).first;
	if (!r) {
		qCWarning(DFHackLog) << "editUnit failed" << make_error_code(r.cr).message();
		co_return;
	}
	if (!r->success()) {
		qCWarning(DFHackLog) << "editUnit failed" << r->error();
		co_return;
	}
	aboutToBeUpdated();
	if (changes.nickname) {
		_u->name.nickname = toCP437(*changes.nickname);
		refresh();
	}
	if (changes.only_do_assigned_jobs) {
		_u->flags4.bits.only_do_assigned_jobs = *changes.only_do_assigned_jobs;
	}
	updated();
}

