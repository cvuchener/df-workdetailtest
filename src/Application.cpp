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
#include "ScriptManager.h"
#include "GridViewManager.h"
#include "StructuresManager.h"

#include <QPalette>
#include <QProgressDialog>
#include <QtConcurrent>
#include <QCoroFuture>

static void setIconTheme()
{
	auto p = QApplication::palette();
	if (p.window().color().lightness() > p.windowText().color().lightness())
		QIcon::setFallbackThemeName("breeze");
	else
		QIcon::setFallbackThemeName("breeze-dark");
}

Application::Application(int &argc, char **argv):
	QApplication(argc, argv)
{
#ifdef Q_OS_LINUX
	setApplicationName("workdetailtest");
#else
	setApplicationName("WorkDetailTest");
#endif
	//setOrganizationName("WorkDetailTest");
	setApplicationDisplayName("Work Detail Test");
	setApplicationVersion("0.1");

	StandardPaths::init_paths();

	{ // Open log file
		QDir log_dir = StandardPaths::log_location();
		if (!log_dir.exists())
			log_dir.mkpath(".");
		static constexpr std::size_t log_count = 5;
		static const QString log_file = "log.txt";
		static const QString rotated_log_file = "log.txt.%1";
		log_dir.remove(rotated_log_file.arg(log_count));
		for (auto i = log_count-1; i > 0; --i)
			log_dir.rename(rotated_log_file.arg(i-1), rotated_log_file.arg(i));
		log_dir.rename(log_file, rotated_log_file.arg(1));
		MessageHandler::instance().setLogFile(log_dir.filePath(log_file));
	}
	qInfo() << applicationDisplayName() << applicationVersion();
	qInfo() << "Qt version" << QT_VERSION_STR << "(build)," << qVersion() << "(runtime)";

	setIconTheme();

	auto make_initializer = []<typename T>(std::unique_ptr<T> &member) -> std::function<void()> {
		return [&]() { member = std::make_unique<T>(); };
	};
	auto steps = {
		std::pair{ tr("Loading settings..."), make_initializer(_settings) },
		std::pair{ tr("Loading icons..."), make_initializer(_icons) },
		std::pair{ tr("Loading scripts..."), make_initializer(_scripts) },
		std::pair{ tr("Loading grid views..."), make_initializer(_gridviews) },
		std::pair{ tr("Loading structures..."), make_initializer(_structures) },
	};
	QProgressDialog progress;
	progress.setLabelText(tr("Loading data..."));
	progress.setCancelButtonText(tr("Quit"));
	progress.setRange(0, steps.size()+1);
	int i = 0;
	progress.setValue(i);
	progress.show();
	QCoro::waitFor([&]() -> QCoro::Task<> {
		for (const auto &[text, action]: steps) {
			progress.setLabelText(text);
			co_await QtConcurrent::run(action);
			if (progress.wasCanceled()) {
				qInfo() << "Loading was cancelled";
				throw std::runtime_error("loading was cancelled");
			}
			progress.setValue(++i);
		}
	}());
}

Application::~Application()
{
}

bool Application::event(QEvent *e)
{
	if (e->type() == QEvent::ApplicationPaletteChange) {
		setIconTheme();
	}
	return QApplication::event(e);
}
