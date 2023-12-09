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

#include "df/utils.h"

#include <map>

static const char16_t CP437Table[256] = {
	/*0x*/ u'\u0000'/*␀*/, u'\u263A'/*☺*/, u'\u263B'/*☻*/, u'\u2665'/*♥*/, u'\u2666'/*♦*/, u'\u2663'/*♣*/, u'\u2660'/*♠*/, u'\u2022'/*•*/,
	/*  */ u'\u25D8'/*◘*/, u'\u25CB'/*○*/, u'\u25D9'/*◙*/, u'\u2642'/*♂*/, u'\u2640'/*♀*/, u'\u266A'/*♪*/, u'\u266B'/*♫*/, u'\u263C'/*☼*/,
	/*1x*/ u'\u25BA'/*►*/, u'\u25C4'/*◄*/, u'\u2195'/*↕*/, u'\u203C'/*‼*/, u'\u00B6'/*¶*/, u'\u00A7'/*§*/, u'\u25AC'/*▬*/, u'\u21A8'/*↨*/,
	/*  */ u'\u2191'/*↑*/, u'\u2193'/*↓*/, u'\u2192'/*→*/, u'\u2190'/*←*/, u'\u221F'/*∟*/, u'\u2194'/*↔*/, u'\u25B2'/*▲*/, u'\u25BC'/*▼*/,
	/*2x*/ u' ', u'!', u'"', u'#', u'$', u'%', u'&', u'\'', u'(', u')', u'*', u'+', u',', u'-', u'.', u'/',
	/*3x*/ u'0', u'1', u'2', u'3', u'4', u'5', u'6', u'7', u'8', u'9', u':', u';', u'<', u'=', u'>', u'?',
	/*4x*/ u'@', u'A', u'B', u'C', u'D', u'E', u'F', u'G', u'H', u'I', u'J', u'K', u'L', u'M', u'N', u'O',
	/*5x*/ u'P', u'Q', u'R', u'S', u'T', u'U', u'V', u'W', u'X', u'Y', u'Z', u'[', u'\\', u']', u'^', u'_',
	/*6x*/ u'`', u'a', u'b', u'c', u'd', u'e', u'f', u'g', u'h', u'i', u'j', u'k', u'l', u'm', u'n', u'o',
	/*7x*/ u'p', u'q', u'r', u's', u't', u'u', u'v', u'w', u'x', u'y', u'z', u'{', u'|', u'}', u'~', u'\u2302'/*⌂*/,
	/*8x*/ u'\u00C7'/*Ç*/, u'\u00FC'/*ü*/, u'\u00E9'/*é*/, u'\u00E2'/*â*/, u'\u00E4'/*ä*/, u'\u00E0'/*à*/, u'\u00E5'/*å*/, u'\u00E7'/*ç*/,
	/*  */ u'\u00EA'/*ê*/, u'\u00EB'/*ë*/, u'\u00E8'/*è*/, u'\u00EF'/*ï*/, u'\u00EE'/*î*/, u'\u00EC'/*ì*/, u'\u00C4'/*Ä*/, u'\u00C5'/*Å*/,
	/*9x*/ u'\u00C9'/*É*/, u'\u00E6'/*æ*/, u'\u00C6'/*Æ*/, u'\u00F4'/*ô*/, u'\u00F6'/*ö*/, u'\u00F2'/*ò*/, u'\u00FB'/*û*/, u'\u00F9'/*ù*/,
	/*  */ u'\u00FF'/*ÿ*/, u'\u00D6'/*Ö*/, u'\u00DC'/*Ü*/, u'\u00A2'/*¢*/, u'\u00A3'/*£*/, u'\u00A5'/*¥*/, u'\u20A7'/*₧*/, u'\u0192'/*ƒ*/,
	/*Ax*/ u'\u00E1'/*á*/, u'\u00ED'/*í*/, u'\u00F3'/*ó*/, u'\u00FA'/*ú*/, u'\u00F1'/*ñ*/, u'\u00D1'/*Ñ*/, u'\u00AA'/*ª*/, u'\u00BA'/*º*/,
	/*  */ u'\u00BF'/*¿*/, u'\u2310'/*⌐*/, u'\u00AC'/*¬*/, u'\u00BD'/*½*/, u'\u00BC'/*¼*/, u'\u00A1'/*¡*/, u'\u00AB'/*«*/, u'\u00BB'/*»*/,
	/*Bx*/ u'\u2591'/*░*/, u'\u2592'/*▒*/, u'\u2593'/*▓*/, u'\u2502'/*│*/, u'\u2524'/*┤*/, u'\u2561'/*╡*/, u'\u2562'/*╢*/, u'\u2556'/*╖*/,
	/*  */ u'\u2555'/*╕*/, u'\u2563'/*╣*/, u'\u2551'/*║*/, u'\u2557'/*╗*/, u'\u255D'/*╝*/, u'\u255C'/*╜*/, u'\u255B'/*╛*/, u'\u2510'/*┐*/,
	/*Cx*/ u'\u2514'/*└*/, u'\u2534'/*┴*/, u'\u252C'/*┬*/, u'\u251C'/*├*/, u'\u2500'/*─*/, u'\u253C'/*┼*/, u'\u255E'/*╞*/, u'\u255F'/*╟*/,
	/*  */ u'\u255A'/*╚*/, u'\u2554'/*╔*/, u'\u2569'/*╩*/, u'\u2566'/*╦*/, u'\u2560'/*╠*/, u'\u2550'/*═*/, u'\u256C'/*╬*/, u'\u2567'/*╧*/,
	/*Dx*/ u'\u2568'/*╨*/, u'\u2564'/*╤*/, u'\u2565'/*╥*/, u'\u2559'/*╙*/, u'\u2558'/*╘*/, u'\u2552'/*╒*/, u'\u2553'/*╓*/, u'\u256B'/*╫*/,
	/*  */ u'\u256A'/*╪*/, u'\u2518'/*┘*/, u'\u250C'/*┌*/, u'\u2588'/*█*/, u'\u2584'/*▄*/, u'\u258C'/*▌*/, u'\u2590'/*▐*/, u'\u2580'/*▀*/,
	/*Ex*/ u'\u03B1'/*α*/, u'\u00DF'/*ß*/, u'\u0393'/*Γ*/, u'\u03C0'/*π*/, u'\u03A3'/*Σ*/, u'\u03C3'/*σ*/, u'\u00B5'/*µ*/, u'\u03C4'/*τ*/,
	/*  */ u'\u03A6'/*Φ*/, u'\u0398'/*Θ*/, u'\u03A9'/*Ω*/, u'\u03B4'/*δ*/, u'\u221E'/*∞*/, u'\u03C6'/*φ*/, u'\u03B5'/*ε*/, u'\u2229'/*∩*/,
	/*Fx*/ u'\u2261'/*≡*/, u'\u00B1'/*±*/, u'\u2265'/*≥*/, u'\u2264'/*≤*/, u'\u2320'/*⌠*/, u'\u2321'/*⌡*/, u'\u00F7'/*÷*/, u'\u2248'/*≈*/,
	/*  */ u'\u00B0'/*°*/, u'\u2219'/*∙*/, u'\u00B7'/*·*/, u'\u221A'/*√*/, u'\u207F'/*ⁿ*/, u'\u00B2'/*²*/, u'\u25A0'/*■*/, u'\u00A0'/*NBSP*/,
};

QChar df::fromCP437(char c)
{
	return CP437Table[(unsigned char)c];
}

char df::toCP437(QChar c)
{
	static const auto CP437Map = [](){
		std::map<char16_t, char> m;
		for (std::size_t i = 0; i < 32; ++i)
			m.emplace(CP437Table[i], i);
		for (std::size_t i = 127; i < 256; ++i)
			m.emplace(CP437Table[i], i);
		return m;
	}();
	if (c.unicode() >= u' ' && c.unicode() <= u'~') // common with ASCII
		return c.unicode();
	else {
		auto it = CP437Map.find(c.unicode());
		if (it == CP437Map.end())
			return '?';
		else
			return it->second;
	}
}

QString df::fromCP437(std::string_view str)
{
	QString res;
	res.reserve(str.size());
	for (auto c: str)
		res.append(fromCP437(c));
	return res;
}

std::string df::toCP437(QStringView str)
{
	std::string res;
	res.reserve(str.size());
	for (auto c: str)
		res.push_back(toCP437(c));
	return res;
}
