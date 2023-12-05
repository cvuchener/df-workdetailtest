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

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QPersistentModelIndex>
#include "DwarfFortress.h"

class GridViewModel;
class QSortFilterProxyModel;

namespace Ui { class MainWindow; }

class QStyle;

class MainWindow: public QMainWindow
{
	Q_OBJECT
public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;

private slots:
	void onStateChanged(DwarfFortress::State state);
	void updateTemporaryFilter();
	void addFilter();

	// auto-connected slots
	void on_connect_action_triggered();
	void on_advanced_connection_action_triggered();
	void on_disconnect_action_triggered();
	void on_update_action_triggered();
	void on_preferences_action_triggered();
	void on_about_action_triggered();

private:
	enum class FilterType {
		Simple,
		Regex,
	};
	enum class BuiltinFilter {
		Worker,
	};

	std::unique_ptr<Ui::MainWindow> _ui;
	struct StatusBarUi;
	std::unique_ptr<StatusBarUi> _sb_ui;

	std::unique_ptr<DwarfFortress> _df;
	std::unique_ptr<GridViewModel> _model;
	std::unique_ptr<QSortFilterProxyModel> _sort_model;
	QPersistentModelIndex _current_unit;
};

#endif
