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

#include "Application.h"
#include "StandardPaths.h"
#include "MessageHandler.h"
#include "IconProvider.h"

Application::Application(int &argc, char **argv):
	QApplication(argc, argv)
{
	setApplicationName("WorkDetailTest");
	//setOrganizationName("WorkDetailTest");
	setApplicationVersion("0.1");

	StandardPaths::init_paths();

	{ // Open log file
		QDir log_dir = StandardPaths::log_location();
		static constexpr std::size_t log_count = 5;
		static const QString log_file = "log.txt";
		static const QString rotated_log_file = "log.txt.%1";
		log_dir.remove(rotated_log_file.arg(log_count));
		for (auto i = log_count-1; i > 0; --i)
			log_dir.rename(rotated_log_file.arg(i-1), rotated_log_file.arg(i));
		log_dir.rename(log_file, rotated_log_file.arg(1));
		MessageHandler::instance().setLogFile(log_dir.filePath(log_file));
	}

	_settings = std::make_unique<Settings>();

	_icons = std::make_unique<IconProvider>();
}

Application::~Application()
{
}
