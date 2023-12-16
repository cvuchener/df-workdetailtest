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

#ifndef UNIT_SCRIPT_WRAPPER_H
#define UNIT_SCRIPT_WRAPPER_H

#include <QObject>

#include "df_enums.h"

class Unit;

class UnitScriptWrapper: public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString name READ displayName)
	Q_PROPERTY(QString race_name READ raceName)
	Q_PROPERTY(QString caste_name READ casteName)
	Q_PROPERTY(df::profession_t profession READ profession)
public:
	UnitScriptWrapper(); // test dummy constructor
	UnitScriptWrapper(const Unit &unit);
	~UnitScriptWrapper() override;

	QString displayName() const;
	QString raceName() const;
	QString casteName() const;
	df::profession_t profession() const;

	Q_INVOKABLE bool isFortControlled() const;
	Q_INVOKABLE bool isCrazed() const;
	Q_INVOKABLE bool isOpposedToLife() const;
	Q_INVOKABLE bool isOwnGroup() const;
	Q_INVOKABLE bool canLearn() const;
	Q_INVOKABLE bool canAssignWork() const;
	Q_INVOKABLE bool isTamable() const;
	Q_INVOKABLE bool isBaby() const;
	Q_INVOKABLE bool isChild() const;
	Q_INVOKABLE bool isAdult() const;
	Q_INVOKABLE bool hasMenialWorkExemption() const;

private:
	std::shared_ptr<const Unit> _unit;
};

#endif
