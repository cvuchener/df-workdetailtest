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

#include "UnitDescriptors.h"

QString UnitDescriptors::attributeName(df::physical_attribute_type_t attr)
{
	using namespace df::physical_attribute_type;
	switch (attr) {
	case STRENGTH:
		return tr("Strength");
	case AGILITY:
		return tr("Agility");
	case TOUGHNESS:
		return tr("Toughness");
	case ENDURANCE:
		return tr("Endurance");
	case RECUPERATION:
		return tr("Recuperation");
	case DISEASE_RESISTANCE:
		return tr("Disease resistance");
	default:
		return tr("Invalid physical attribute");
	}
}

QString UnitDescriptors::attributeName(df::mental_attribute_type_t attr)
{
	using namespace df::mental_attribute_type;
	switch (attr) {
	case ANALYTICAL_ABILITY:
		return tr("Analytical ability");
	case FOCUS:
		return tr("Focus");
	case WILLPOWER:
		return tr("Willpower");
	case CREATIVITY:
		return tr("Creativity");
	case INTUITION:
		return tr("Intuition");
	case PATIENCE:
		return tr("Patience");
	case MEMORY:
		return tr("Memory");
	case LINGUISTIC_ABILITY:
		return tr("Linguistic ability");
	case SPATIAL_SENSE:
		return tr("Spatial sense");
	case MUSICALITY:
		return tr("Musicality");
	case KINESTHETIC_SENSE:
		return tr("Kinesthetic sense");
	case EMPATHY:
		return tr("Empathy");
	case SOCIAL_AWARENESS:
		return tr("Social awareness");
	default:
		return tr("Invalid mental attribute");
	}
}

enum AttributeLevel {
	Average = -1,
	Lowest = 0,
	VeryVeryLow,
	VeryLow,
	Low,
	High,
	VeryHigh,
	VeryVeryHigh,
	Highest,
	AttributeLevelCount,
};

static AttributeLevel get_attribute_level(int caste_rating)
{
	if (caste_rating <= -100)
		return Lowest;
	else if (caste_rating <= -75)
		return VeryVeryLow;
	else if (caste_rating <= -50)
		return VeryLow;
	else if (caste_rating <= -25)
		return Low;
	else if (caste_rating >= 100)
		return Highest;
	else if (caste_rating >= 75)
		return VeryVeryHigh;
	else if (caste_rating >= 50)
		return VeryHigh;
	else if (caste_rating >= 25)
		return High;
	else
		return Average;
}

QString UnitDescriptors::attributeDescription(df::physical_attribute_type_t attr, int caste_rating)
{
	static constexpr std::array<std::array<const char *, AttributeLevelCount>, df::physical_attribute_type::Count> strings = {
		std::array{ // STRENGTH
			QT_TR_NOOP("unfathomably weak"),
			QT_TR_NOOP("unquestionably weak"),
			QT_TR_NOOP("very weak"),
			QT_TR_NOOP("weak"),
			QT_TR_NOOP("strong"),
			QT_TR_NOOP("very strong"),
			QT_TR_NOOP("mighty"),
			QT_TR_NOOP("unbelievably strong"),
		},
		std::array{ // AGILITY
			QT_TR_NOOP("abysmally clumsy"),
			QT_TR_NOOP("totally clumsy"),
			QT_TR_NOOP("quite clumsy"),
			QT_TR_NOOP("clumsy"),
			QT_TR_NOOP("agile"),
			QT_TR_NOOP("very agile"),
			QT_TR_NOOP("extremely agile"),
			QT_TR_NOOP("amazingly agile"),
		},
		std::array{ // TOUGHNESS
			QT_TR_NOOP("shockingly fragile"),
			QT_TR_NOOP("remarkably flimsy"),
			QT_TR_NOOP("very flimsy"),
			QT_TR_NOOP("flimsy"),
			QT_TR_NOOP("tough"),
			QT_TR_NOOP("quite durable"),
			QT_TR_NOOP("incredibly tough"),
			QT_TR_NOOP("basically unbreakable"),
		},
		std::array{ // ENDURANCE
			QT_TR_NOOP("truly quick to tire"),
			QT_TR_NOOP("extremely quick to tire"),
			QT_TR_NOOP("very quick to tire"),
			QT_TR_NOOP("quick to tire"),
			QT_TR_NOOP("slow to tire"),
			QT_TR_NOOP("very slow to tire"),
			QT_TR_NOOP("indefatigable"),
			QT_TR_NOOP("absolutely inexhaustible"),
		},
		std::array{ // RECUPERATION
			QT_TR_NOOP("shockingly slow to heal"),
			QT_TR_NOOP("really slow to heal"),
			QT_TR_NOOP("very slow to heal"),
			QT_TR_NOOP("slow to heal"),
			QT_TR_NOOP("quick to heal"),
			QT_TR_NOOP("quite quick to heal"),
			QT_TR_NOOP("incredibly quick to heal"),
			QT_TR_NOOP("possessed of amazing recuperative power"),
		},
		std::array{ // DISEASE_RESISTANCE
			QT_TR_NOOP("stunningly susceptible to disease"),
			QT_TR_NOOP("really susceptible to disease"),
			QT_TR_NOOP("quite susceptible to disease"),
			QT_TR_NOOP("susceptible to disease"),
			QT_TR_NOOP("rarely sick"),
			QT_TR_NOOP("very rarely sick"),
			QT_TR_NOOP("almost never sick"),
			QT_TR_NOOP("virtually never sick"),
		},
	};
	auto level = get_attribute_level(caste_rating);
	if (level != Average)
		return tr(strings[attr][level]);
	else
		return {};
}

