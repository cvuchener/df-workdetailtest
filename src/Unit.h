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
#include "df/types.h"
#include <QCoroTask>

class DwarfFortressData;

namespace DFHack { class Client; }
namespace dfproto::workdetailtest {
class UnitId;
class UnitProperties;
class UnitResult;
}

class Unit: public QObject, public std::enable_shared_from_this<Unit>
{
	Q_OBJECT
public:
	Unit(std::unique_ptr<df::unit> &&unit, DwarfFortressData &df, QObject *parent = nullptr);
	~Unit() override;

	using df_type = df::unit;
	static inline constexpr auto sorted_key = &df::unit::id;

	void update(std::unique_ptr<df::unit> &&unit);

	const df::unit *get() const { return _u.get(); }
	const df::unit &operator*() const { return *_u; }
	const df::unit *operator->() const { return _u.get(); }

	const QString &displayName() const { return _display_name; }

	const df::creature_raw *creature() const;
	const df::caste_raw *caste() const;
	const df::identity *currentIdentity() const;

	template <std::same_as<df::caste_raw_flags_t>... Args>
	bool hasCasteFlag(Args... args) const
	{
		if (auto c = caste())
			return (c->flags.isSet(args) || ...);
		else
			return false;
	}

	df::time age() const;

	using attribute_t = std::variant<df::physical_attribute_type_t, df::mental_attribute_type_t>;

	template <typename T>
	const df::unit_attribute *attribute(T attr) const;
	template <typename T>
	int attributeValue(T attr) const;
	template <typename T>
	int attributeCasteRating(T attr) const;
	inline const df::unit_attribute *attribute(const attribute_t &attr) const
	{
		return visit([this](auto attr) { return attribute(attr); }, attr);
	}
	inline int attributeValue(const attribute_t &attr) const
	{
		return visit([this](auto attr) { return attributeValue(attr); }, attr);
	}
	inline int attributeCasteRating(const attribute_t &attr) const
	{
		return visit([this](auto attr) { return attributeCasteRating(attr); }, attr);
	}


	bool isFortControlled() const;
	bool isCrazed() const;
	bool isOpposedToLife() const;
	bool isOwnGroup() const;
	bool canLearn() const;
	bool canSpeak() const;
	bool canAssignWork() const;
	bool isTamable() const;
	bool isBaby() const;
	bool isChild() const;
	bool isAdult() const;
	bool hasMenialWorkExemption() const;
	bool canBeAdopted() const;
	bool canBeSlaughtered() const;
	bool canBeGelded() const;

	enum class Category {
		Citizens,
		PetsOrLivestock,
		Others,
		Dead,
		Invisible,
	};
	Category category() const;

	enum class Flag {
		OnlyDoAssignedJobs,
		AvailableForAdoption,
		MarkedForSlaugter,
		MarkedForGelding,
	};
	Q_ENUM(Flag)

	inline bool hasFlag(Flag flag) const {
		switch (flag) {
		case Flag::OnlyDoAssignedJobs:
			return _u->flags4.bits.only_do_assigned_jobs;
		case Flag::AvailableForAdoption:
			return _u->flags3.bits.available_for_adoption;
		case Flag::MarkedForSlaugter:
			return _u->flags2.bits.slaughter;
		case Flag::MarkedForGelding:
			return _u->flags3.bits.marked_for_gelding;
		}
		Q_UNREACHABLE();
	}

	inline bool canEdit(Flag flag) const {
		switch (flag) {
		case Flag::OnlyDoAssignedJobs:
			return canAssignWork();
		case Flag::AvailableForAdoption:
			return canBeAdopted();
		case Flag::MarkedForSlaugter:
			return canBeSlaughtered();
		case Flag::MarkedForGelding:
			return canBeGelded();
		}
		Q_UNREACHABLE();
	}

	struct Properties {
		std::optional<QString> nickname;
		std::map<Flag, bool> flags;

		void setArgs(dfproto::workdetailtest::UnitProperties &) const;
	};
	QCoro::Task<> edit(Properties properties);
	static QCoro::Task<> edit(std::shared_ptr<DwarfFortressData> df, std::vector<std::shared_ptr<Unit>> units, Properties properties);
	static QCoro::Task<> toggle(std::shared_ptr<DwarfFortressData> df, std::vector<std::shared_ptr<Unit>> units, Flag flag);

private:
	void refresh();
	void setProperties(const Properties &properties, const dfproto::workdetailtest::UnitResult &results);

	std::unique_ptr<df::unit> _u;
	DwarfFortressData &_df;

	QString _display_name;
};

extern template const df::unit_attribute *Unit::attribute<df::physical_attribute_type_t>(df::physical_attribute_type_t) const;
extern template const df::unit_attribute *Unit::attribute<df::mental_attribute_type_t>(df::mental_attribute_type_t) const;
extern template int Unit::attributeValue<df::physical_attribute_type_t>(df::physical_attribute_type_t) const;
extern template int Unit::attributeValue<df::mental_attribute_type_t>(df::mental_attribute_type_t) const;
extern template int Unit::attributeCasteRating<df::physical_attribute_type_t>(df::physical_attribute_type_t) const;
extern template int Unit::attributeCasteRating<df::mental_attribute_type_t>(df::mental_attribute_type_t) const;

#endif
