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

#include "Item.h"

#include "Material.h"
#include "df/raws.h"
#include "df/items.h"
#include "df/utils.h"

#include <QStringList>

template <std::derived_from<df::itemdef> ItemDef>
static QString itemdef_string(
		const ItemDef *itemdef,
		Material material,
		bool plural)
{
	QStringList words;
	if (itemdef) {
		if constexpr (df::ItemDefHasPrePlural<ItemDef>) {
			if (plural && !itemdef->name_preplural.empty())
				words << df::fromCP437(itemdef->name_preplural);
		}
		if constexpr (df::ItemDefHasAdjective<ItemDef>) {
			if (!itemdef->adjective.empty())
				words << df::fromCP437(itemdef->adjective);
		}
	}
	if (material)
		words << material.adjective();
	else if (itemdef)
		if constexpr (df::ItemDefHasMatPlaceholder<ItemDef>) {
			if (!itemdef->material_placeholder.empty())
				words << df::fromCP437(itemdef->material_placeholder);
		}
	// gloves flags item->handedness
	if (itemdef) {
		if constexpr (df::ItemDefHasArmorFlags<ItemDef>) {
			auto m = material.get();
			if (m && m->flags.isSet(df::material_flags::IS_METAL)
					&& itemdef->armor_flags.isSet(df::armor_general_flags::CHAIN_METAL_TEXT))
				words << "chain";
		}
	}
	if (itemdef) {
		if constexpr (df::ItemDefHasPlural<ItemDef>) {
			if (plural)
				words << df::fromCP437(itemdef->name_plural);
			else
				words << df::fromCP437(itemdef->name);
		}
		else
			words << df::fromCP437(itemdef->name);
	}
	else switch (df::itemdef_item_t<ItemDef>::item_type) {
	case df::item_type::INSTRUMENT:
		words << (plural ? "musical instruments" : "musical instrument");
	case df::item_type::TOY:
		words << (plural ? "toys" : "toy");
	case df::item_type::WEAPON:
		words << (plural ? "weapons" : "weapon");
	case df::item_type::ARMOR:
		words << (plural ? "suits of armor" : "armor");
	case df::item_type::SHOES:
		words << "footwear";
	case df::item_type::SHIELD:
		words << (plural ? "shields/bucklers" : "shield/buckler");
	case df::item_type::HELM:
		words << "headwear";
	case df::item_type::GLOVES:
		words << "handwear";
	case df::item_type::AMMO:
		words << "ammunition";
	case df::item_type::PANTS:
		words << "legwear";
	case df::item_type::SIEGEAMMO:
		words << "siege ammo";
	case df::item_type::TRAPCOMP:
		words << (plural ? "trap components" : "trap component");
	case df::item_type::FOOD:
		words << (plural ? "prepared meals" : "prepared meal");
	case df::item_type::TOOL:
		words << (plural ? "tools" : "tool");
	default:
		words << "unknown item";
	}
	return words.join(" ");
}

static QChar quality_symbol(df::item_quality_t quality)
{
	switch (quality) {
	case df::item_quality::WellCrafted:
		return '-';
	case df::item_quality::FinelyCrafted:
		return '+';
	case df::item_quality::Superior:
		return '*';
	case df::item_quality::Exceptional:
		return u'\u2261'; // ≡
	case df::item_quality::Masterful:
		return u'\u263c'; // ☼
	default:
		return {};
	}
}