QString UnitDescriptors::attributeDescription(df::mental_attribute_type_t attr, int caste_rating)
{
	static constexpr std::array<std::array<const char *, AttributeLevelCount>, df::mental_attribute_type::Count> strings = {
		std::array{ // ANALYTICAL_ABILITY
			QT_TR_NOOP("a stunning lack of analytical ability"),
			QT_TR_NOOP("a lousy intellect"),
			QT_TR_NOOP("very bad analytical abilities"),
			QT_TR_NOOP("poor analytical abilities"),
			QT_TR_NOOP("a good intellect"),
			QT_TR_NOOP("a sharp intellect"),
			QT_TR_NOOP("great analytical abilities"),
			QT_TR_NOOP("awesome intellectual powers"),
		},
		std::array{ // FOCUS
			QT_TR_NOOP("the absolute inability to focus"),
			QT_TR_NOOP("really poor focus"),
			QT_TR_NOOP("quite poor focus"),
			QT_TR_NOOP("poor focus"),
			QT_TR_NOOP("the ability to focus"),
			QT_TR_NOOP("very good focus"),
			QT_TR_NOOP("a great ability to focus"),
			QT_TR_NOOP("unbreakable focus"),
		},
		std::array{ // WILLPOWER
			QT_TR_NOOP("absolutely no willpower"),
			QT_TR_NOOP("next to no willpower"),
			QT_TR_NOOP("a large deficit of willpower"),
			QT_TR_NOOP("little willpower"),
			QT_TR_NOOP("willpower"),
			QT_TR_NOOP("a lot of willpower"),
			QT_TR_NOOP("an iron will"),
			QT_TR_NOOP("an unbreakable will"),
		},
		std::array{ // CREATIVITY
			QT_TR_NOOP("next to no creative talent"),
			QT_TR_NOOP("lousy creativity"),
			QT_TR_NOOP("poor creativity"),
			QT_TR_NOOP("meager creativity"),
			QT_TR_NOOP("good creativity"),
			QT_TR_NOOP("very good creativity"),
			QT_TR_NOOP("great creativity"),
			QT_TR_NOOP("a boundless creative imagination"),
		},
		std::array{ // INTUITION
			QT_TR_NOOP("horrible intuition"),
			QT_TR_NOOP("lousy intuition"),
			QT_TR_NOOP("very bad intuition"),
			QT_TR_NOOP("bad intuition"),
			QT_TR_NOOP("good intuition"),
			QT_TR_NOOP("very good intuition"),
			QT_TR_NOOP("great intuition"),
			QT_TR_NOOP("uncanny intuition"),
		},
		std::array{ // PATIENCE
			QT_TR_NOOP("no patience at all"),
			QT_TR_NOOP("very little patience"),
			QT_TR_NOOP("little patience"),
			QT_TR_NOOP("a shortage of patience"),
			QT_TR_NOOP("a sum of patience"),
			QT_TR_NOOP("a great deal of patience"),
			QT_TR_NOOP("a deep well of patience"),
			QT_TR_NOOP("absolutely boundless patience"),
		},
		std::array{ // MEMORY
			QT_TR_NOOP("little memory to speak of"),
			QT_TR_NOOP("a really bad memory"),
			QT_TR_NOOP("a poor memory"),
			QT_TR_NOOP("an iffy memory"),
			QT_TR_NOOP("a good memory"),
			QT_TR_NOOP("a great memory"),
			QT_TR_NOOP("an amazing memory"),
			QT_TR_NOOP("an astonishing memory"),
		},
		std::array{ // LINGUISTIC_ABILITY
			QT_TR_NOOP("difficulty with words and language"),
			QT_TR_NOOP("very little linguistic ability"),
			QT_TR_NOOP("little linguistic ability"),
			QT_TR_NOOP("a little difficulty with words"),
			QT_TR_NOOP("a way with words"),
			QT_TR_NOOP("a natural inclination toward language"),
			QT_TR_NOOP("a great affinity for language"),
			QT_TR_NOOP("an astonishing ability with languages and words"),
		},
		std::array{ // SPATIAL_SENSE
			QT_TR_NOOP("no sense for spatial relationships"),
			QT_TR_NOOP("an atrocious spatial sense"),
			QT_TR_NOOP("poor spatial senses"),
			QT_TR_NOOP("a questionable spatial sense"),
			QT_TR_NOOP("a good spatial sense"),
			QT_TR_NOOP("a great feel for the surrounding space"),
			QT_TR_NOOP("an amazing spatial sense"),
			QT_TR_NOOP("a stunning feel for spatial relationships"),
		},
		std::array{ // MUSICALITY
			QT_TR_NOOP("absolutely no feel for music at all"),
			QT_TR_NOOP("next to no natural musical ability"),
			QT_TR_NOOP("little natural inclination toward music"),
			QT_TR_NOOP("an iffy sense for music"),
			QT_TR_NOOP("a feel for music"),
			QT_TR_NOOP("a natural ability with music"),
			QT_TR_NOOP("a great musical sense"),
			QT_TR_NOOP("an astonishing knack for music"),
		},
		std::array{ // KINESTHETIC_SENSE
			QT_TR_NOOP("an unbelievably atrocious sense of the position of their own body"),
			QT_TR_NOOP("a very clumsy kinesthetic sense"),
			QT_TR_NOOP("a poor kinesthetic sense"),
			QT_TR_NOOP("a meager kinesthetic sense"),
			QT_TR_NOOP("a good kinesthetic sense"),
			QT_TR_NOOP("a very good sense of the position of their own body"),
			QT_TR_NOOP("a great kinesthetic sense"),
			QT_TR_NOOP("an astounding feel for the position of their own body"),
		},
		std::array{ // EMPATHY
			QT_TR_NOOP("the utter inability to judge others' emotions"),
			QT_TR_NOOP("next to no empathy"),
			QT_TR_NOOP("a very bad sense of empathy"),
			QT_TR_NOOP("poor empathy"),
			QT_TR_NOOP("an ability to read emotions fairly well"),
			QT_TR_NOOP("a very good sense of empathy"),
			QT_TR_NOOP("a great sense of empathy"),
			QT_TR_NOOP("an absolutely remarkable sense of others' emotions"),
		},
		std::array{ // SOCIAL_AWARENESS
			QT_TR_NOOP("an absolute inability to understand social relationships"),
			QT_TR_NOOP("a lack of understanding of social relationships"),
			QT_TR_NOOP("a poor ability to manage or understand social relationships"),
			QT_TR_NOOP("a meager ability with social relationships"),
			QT_TR_NOOP("a good feel for social relationships"),
			QT_TR_NOOP("a very good feel for social relationships"),
			QT_TR_NOOP("a great feel for social relationships"),
			QT_TR_NOOP("a shockingly profound feel for social relationships"),
		},
	};
	auto level = get_attribute_level(caste_rating);
	if (level != Average)
		return tr(strings[attr][level]);
	else
		return {};
}
