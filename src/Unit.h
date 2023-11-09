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

#ifndef UNIT_H
#define UNIT_H

#include <QObject>
#include "DFTypes.h"
#include <QCoroTask>

class DwarfFortress;

class Unit: public QObject, public std::enable_shared_from_this<Unit>
{
	Q_OBJECT
public:
	Unit(std::unique_ptr<df::unit> &&unit, DwarfFortress &df, QObject *parent = nullptr);
	~Unit() override;

	using df_type = df::unit;
	static inline constexpr auto sorted_key = &df::unit::id;

	void update(std::unique_ptr<df::unit> &&unit);

	const df::unit *get() const { return _u.get(); }
	const df::unit &operator*() const { return *_u; }
	const df::unit *operator->() const { return _u.get(); }

	const QString &displayName() const { return _display_name; }

	const df::creature_raw &creature_raw() const;
	const df::caste_raw &caste_raw() const;

	bool isFortControlled() const;
	bool isCrazed() const;
	bool isOpposedToLife() const;
	bool isOwnGroup() const;
	bool canLearn() const;
	bool canAssignWork() const;
	bool isTamable() const;
	bool isBaby() const;
	bool isChild() const;
	bool isAdult() const;
	bool hasMenialWorkExemption() const;

	struct Properties {
		std::optional<QString> nickname;
		std::optional<bool> only_do_assigned_jobs;
	};
	QCoro::Task<> edit(Properties properties);

signals:
	void aboutToBeUpdated();
	void updated();

private:
	void refresh();

	std::unique_ptr<df::unit> _u;
	DwarfFortress &_df;

	QString _display_name;
};

#endif
