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

#ifndef STANDARD_PATHS_H
#define STANDARD_PATHS_H

#include <QDir>
#include <QSettings>
#include <memory>

class StandardPaths
{
public:
	enum class Mode {
		Standard,
		Portable,
		Developer,
	};

	static inline constexpr Mode DefaultMode =
#if defined BUILD_PORTABLE
		Mode::Portable;
#elif defined DEVMODE_PATH
		Mode::Developer;
#else
		Mode::Standard;
#endif

	static void init_paths(Mode mode = DefaultMode, QString source_datadir = {});

	static QSettings settings();
	static QString locate_data(const QString &filename);
	static QStringList data_locations();
	static QString writable_data_location();
	static QString log_location();

private:
	static Mode mode;
	static QDir source_datadir;
	static QDir appdir;
	static QDir custom_datadir, custom_configdir;
};

#endif
