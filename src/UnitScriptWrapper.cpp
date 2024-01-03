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

#include "UnitScriptWrapper.h"

#include "Unit.h"
#include "df/utils.h"

UnitScriptWrapper::UnitScriptWrapper()
{
}

UnitScriptWrapper::UnitScriptWrapper(const Unit &unit):
	_unit(unit.shared_from_this())
{
}

UnitScriptWrapper::~UnitScriptWrapper()
{
}

#define MAKE_WRAPPER_METHOD(type, name) \
type UnitScriptWrapper::name() const \
{ \
	if (_unit) \
		return _unit->name(); \
	else \
		return {}; \
}

MAKE_WRAPPER_METHOD(QString, displayName)
MAKE_WRAPPER_METHOD(bool, isFortControlled)
MAKE_WRAPPER_METHOD(bool, isCrazed)
MAKE_WRAPPER_METHOD(bool, isOpposedToLife)
MAKE_WRAPPER_METHOD(bool, isOwnGroup)
MAKE_WRAPPER_METHOD(bool, canLearn)
MAKE_WRAPPER_METHOD(bool, canSpeak)
MAKE_WRAPPER_METHOD(bool, canAssignWork)
MAKE_WRAPPER_METHOD(bool, isTamable)
MAKE_WRAPPER_METHOD(bool, isBaby)
MAKE_WRAPPER_METHOD(bool, isChild)
MAKE_WRAPPER_METHOD(bool, isAdult)
MAKE_WRAPPER_METHOD(bool, hasMenialWorkExemption)

QString UnitScriptWrapper::raceName() const
{
	if (!_unit)
		return {};
	if (auto creature = _unit->creature_raw())
		return df::fromCP437(creature->name[0]);
	else
		return {};
}

QString UnitScriptWrapper::casteName() const
{
	if (!_unit)
		return {};
	if (auto caste = _unit->caste_raw())
		return df::fromCP437(caste->caste_name[0]);
	else
		return {};
}

df::profession_t UnitScriptWrapper::profession() const
{
	if (!_unit)
		return {};
	else
		return (*_unit)->profession;
}

