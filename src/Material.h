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

#ifndef MATERIAL_H
#define MATERIAL_H

#include "DwarfFortressData.h"
#include "df_enums.h"

class Material
{
public:
	enum class Category {
		Invalid,
		Builtin,
		Inorganic,
		Creature,
		HistoricalFigure,
		Plant,
	};

	Material(const DwarfFortressData &df, int type, int index);

	operator bool() const noexcept { return _category != Category::Invalid; }
	Category category() const noexcept { return _category; }
	int index() const noexcept { return _index; }
	int subindex() const noexcept { return _subindex; }

	const df::material *get() const;
	std::optional<df::builtin_mats_t> builtin() const;
	const df::inorganic_raw *inorganic() const;
	const df::historical_figure *historicalFigure() const;
	const df::creature_raw *creature() const;
	const df::plant_raw *plant() const;

	enum StringType {
		Name,
		Adjective,
	};
	template <StringType Type>
	QString toString(df::matter_state_t state = df::matter_state::Solid) const;
	QString name(df::matter_state_t state = df::matter_state::Solid) const {
		return toString<Name>(state);
	}
	QString adjective(df::matter_state_t state = df::matter_state::Solid) const {
		return toString<Adjective>(state);
	}

private:
	const DwarfFortressData &_df;
	Category _category;
	int _index, _subindex;
};

extern template QString Material::toString<Material::Name>(df::matter_state_t) const;
extern template QString Material::toString<Material::Adjective>(df::matter_state_t) const;

#endif