QString Item::toString(const DwarfFortressData &df, const df::item &item)
{
	return df::visit_item([&df]<std::derived_from<df::item> T>(const T &item) -> QString {
		// TODO: TOOL: unglazed, shape
		// TODO: LIQUID_MISC, GLOB: contaminants, pressed/paste
		// TODO: ORTHOPEDIC_CAST: material, body part
		// TODO: SHEET: pressed/paste
		// TODO: GLOVES: right/left flag
		// TODO: GEM, SMALLGEM: shape
		// TODO: artifacts
		QString prefix, suffix;
		if (item.flags.bits.on_fire) {
			prefix.append(u'\u203c'); // ‼
			suffix.prepend(u'\u203c'); // ‼
		}
		if constexpr (std::derived_from<T, df::item_actual>)
			switch (item.wear) {
			case 0:
				break;
			case 1:
				prefix.append('x');
				suffix.prepend('x');
				break;
			case 2:
				prefix.append('X');
				suffix.prepend('X');
				break;
			default:
				prefix.append("XX");
				suffix.prepend("XX");
			}
		if (item.flags.bits.foreign) {
			prefix.append('(');
			suffix.prepend(')');
		}
		if (item.flags.bits.forbid) {
			prefix.append('{');
			suffix.prepend('}');
		}
		if constexpr (std::derived_from<T, df::item_constructed>)
			if (std::ranges::any_of(item.improvements, &df::itemimprovement::isDecoration)) {
				auto best_improv = std::ranges::max_element(
						item.improvements, {},
						&df::itemimprovement::quality);
				auto q = quality_symbol((*best_improv)->quality);
				prefix.append(q);
				prefix.append(u'\u00ab'); // «
				suffix.prepend(q);
				suffix.prepend(u'\u00bb'); // »

			}
		// TODO: magic ◄ \u25c4 ► \u25ba
		if constexpr (std::derived_from<T, df::item_crafted>) {
			auto q = quality_symbol(item.quality);
			if (!q.isNull()) {
				prefix.append(q);
				suffix.prepend(q);
			}
		}

		int subtype = -1;
		if constexpr (df::ItemHasItemDef<T>)
			subtype = item.subtype->subtype;
		int mat_type = -1, mat_index = -1;
		if constexpr (df::ItemHasMaterial<T>) {
			mat_type = item.mat_type;
			mat_index = item.mat_index;
		}
		int stack_size = -1;
		if constexpr (df::ItemHasStackSize<T>)
			stack_size = item.stack_size;
		auto item_type = df::item_type::NONE;
		if constexpr (!std::same_as<T, df::item>)
			item_type = T::item_type;

		QStringList words;
		// TODO: large/small from item_crafted.maker_race?
		if (item.flags2.bits.grown)
			words << "grown";
		words << toString(df, item_type, subtype, mat_type, mat_index, stack_size != 1);
		if (stack_size > 1)
			words << QString("[%2]").arg(stack_size);
		return prefix + words.join(" ") + suffix;
	}, item);
}

static QString gem_name(Material material, bool plural)
{
	if (auto m = material.get()) {
		if (plural) {
			if (m->gem_name2 == "INP")
				return df::fromCP437(m->gem_name1);
			else if (m->gem_name2 == "STP")
				return df::fromCP437(m->gem_name1)+"s";
			else if (!m->gem_name2.empty())
				return df::fromCP437(m->gem_name2);
		}
		if (!m->gem_name1.empty())
			return df::fromCP437(m->gem_name1);
		else
			return material.name();
	}
	else
		return plural ? "gems" : "gem";
}

static QChar sex_symbol(df::pronoun_type_t sex)
{
	switch (sex) {
	case df::pronoun_type::he:
		return u'\u2642';
	case df::pronoun_type::she:
		return u'\u2640';
	default:
		return {};
	}
}


