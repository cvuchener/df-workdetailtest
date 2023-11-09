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

#include "DFRaws.h"

#include "CP437.h"

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

QString df::world_raws::language_t::translate_name(const df::language_name &name, bool english) const
{
	using namespace df::language_name_component;
	QString str;
	if (!name.first_name.empty()) {
		str += fromCP437(name.first_name);
		str += " ";
	}
	if (!name.nickname.empty()) {
		str += u"\u2018"; // left single quote: ‘
		str += fromCP437(name.nickname);
		str += u"\u2019 "; // right single quote: ’
	}
	if (english) {
		if (name.words[FrontCompound] != -1)
			str += fromCP437(english_word(name, FrontCompound));
		if (name.words[RearCompound] != -1)
			str += fromCP437(english_word(name, RearCompound));
		bool word_added = false;
		for (int i = 2; i < 6; ++i) {
			if (name.words[i] != -1) {
				if (!word_added) {
					str += " the ";
					word_added = true;
				}
				else if (i == 5 && name.words[4] != -1)
					str += "-";
				else
					str += " ";
				str += fromCP437(english_word(name, df::language_name_component_t(i)));
			}
		}
		if (name.words[OfX] != -1) {
			str += " of ";
			str += fromCP437(english_word(name, OfX));
		}
	}
	else {
		if (name.words[FrontCompound] != -1)
			str += fromCP437(local_word(name, FrontCompound));
		if (name.words[RearCompound] != -1)
			str += fromCP437(local_word(name, RearCompound));
		bool word_added = false;
		for (int i = 2; i < 6; ++i) {
			if (name.words[i] != -1) {
				if (!word_added) {
					str += " ";
					word_added = true;
				}
				str += fromCP437(local_word(name, df::language_name_component_t(i)));
			}
		}
		if (name.words[OfX] != -1) {
			str += " ";
			str += fromCP437(local_word(name, OfX));
		}
	}
	return str;
}

