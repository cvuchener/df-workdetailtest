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

#include "Preference.h"

#include "df/types.h"
#include "df/utils.h"
#include "Material.h"
#include "Item.h"

#include <QCoreApplication>

static QString material_preference_string(const DwarfFortressData &df, Material material, df::matter_state_t state)
{
	auto str = material.toString<Material::Name>(state);
	auto m = material.get();
	if (material.category() == Material::Category::Plant && m) {
		if (m->flags.isSet(df::material_flags::WOOD))
			str.append(" wood");
		else if (m->flags.isSet(df::material_flags::THREAD_PLANT))
			str.append(" fabric");
	}
	return str;
}

QString Preference::toString(const DwarfFortressData &df, const df::unit_preference &p)
{
	switch (p.type) {
	case df::unit_preference_type::LikeMaterial:
		return material_preference_string(df, {df, p.mat_type, p.mat_index}, p.mat_state);
	case df::unit_preference_type::LikeCreature:
		if (auto creature = df.creature(p.id.creature_id))
			return df::fromCP437(creature->name[1]);
		return "Unknown creature";
	case df::unit_preference_type::LikeFood:
		return Item::toString(df, p.id.item_type, p.item_subtype, p.mat_type, p.mat_index, true);
	case df::unit_preference_type::HateCreature:
		if (auto creature = df.creature(p.id.creature_id))
			return df::fromCP437(creature->name[1]);
		return "Unknown creature";
	case df::unit_preference_type::LikeItem:
		return Item::toString(df, p.id.item_type, p.item_subtype, -1, -1, true);
	case df::unit_preference_type::LikePlant:
		if (auto plant = df.plant(p.id.plant_id))
			return df::fromCP437(plant->name_plural);
		return "Unknown plant";
	case df::unit_preference_type::LikeTree:
		if (auto plant = df.plant(p.id.plant_id))
			return df::fromCP437(plant->name_plural);
		return "Unknown tree";
	case df::unit_preference_type::LikeColor:
		return "Unknown color";
	case df::unit_preference_type::LikeShape:
		return "Unknown shape";
	case df::unit_preference_type::LikePoeticForm:
		return "Unknown poetic form";
	case df::unit_preference_type::LikeMusicalForm:
		return "Unknown musical form";
	case df::unit_preference_type::LikeDanceForm:
		return "Unknown dance form";
	default:
		return "Unknown preference";
	}
}

static inline QString tr(const char *source_text, const char *disambiguation = nullptr, int n = -1)
{
	return QCoreApplication::translate("Preference", source_text, disambiguation, n);
}

QString Preference::toString(df::unit_preference_type_t type)
{
	switch (type) {
	case df::unit_preference_type::LikeMaterial:
		return tr("Material");
	case df::unit_preference_type::LikeCreature:
		return tr("Creature");
	case df::unit_preference_type::LikeFood:
		return tr("Food");
	case df::unit_preference_type::HateCreature:
		return tr("Hate");
	case df::unit_preference_type::LikeItem:
		return tr("Item");
	case df::unit_preference_type::LikePlant:
		return tr("Plant");
	case df::unit_preference_type::LikeTree:
		return tr("Tree");
	case df::unit_preference_type::LikeColor:
		return tr("Color");
	case df::unit_preference_type::LikeShape:
		return tr("Shape");
	case df::unit_preference_type::LikePoeticForm:
		return tr("Poetry");
	case df::unit_preference_type::LikeMusicalForm:
		return tr("Music");
	case df::unit_preference_type::LikeDanceForm:
		return tr("Dance");
	default: return QString::fromLocal8Bit(to_string(type));
	}
}