QString Item::toString(const DwarfFortressData &df, df::item_type_t type, int subtype, int mattype, int matindex, bool plural)
{
	Material material{df, mattype, matindex};
#define ITEM_NAME_MAT(name_singular, name_plural) \
	(QString(plural \
		? "%1 " name_plural \
		: "%1 " name_singular) \
		.arg(material.adjective()))
#define ITEM_NAME_MAT_NAME(name_singular, name_plural) \
	(QString(plural \
		? "%1 " name_plural \
		: "%1 " name_singular) \
		.arg(material.name()))
#define ITEM_NAME(name_singular, name_plural) \
	(material \
		? ITEM_NAME_MAT(name_singular, name_plural) \
		: (plural ? name_plural : name_singular))
#define ITEM_NAME_INV(name) \
	(material \
		? QString("%1 " name).arg(material.adjective()) \
		: name)
#define ITEMDEF_NAME(type) \
	itemdef_string( \
		df.raws ? df::get(df.raws->itemdefs.type, subtype) : nullptr, \
		material, \
		plural)
	switch (type) {
	case df::item_type::BAR:
		switch (material.category()) {
		case Material::Category::Inorganic:
			if (auto inorganic = material.inorganic()) {
				return QString(inorganic->flags.isSet(df::inorganic_flags::WAFERS)
					? "%1 wafers"
					: "%1 bars")
					.arg(material.adjective());
			}
			else
				return "metal bars";
		case Material::Category::Builtin:
			if (material.builtin() == df::builtin_mats::COAL) {
				switch (material.subindex()) {
				case 0:
					return "coke";
				case 1:
					return "charcoal";
				default:
					return "refined coal";
				}
			}
			[[fallthrough]];
		default:
			if (material)
				return material.name();
			return "bars";
		}
	case df::item_type::SMALLGEM:
		switch (material.category()) {
		case Material::Category::Builtin:
			switch (*material.builtin()) {
			case df::builtin_mats::GLASS_GREEN:
				return "cut green glass gems";
			case df::builtin_mats::GLASS_CLEAR:
				return "cut clear glass gems";
			case df::builtin_mats::GLASS_CRYSTAL:
				return "cut crystal glass gems";
			default:
				break;
			}
			[[fallthrough]];
		default:
			return gem_name(material, true);
		}
	case df::item_type::BLOCKS:
		if (auto m = material.get()) {
			return QString("%1 %2")
				.arg(material.adjective())
				.arg(m->block_name[1].empty()
					? "blocks"
					: df::fromCP437(m->block_name[1]));
		}
		else if (material)
			return QString("%1 blocks").arg(material.adjective());
		return "blocks";
	case df::item_type::ROUGH:
		switch (material.category()) {
		case Material::Category::Builtin:
			switch (*material.builtin()) {
			case df::builtin_mats::GLASS_GREEN:
				return "raw green glass gems";
			case df::builtin_mats::GLASS_CLEAR:
				return "raw clear glass gems";
			case df::builtin_mats::GLASS_CRYSTAL:
				return "raw crystal glass gems";
			default:
				break;
			}
			[[fallthrough]];
		default:
			return QString("rough %1").arg(gem_name(material, plural));
		}
	case df::item_type::BOULDER:
		switch (material.category()) {
		case Material::Category::Invalid:
			return "stones";
		case Material::Category::Inorganic:
			if (auto m = material.get()) {
				if (!m->stone_name.empty())
					return df::fromCP437(m->stone_name);
			}
			[[fallthrough]];
		default:
			return material.name();
		}
	case df::item_type::WOOD:
		return ITEM_NAME_INV("logs");
	case df::item_type::DOOR:
		if (auto m = material.get())
			if (m->flags.isSet(df::material_flags::IS_GLASS))
				return ITEM_NAME_MAT("portal", "portals");
		return ITEM_NAME("door", "doors");
	case df::item_type::FLOODGATE:
		return ITEM_NAME("floodgate", "floodgates");
	case df::item_type::BED:
		return ITEM_NAME("bed", "beds");
	case df::item_type::CHAIR:
		switch (material.category()) {
		case Material::Category::Plant:
			return ITEM_NAME_MAT("chair", "chairs");
		default:
			return ITEM_NAME("throne", "thrones");
		}
	case df::item_type::CHAIN:
		if (auto m = material.get())
			if (m->flags.isSet(df::material_flags::THREAD_PLANT)
					|| m->flags.isSet(df::material_flags::LEATHER)
					|| m->flags.isSet(df::material_flags::SILK)
					|| m->flags.isSet(df::material_flags::YARN))
				return ITEM_NAME_MAT("rope", "ropes");
		return ITEM_NAME("chain", "chains");
	case df::item_type::FLASK:
		if (auto m = material.get()) {
			if (m->flags.isSet(df::material_flags::THREAD_PLANT)
					|| m->flags.isSet(df::material_flags::LEATHER)
					|| m->flags.isSet(df::material_flags::SILK)
					|| m->flags.isSet(df::material_flags::YARN))
				return ITEM_NAME_MAT("waterskin", "waterskins");
			if (m->flags.isSet(df::material_flags::IS_GLASS))
				return ITEM_NAME_MAT("vial", "vials");
		}
		return ITEM_NAME("flask", "flasks");
	case df::item_type::GOBLET:
		switch (material.category()) {
		case Material::Category::Plant:
			return ITEM_NAME_MAT("cup", "cups");
		case Material::Category::Inorganic:
			if (auto m = material.get())
				if (m->flags.isSet(df::material_flags::IS_METAL))
					return ITEM_NAME_MAT("mug", "mugs");
			break;
		default:
			break;
		}
		return ITEM_NAME("goblet", "goblets");
	case df::item_type::INSTRUMENT:
		return ITEMDEF_NAME(instruments);
	case df::item_type::TOY:
		return ITEMDEF_NAME(toys);
	case df::item_type::WINDOW:
		return ITEM_NAME("window", "windows");
	case df::item_type::CAGE:
		if (auto m = material.get())
			if (m->flags.isSet(df::material_flags::IS_GLASS))
				return ITEM_NAME_MAT("terrarium", "terrariums");
		return ITEM_NAME("cage", "cages");
	case df::item_type::BARREL:
		return ITEM_NAME("barrel", "barrels");
	case df::item_type::BUCKET:
		return ITEM_NAME("bucket", "buckets");
	case df::item_type::ANIMALTRAP:
		return ITEM_NAME("animal trap", "animal traps");
	case df::item_type::TABLE:
		return ITEM_NAME("table", "tables");
	case df::item_type::COFFIN:
		switch (material.category()) {
		case Material::Category::Plant:
			return ITEM_NAME_MAT("casket", "caskets");
		default:
			if (auto m = material.get())
				if (m->flags.isSet(df::material_flags::IS_METAL))
					return ITEM_NAME_MAT("sarcophagus", "sarcophagi");
			return ITEM_NAME("coffin", "coffins");
		}
	case df::item_type::STATUE:
		return ITEM_NAME("statue", "statues");
	case df::item_type::CORPSE:
		return plural ? "corpses" : "corpse";
	case df::item_type::WEAPON:
		return ITEMDEF_NAME(weapons);
	case df::item_type::ARMOR:
		return ITEMDEF_NAME(armor);
	case df::item_type::SHOES:
		return ITEMDEF_NAME(shoes);
	case df::item_type::SHIELD:
		return ITEMDEF_NAME(shields);
	case df::item_type::HELM:
		return ITEMDEF_NAME(helms);
	case df::item_type::GLOVES:
		return ITEMDEF_NAME(gloves);
	case df::item_type::BOX:
		if (auto m = material.get()) {
			if (material.category() == Material::Category::Inorganic
					&& !m->flags.isSet(df::material_flags::IS_METAL))
				return ITEM_NAME_MAT("coffer", "coffers");
			else
				return ITEM_NAME_MAT("chest", "chests");
		}
		return ITEM_NAME("box", "boxes");
	case df::item_type::BAG:
		return ITEM_NAME("bag", "bags");
	case df::item_type::BIN:
		return ITEM_NAME("bin", "bins");
	case df::item_type::ARMORSTAND:
		return ITEM_NAME("armor stand", "armor stands");
	case df::item_type::WEAPONRACK:
		return ITEM_NAME("weapon rack", "weapon racks");
	case df::item_type::CABINET:
		return ITEM_NAME("cabinet", "cabinets");
	case df::item_type::FIGURINE:
		return ITEM_NAME("figurine", "figurines");
	case df::item_type::AMULET:
		return ITEM_NAME("amulet", "amulets");
	case df::item_type::SCEPTER:
		return ITEM_NAME("scepter", "scepters");
	case df::item_type::AMMO:
		return ITEMDEF_NAME(ammo);
	case df::item_type::CROWN:
		return ITEM_NAME("crown", "crowns");
	case df::item_type::RING:
		return ITEM_NAME("ring", "rings");
	case df::item_type::EARRING:
		return ITEM_NAME("earring", "earrings");
	case df::item_type::BRACELET:
		return ITEM_NAME("bracelet", "bracelets");
	case df::item_type::GEM:
		switch (material.category()) {
		case Material::Category::Invalid:
			return plural ? "large gems" : "large gem";
		case Material::Category::Inorganic:
			return QString("large %1").arg(gem_name(material, plural));
		case Material::Category::Builtin:
			switch (*material.builtin()) {
			case df::builtin_mats::GLASS_GREEN:
				return plural
					? "large green glass gems"
					: "large green glass gem";
			case df::builtin_mats::GLASS_CLEAR:
				return plural
					? "large clear glass gems"
					: "large clear glass gem";
			case df::builtin_mats::GLASS_CRYSTAL:
				return plural
					? "large crystal glass gems"
					: "large crystal glass gem";
			default:
				break;
			}
			[[fallthrough]];
		default:
			return QString(plural
				? "large %1 gems"
				: "large %1 gem")
				.arg(material.adjective());
		}
	case df::item_type::ANVIL:
		return ITEM_NAME("anvil", "anvils");
	case df::item_type::CORPSEPIECE:
		return plural ? "body parts" : "body part";
	case df::item_type::REMAINS:
		if (auto c = df.caste(mattype, matindex))
			return QString("%1 %2")
				.arg(df.creatureName(mattype, false))
				.arg(df::fromCP437(plural
					? c->remains[1]
					: c->remains[0]));
		else
			return "remains";

	case df::item_type::MEAT:
		if (auto m = material.get()) {
			QString prefix;
			if (!m->meat_name[2].empty()) {
				prefix += df::fromCP437(m->meat_name[2]);
				prefix += " ";
			}
			if (auto c = material.creature()) {
				prefix += df::fromCP437(c->name[0]);
				prefix += " ";
			}
			if (!m->meat_name[0].empty())
				return prefix + df::fromCP437(plural ? m->meat_name[1] : m->meat_name[0]);
			else
				return prefix + QString(plural ? "%1 chops" : "%1 chop")
					.arg(df::fromCP437(m->state_name[df::matter_state::Solid]));
		}
		return "meat";
	case df::item_type::FISH:
		if (mattype >= 0) {
			auto name = df.creatureName(mattype, false, matindex);
			QChar s;
			if (auto c = df.caste(mattype, matindex))
				s = sex_symbol(c->sex);
			return s.isNull()
				? name
				: QString("%1, %2").arg(name).arg(s);
		}
		return "fish";
	case df::item_type::FISH_RAW:
		if (mattype >= 0) {
			auto name = df.creatureName(mattype, false, matindex);
			QChar s;
			if (auto c = df.caste(mattype, matindex))
				s = sex_symbol(c->sex);
			return s.isNull()
				? QString("raw %1").arg(name)
				: QString("raw %1, %2").arg(name).arg(s);
		}
		return "raw fish";
	case df::item_type::VERMIN:
		if (mattype >= 0)
			return QString("live %1").arg(df.creatureName(mattype, plural, matindex));
		return plural ? "small live animals" : "small live animal";
	case df::item_type::PET:
		if (mattype >= 0)
			return QString("tame %1").arg(df.creatureName(mattype, plural, matindex));
		return plural ? "small tame animals" : "small tame animal";
	case df::item_type::SEEDS:
		if (auto p = material.plant())
			return df::fromCP437(p->seed_plural);
		return ITEM_NAME_INV("seeds");
	case df::item_type::PLANT:
		if (auto p = df.plant(matindex))
			return plural
				? df::fromCP437(p->name_plural)
				: df::fromCP437(p->name);
		else
			return "plants";
	case df::item_type::SKIN_TANNED:
		if (material)
			return material.adjective();
		return plural ? "tanned hides" : "tanned hide";
	case df::item_type::PLANT_GROWTH:
		if (auto p = material.plant()) {
			if (auto g = df::get(p->growths, subtype))
				return df::fromCP437(plural ? g->name_plural : g->name);
		}
		return ITEM_NAME("leaf or fruit", "leaves and fruit");
	case df::item_type::THREAD:
		if (auto m = material.get()) {
			if (m->flags.isSet(df::material_flags::SILK))
				return ITEM_NAME_MAT("web", "webs");
			if (m->flags.isSet(df::material_flags::IS_METAL))
				return QString("%1 strands")
					.arg(material.adjective());
			if (m->flags.isSet(df::material_flags::YARN))
				return QString("%1 yarn")
					.arg(material.adjective());
		}
		return ITEM_NAME_INV("thread");
	case df::item_type::CLOTH:
		return ITEM_NAME_INV("cloth");
	case df::item_type::TOTEM:
		return plural ? "totems" : "totem";
	case df::item_type::PANTS:
		return ITEMDEF_NAME(pants);
	case df::item_type::BACKPACK:
		return ITEM_NAME("backpack", "backpacks");
	case df::item_type::QUIVER:
		return ITEM_NAME("quiver", "quivers");
	case df::item_type::CATAPULTPARTS:
		return ITEM_NAME_INV("catapult parts");
	case df::item_type::BALLISTAPARTS:
		return ITEM_NAME_INV("ballista parts");
	case df::item_type::SIEGEAMMO:
		return ITEMDEF_NAME(siege_ammo);
	case df::item_type::BALLISTAARROWHEAD:
		return ITEM_NAME("ballista arrow head", "ballista array heads");
	case df::item_type::TRAPPARTS:
		return ITEM_NAME_INV("mechanisms");
	case df::item_type::TRAPCOMP:
		return ITEMDEF_NAME(trapcomps);
	case df::item_type::DRINK:
		if (material)
			return material.name(df::matter_state::Liquid);
		return plural ? "drinks" : "drink";
	case df::item_type::POWDER_MISC:
		if (material)
			return material.name(df::matter_state::Powder);
		return "powder";
	case df::item_type::CHEESE:
		if (material)
			return material.name();
		return "cheese";
	case df::item_type::FOOD:
		return ITEMDEF_NAME(food);
	case df::item_type::LIQUID_MISC:
		if (material)
			return material.name(df::matter_state::Liquid);
		return "liquid";
	case df::item_type::COIN:
		return ITEM_NAME("coin", "coins");
	case df::item_type::GLOB:
		return "glob";
	case df::item_type::ROCK:
		if (material)
			return QString("small %1 rock").arg(material.name());
		return "small rock";
	case df::item_type::PIPE_SECTION:
		if (auto m = material.get())
			if (m->flags.isSet(df::material_flags::IS_GLASS))
				return ITEM_NAME_MAT("tube", "tubes");
		return ITEM_NAME("pipe section", "pipe sections");
	case df::item_type::HATCH_COVER:
		return ITEM_NAME("hatch cover", "hatch covers");
	case df::item_type::GRATE:
		return ITEM_NAME("grate", "grates");
	case df::item_type::QUERN:
		return ITEM_NAME("quern", "querns");
	case df::item_type::MILLSTONE:
		return ITEM_NAME("millstone", "millstones");
	case df::item_type::SPLINT:
		return ITEM_NAME("splint", "splints");
	case df::item_type::CRUTCH:
		return ITEM_NAME("crutch", "crutches");
	case df::item_type::TRACTION_BENCH:
		return ITEM_NAME("traction bench", "traction benches");
	case df::item_type::ORTHOPEDIC_CAST:
		return ITEM_NAME("limb/body cast", "limb/body casts");
	case df::item_type::TOOL:
		return ITEMDEF_NAME(tools);
	case df::item_type::SLAB:
		return ITEM_NAME("slab", "slabs");
	case df::item_type::EGG:
		if (mattype >= 0)
			return QString("%1 egg").arg(df.creatureName(mattype, false, matindex));
		return "egg";
	case df::item_type::BOOK:
		if (material)
			return QString(plural
				? "%1-bound codices"
				: "%1-bound codex")
				.arg(material.adjective());
		return plural ? "codices" : "codex";
	case df::item_type::SHEET:
		return ITEM_NAME_INV("sheet");
	case df::item_type::BRANCH:
		if (material)
			return ITEM_NAME_MAT_NAME("branch", "branches");
		return plural ? "branches" : "branch";
	default:
		return ITEM_NAME("item", "items");
	}
}
