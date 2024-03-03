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

#include "Material.h"

#include "df/raws.h"
#include "df/types.h"
#include "df/utils.h"

static constexpr std::size_t CreatureBase = 19;
static constexpr std::size_t HistFigureBase = 219;
static constexpr std::size_t PlantBase = 419;
static constexpr std::size_t MaxMaterialType = 200;

Material::Material(const DwarfFortressData &df, int type, int index):
	_df(df)
{
	std::size_t creature_mat = type - CreatureBase;
	if (creature_mat < MaxMaterialType) {
		_category = Category::Creature;
		_index = index;
		_subindex = creature_mat;
		return;
	}

	std::size_t histfig_mat = type - HistFigureBase;
	if (histfig_mat < MaxMaterialType) {
		_category = Category::HistoricalFigure;
		_index = index;
		_subindex = histfig_mat;
		return;
	}

	std::size_t plant_mat = type - PlantBase;
	if (plant_mat < MaxMaterialType) {
		_category = Category::Plant;
		_index = index;
		_subindex = plant_mat;
		return;
	}

	if (type == df::builtin_mats::INORGANIC) {
		_category = Category::Inorganic;
		_index = index;
		_subindex = -1;
		return;
	}

	if (type >= 0) {
		_category = Category::Builtin;
		_index = type;
		_subindex = index;
	}
	else {
		_category = Category::Invalid;
		_index = -1;
		_subindex = -1;
	}
}

const df::material *Material::get() const
{
	if (!_df.raws)
		return nullptr;

	switch (_category) {
	case Category::Builtin:
		return df::get(_df.raws->builtin_mats, _index);
	case Category::Inorganic:
		if (auto inorganic = df::get(_df.raws->inorganics, _index))
			return &inorganic->material;
		else
			return _df.raws->builtin_mats[df::builtin_mats::INORGANIC].get();
	case Category::Creature:
		if (auto creature = df::get(_df.raws->creatures.all, _index))
			if (auto material = df::get(creature->material, _subindex))
				return material;
		return _df.raws->builtin_mats[CreatureBase].get();
	case Category::HistoricalFigure:
		if (auto histfig = df::find(_df.histfigs, _index)) {
			if (auto creature = df::get(_df.raws->creatures.all, histfig->race))
				if (auto material = df::get(creature->material, _subindex))
					return material;
		}
		return _df.raws->builtin_mats[CreatureBase].get();
	case Category::Plant:
		if (auto plant = df::get(_df.raws->plants.all, _index))
			if (auto material = df::get(plant->material, _subindex))
				return material;
		return _df.raws->builtin_mats[PlantBase].get();
	default:
		return nullptr;
	}
}

std::optional<df::builtin_mats_t> Material::builtin() const
{
	switch (_category) {
	case Category::Builtin:
		if (unsigned(_index) < df::builtin_mats::Count)
			return static_cast<df::builtin_mats_t>(_index);
		[[fallthrough]];
	default:;
		return std::nullopt;
	}
}

const df::inorganic_raw *Material::inorganic() const
{
	if (!_df.raws)
		return nullptr;

	switch (_category) {
	case Category::Inorganic:
		return df::get(_df.raws->inorganics, _index);
	default:
		return nullptr;
	}
}

const df::historical_figure *Material::historicalFigure() const
{
	if (!_df.raws)
		return nullptr;

	switch (_category) {
	case Category::HistoricalFigure:
		return df::find(_df.histfigs, _index);
	default:
		return nullptr;
	}
}

const df::creature_raw *Material::creature() const
{
	if (!_df.raws)
		return nullptr;

	switch (_category) {
	case Category::Creature:
		return df::get(_df.raws->creatures.all, _index);
	case Category::HistoricalFigure:
		if (auto histfig = df::find(_df.histfigs, _index))
			return df::get(_df.raws->creatures.all, histfig->race);
		else
			return nullptr;
	default:
		return nullptr;
	}
}

const df::plant_raw *Material::plant() const
{
	if (!_df.raws)
		return nullptr;

	switch (_category) {
	case Category::Plant:
		return df::get(_df.raws->plants.all, _index);
	default:
		return nullptr;
	}
}

template <Material::StringType Type>
QString Material::toString(df::matter_state_t state) const
{
	QString out;
	if (auto hf = historicalFigure()) {
		out.append(_df.raws->language.translate_name(hf->name));
		out.append("'s ");
	}
	if (auto material = get()) {
		if (!material->prefix.empty()) {
			out.append(df::fromCP437(material->prefix));
			out.append(" ");
		}
		if constexpr (Type == Material::Name)
			out.append(df::fromCP437(material->state_name[state]));
		else if constexpr (Type == Material::Adjective)
			out.append(df::fromCP437(material->state_adj[state]));
		else
			static_assert(false, "Missing type");
	}
	else
		out.append("unknown material");
	return out;
}

template QString Material::toString<Material::Name>(df::matter_state_t) const;
template QString Material::toString<Material::Adjective>(df::matter_state_t) const;
