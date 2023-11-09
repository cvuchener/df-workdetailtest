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

#include "StandardPaths.h"

#include <QStandardPaths>
#include <QCoreApplication>

constexpr StandardPaths::Mode StandardPaths::DefaultMode;

StandardPaths::Mode StandardPaths::mode;
#ifdef DEVMODE_PATH
QDir StandardPaths::source_datadir = QDir(DEVMODE_PATH);
#else
QDir StandardPaths::source_datadir;
#endif
QDir StandardPaths::appdir;
QDir StandardPaths::custom_datadir;
QDir StandardPaths::custom_configdir;

void StandardPaths::init_paths(Mode mode, QString source_datadir)
{
	StandardPaths::mode = mode;
	if (!source_datadir.isEmpty())
		StandardPaths::source_datadir.setPath(source_datadir);

	appdir.setPath(QCoreApplication::applicationDirPath());
	switch (mode) {
	case Mode::Portable:
#if (defined Q_OS_WIN)
		custom_datadir.setPath(appdir.filePath("data"));
		custom_configdir = appdir;
#elif (defined Q_OS_OSX)
		custom_datadir.setPath(appdir.filePath("../Resources"));
		custom_configdir.setPath(appdir.filePath("../Resources"));
#elif (defined Q_OS_LINUX)
		custom_datadir.setPath(appdir.filePath("../share"));
		custom_configdir.setPath(appdir.filePath("../etc"));
#else
#	error "Unsupported OS"
#endif
		break;
	case Mode::Developer:
		custom_datadir.setPath(appdir.filePath("data"));
		custom_configdir = appdir;
		break;
	default:
		break;
	}
}

QSettings StandardPaths::settings()
{
	switch (mode) {
	case Mode::Portable:
	case Mode::Developer:
	      return {
			custom_configdir.filePath(QString("%1.ini").arg(QCoreApplication::applicationName())),
			QSettings::IniFormat
	      };
	case Mode::Standard:
	default:
	      // Organization is not set on Linux/Windows to avoid issue with QStandardPaths
	      // using "orgname/appname" folder instead "packagename". Force QSettings
	      // to use the application name for the configuration directory.
	      return {
			QSettings::IniFormat, QSettings::UserScope,
			QCoreApplication::applicationName(),
			QCoreApplication::applicationName()
	      };
	}
}

QString StandardPaths::locate_data(const QString &filename)
{
	switch (mode) {
	case Mode::Portable:
		return custom_datadir.exists(filename)
			? custom_datadir.filePath(filename)
			: QString();
	case Mode::Developer:
		if (custom_datadir.exists(filename))
			return custom_datadir.filePath(filename);
		else if (source_datadir.exists(filename))
			return source_datadir.filePath(filename);
		else
			return QString();
	case Mode::Standard:
	default:
		return QStandardPaths::locate(QStandardPaths::AppDataLocation, filename);
	}
}

QStringList StandardPaths::data_locations()
{
	switch (mode) {
	case Mode::Portable:
		return { custom_datadir.path() };
	case Mode::Developer:
		return { custom_datadir.path(), source_datadir.path() };
	case Mode::Standard:
	default:
		return QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
	}
}

QString StandardPaths::writable_data_location()
{
	switch (mode) {
	case Mode::Portable:
	case Mode::Developer:
		return custom_datadir.path();
	case Mode::Standard:
	default:
		return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	}
}

QString StandardPaths::log_location()
{
	switch (mode) {
	case Mode::Portable:
	case Mode::Developer:
		return appdir.path();
	case Mode::Standard:
	default:
#if (defined Q_OS_WIN)
		return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
			+ "\\log";
#elif (defined Q_OS_OSX)
		return QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
			+ "/Library/Logs/"
			+ QCoreApplication::applicationName();
#elif (defined Q_OS_LINUX)
		return QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
			+ "/log";
#else
#	error "Unsupported OS"
#endif
	}
}

