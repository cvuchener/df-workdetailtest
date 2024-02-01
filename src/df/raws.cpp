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

#include "df/raws.h"
#include "df/utils.h"

std::string_view df::world_raws::language_t::english_word(const df::language_name &name, df::language_name_component_t comp) const
{
	Q_ASSERT(name.words[comp] >= 0 && unsigned(name.words[comp]) < words.size());
	Q_ASSERT(name.parts_of_speech[comp] >= 0 && name.parts_of_speech[comp] < 9);
	return words[name.words[comp]]->forms[name.parts_of_speech[comp]];
}

std::string_view df::world_raws::language_t::local_word(const df::language_name &name, df::language_name_component_t comp) const
{
	Q_ASSERT(name.language >= 0 && unsigned(name.language) < translations.size());
	Q_ASSERT(name.words[comp] >= 0 && unsigned(name.words[comp]) < translations[name.language]->words.size());
	return *translations[name.language]->words[name.words[comp]];
}

static QString capitalize(QString &&str)
{
	if (!str.isEmpty())
		str[0] = str[0].toUpper();
	return str;
}

QString df::world_raws::language_t::translate_name(const df::language_name &name, bool english) const
{
	using namespace df::language_name_component;
	QString str;
	if (!name.first_name.empty()) {
		str += capitalize(fromCP437(name.first_name));
		str += " ";
	}
	if (!name.nickname.empty()) {
		str += u"\u2018"; // left single quote: ‘
		str += fromCP437(name.nickname);
		str += u"\u2019 "; // right single quote: ’
	}
	if (english) {
		QString last_name;
		if (name.words[FrontCompound] != -1)
			last_name += fromCP437(english_word(name, FrontCompound));
		if (name.words[RearCompound] != -1)
			last_name += fromCP437(english_word(name, RearCompound));
		str += capitalize(std::move(last_name));
		bool word_added = false;
		for (int i = 2; i < 6; ++i) {
			if (name.words[i] != -1) {
				if (!word_added) {
					if (str.isEmpty())
						str += "The ";
					else
						str += " the ";
					word_added = true;
				}
				else if (i == TheX && name.words[HyphenCompound] != -1)
					str += "-";
				else
					str += " ";
				auto word = english_word(name, df::language_name_component_t(i));
				str += capitalize(fromCP437(word));
			}
		}
		if (name.words[OfX] != -1) {
			if (str.isEmpty())
				str += "Of ";
			else
				str += " of ";
			str += capitalize(fromCP437(english_word(name, OfX)));
		}
	}
	else {
		QString last_name;
		if (name.words[FrontCompound] != -1)
			last_name += fromCP437(local_word(name, FrontCompound));
		if (name.words[RearCompound] != -1)
			last_name += fromCP437(local_word(name, RearCompound));
		str += capitalize(std::move(last_name));
		bool need_space = !str.isEmpty();
		for (int i = 2; i < 6; ++i) {
			if (name.words[i] != -1) {
				auto word = local_word(name, df::language_name_component_t(i));
				if (need_space) {
					str += " ";
					need_space = false;
					str += capitalize(fromCP437(word));
				}
				else
					str += fromCP437(word);
			}
		}
		if (name.words[OfX] != -1) {
			str += " ";
			str += capitalize(fromCP437(local_word(name, OfX)));
		}
	}
	return str;
}

