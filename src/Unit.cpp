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

#include "DwarfFortressData.h"
#include <QCoroFuture>
#include "df/utils.h"

#include <dfhack-client-qt/Function.h>
#include "workdetailtest.pb.h"

static const DFHack::Function<
	dfproto::workdetailtest::EditUnit,
	dfproto::workdetailtest::UnitResult
> EditUnit = {"workdetailtest", "EditUnit"};
static const DFHack::Function<
	dfproto::workdetailtest::EditUnits,
	dfproto::workdetailtest::UnitResults
> EditUnits = {"workdetailtest", "EditUnits"};

Unit::Unit(std::unique_ptr<df::unit> &&unit, DwarfFortressData &df, DFHack::Client &dfhack, QObject *parent):
	QObject(parent),
	_u(std::move(unit)),
	_df(df),
	_dfhack(&dfhack)
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
	using df::fromCP437;
	auto hf = df::find(_df.histfigs, _u->hist_figure_id);
	auto identity = hf && hf->info && hf->info->reputation
		? df::find(_df.identities, hf->info->reputation->cur_identity)
		: nullptr;
	if (!_df.raws) {
		_display_name = tr("Invalid raws");
		return;
	}
	if (identity)
		_display_name = _df.raws->language.translate_name(identity->name);
	else
		_display_name = _df.raws->language.translate_name(_u->name);
	if (_display_name.isEmpty()) {
		const auto &caste = caste_raw();
		if (isBaby())
			_display_name = fromCP437(caste->baby_name[0]);
		else if (isChild())
			_display_name = fromCP437(caste->child_name[0]);
		else
			_display_name = fromCP437(caste->caste_name[0]);
	}
	if (_display_name.isEmpty()) {
		const auto &creature = creature_raw();
		if (isBaby())
			_display_name = fromCP437(creature->general_baby_name[0]);
		else if (isChild())
			_display_name = fromCP437(creature->general_child_name[0]);
		else
			_display_name = fromCP437(creature->name[0]);
	}
}

const df::creature_raw *Unit::creature_raw() const
{
	if (!_df.raws || _u->race < 0 || unsigned(_u->race) > _df.raws->creatures.all.size())
		return nullptr;
	else
		return _df.raws->creatures.all[_u->race].get();
}

const df::caste_raw *Unit::caste_raw() const
{
	if (auto creature = creature_raw()) {
		if (_u->caste < 0 || unsigned(_u->caste) > creature->caste.size())
			return nullptr;
		else
			return creature->caste[_u->caste].get();
	}
	else
		return nullptr;
}

df::time Unit::age() const
{
	return _df.current_time - _u->birth_year - _u->birth_tick;
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
	return _u->civ_id != -1 && _u->civ_id == _df.current_civ_id;
}

bool Unit::isCrazed() const
{
	if (_u->flags3.bits.scuttle)
		return false;
	if (_u->curse.rem_tags1.bits.CRAZED)
		return false;
	if (_u->curse.add_tags1.bits.CRAZED)
		return true;
	return hasCasteFlag(df::caste_raw_flags::CRAZED);
}

bool Unit::isOpposedToLife() const
{
	if (_u->curse.rem_tags1.bits.OPPOSED_TO_LIFE)
		return false;
	if (_u->curse.add_tags1.bits.OPPOSED_TO_LIFE)
		return true;
	return hasCasteFlag(df::caste_raw_flags::OPPOSED_TO_LIFE);
}

bool Unit::canLearn() const
{
	if (_u->curse.rem_tags1.bits.CAN_LEARN)
		return false;
	if (_u->curse.add_tags1.bits.CAN_LEARN)
		return true;
	return hasCasteFlag(df::caste_raw_flags::CAN_LEARN);
}

bool Unit::canSpeak() const
{
	if (_u->curse.rem_tags1.bits.CAN_SPEAK)
		return false;
	if (_u->curse.add_tags1.bits.CAN_SPEAK)
		return true;
	return hasCasteFlag(df::caste_raw_flags::CAN_SPEAK);
}

bool Unit::isOwnGroup() const
{
	if (auto hf = df::find(_df.histfigs, _u->hist_figure_id))
		return std::ranges::any_of(hf->entity_links, [this](const auto &link) {
			return link->entity_id == _df.current_group_id &&
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
	return hasCasteFlag(
			df::caste_raw_flags::PET,
			df::caste_raw_flags::PET_EXOTIC);
}

bool Unit::hasMenialWorkExemption() const
{
	auto match_position = [](const DwarfFortressData &df, const df::historical_figure &hf, df::entity_position_flags_t flag) {
		for (const auto &link: hf.entity_links) {
			auto el_pos = dynamic_cast<const df::histfig_entity_link_position *>(link.get());
			if (!el_pos)
				continue;
			auto entity = df::find(df.entities, el_pos->entity_id);
			if (!entity || entity->id != df.current_group_id)
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
	if (auto hf = df::find(_df.histfigs, _u->hist_figure_id)) {
		using namespace df::entity_position_flags;
		if (match_position(_df, *hf, MENIAL_WORK_EXEMPTION))
			return true;
		for (const auto &link: hf->histfig_links) {
			if (link->type() != df::histfig_hf_link_type::SPOUSE)
				continue;
			auto spouse_hf = df::find(_df.histfigs, link->target);
			if (!spouse_hf)
				continue;
			if (match_position(_df, *spouse_hf, MENIAL_WORK_EXEMPTION_SPOUSE))
				return true;
		}
	}
	return false;
}

bool Unit::canBeAdopted() const
{
	if (!_u->flags1.bits.tame)
		return false;
	if (_u->pet_owner != -1)
		return false;
	return !hasCasteFlag(df::caste_raw_flags::ADOPTS_OWNER);
}

bool Unit::canBeSlaughtered() const
{
	return _u->pet_owner == -1;
}

bool Unit::canBeGelded() const
{
	if (_u->flags3.bits.ghostly || _u->flags3.bits.gelded || _u->undead)
		return false;
	return hasCasteFlag(df::caste_raw_flags::GELDABLE);
}

Unit::Category Unit::category() const
{
	if (_u->flags1.bits.left || _u->flags1.bits.incoming)
		return Category::Invisible;
	if (!isFortControlled() && _u->flags1.bits.hidden_in_ambush)
		return Category::Invisible;
	// TODO: check map visibility
	if (_u->flags1.bits.inactive | _u->flags3.bits.ghostly)
		return Category::Dead;
	if (!isFortControlled())
		return Category::Others;
	if (isTamable() || _u->undead)
		return Category::PetsOrLivestock;
	if (canSpeak())
		return Category::Citizens;
	else
		return Category::PetsOrLivestock;

}

static auto toProto(Unit::Flag flag)
{
	switch (flag) {
	case Unit::Flag::OnlyDoAssignedJobs:
		return dfproto::workdetailtest::OnlyDoAssignedJobs;
	case Unit::Flag::AvailableForAdoption:
		return dfproto::workdetailtest::AvailableForAdoption;
	case Unit::Flag::MarkedForSlaugter:
		return dfproto::workdetailtest::MarkedForSlaughter;
	case Unit::Flag::MarkedForGelding:
		return dfproto::workdetailtest::MarkedForGelding;
	}
	Q_UNREACHABLE();
}

static std::optional<Unit::Flag> fromProto(dfproto::workdetailtest::UnitFlag flag)
{
	switch (flag) {
	case dfproto::workdetailtest::OnlyDoAssignedJobs:
		return Unit::Flag::OnlyDoAssignedJobs;
	case dfproto::workdetailtest::AvailableForAdoption:
		return Unit::Flag::AvailableForAdoption;
	case dfproto::workdetailtest::MarkedForSlaughter:
		return Unit::Flag::MarkedForSlaugter;
	case dfproto::workdetailtest::MarkedForGelding:
		return Unit::Flag::MarkedForGelding;
	default:
		return {};
	}
}

void Unit::Properties::setArgs(dfproto::workdetailtest::UnitProperties &args) const
{
	if (nickname) {
		args.set_nickname(df::toCP437(*nickname));
	}
	for (auto [flag, value]: flags) {
		auto flag_arg = args.mutable_flags()->Add();
		flag_arg->set_value(value);
		flag_arg->set_flag(toProto(flag));
	}
}

Q_DECLARE_LOGGING_CATEGORY(DFHackLog);
void Unit::setProperties(const Properties &properties, const dfproto::workdetailtest::UnitResult &results)
{
	aboutToBeUpdated();
	if (properties.nickname) {
		_u->name.nickname = df::toCP437(*properties.nickname);
		refresh();
	}
	for (const auto &flag_result: results.flags()) {
		auto flag = fromProto(flag_result.flag());
		if (!flag) {
			qCWarning(DFHackLog) << "Unknown flag in result" << flag_result.flag();
			continue;
		}
		auto it = properties.flags.find(*flag);
		if (it == properties.flags.end()) {
			qCWarning(DFHackLog) << "Unexpected flag in result" << *flag;
			continue;
		}
		if (!flag_result.result().success()) {
			qCWarning(DFHackLog) << "Unit change failed" << flag_result.result().error();
			continue;
		}
		switch (*flag) {
		case Flag::OnlyDoAssignedJobs:
			_u->flags4.bits.only_do_assigned_jobs = it->second;
			break;
		case Flag::AvailableForAdoption:
			_u->flags3.bits.available_for_adoption = it->second;
			if (it->second)
				_u->flags2.bits.slaughter = false;
			break;
		case Flag::MarkedForSlaugter:
			_u->flags2.bits.slaughter = it->second;
			if (it->second) {
				_u->flags3.bits.available_for_adoption = false;
				_u->flags3.bits.marked_for_gelding = false;
			}
			break;
		case Flag::MarkedForGelding:
			_u->flags3.bits.marked_for_gelding = it->second;
			if (it->second)
				_u->flags2.bits.slaughter = false;
			break;
		}
	}
	updated();
}

QCoro::Task<> Unit::edit(Properties changes)
{
	auto thisptr = shared_from_this(); // make sure the object live for the whole coroutine
	Q_ASSERT(thisptr);
	// Prepare arguments
	dfproto::workdetailtest::EditUnit args;
	args.mutable_id()->set_id(_u->id);
	changes.setArgs(*args.mutable_changes());
	// Call
	if (!_dfhack) {
		qCWarning(DFHackLog) << "DFHack client was deleted";
		co_return;
	}
	auto r = co_await EditUnit(*_dfhack, args).first;
	// Check results
	if (!r) {
		qCWarning(DFHackLog) << "EditUnit failed" << make_error_code(r.cr).message();
		co_return;
	}
	if (!r->unit().success()) {
		qCWarning(DFHackLog) << "EditUnit failed" << r->unit().error();
		co_return;
	}
	setProperties(changes, *r);
}

QCoro::Task<> Unit::edit(QPointer<DFHack::Client> dfhack, std::vector<std::shared_ptr<Unit>> units, Properties changes)
{
	// Prepare arguments
	dfproto::workdetailtest::EditUnits args;
	for (auto &unit: units) {
		auto edit = args.mutable_units()->Add();
		edit->mutable_id()->set_id((*unit)->id);
		changes.setArgs(*edit->mutable_changes());
	}
	// Call
	if (!dfhack) {
		qCWarning(DFHackLog) << "DFHack client was deleted";
		co_return;
	}
	auto r = co_await EditUnits(*dfhack, args).first;
	// Check results
	if (!r) {
		qCWarning(DFHackLog) << "EditUnit failed" << make_error_code(r.cr).message();
		co_return;
	}
	for (std::size_t i = 0; i < units.size(); ++i) {
		auto unit_result = r->results(i);
		if (!unit_result.unit().success()) {
			qCWarning(DFHackLog) << "EditUnit failed" << unit_result.unit().error();
			co_return;
		}
		units[i]->setProperties(changes, unit_result);
	}
}

QCoro::Task<> Unit::toggle(QPointer<DFHack::Client> dfhack, std::vector<std::shared_ptr<Unit>> units, Flag flag)
{
	// Prepare arguments
	std::vector<Properties> changes(units.size());
	dfproto::workdetailtest::EditUnits args;
	for (std::size_t i = 0; i < units.size(); ++i) {
		changes[i].flags[flag] = !units[i]->hasFlag(flag);
		auto edit = args.mutable_units()->Add();
		edit->mutable_id()->set_id((*units[i])->id);
		changes[i].setArgs(*edit->mutable_changes());
	}
	// Call
	if (!dfhack) {
		qCWarning(DFHackLog) << "DFHack client was deleted";
		co_return;
	}
	auto r = co_await EditUnits(*dfhack, args).first;
	// Check results
	if (!r) {
		qCWarning(DFHackLog) << "EditUnit failed" << make_error_code(r.cr).message();
		co_return;
	}
	for (std::size_t i = 0; i < units.size(); ++i) {
		auto unit_result = r->results(i);
		if (!unit_result.unit().success()) {
			qCWarning(DFHackLog) << "EditUnit failed" << unit_result.unit().error();
			co_return;
		}
		units[i]->setProperties(changes[i], unit_result);
	}
}
