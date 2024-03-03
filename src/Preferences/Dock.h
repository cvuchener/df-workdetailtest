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

#ifndef PREFERENCES_DOCK_H
#define PREFERENCES_DOCK_H

#include <QDockWidget>

class DwarfFortressData;

namespace Ui { class PreferencesDock; }

namespace Preferences
{

class Model;

class Dock: public QDockWidget
{
	Q_OBJECT
public:
	Dock(std::shared_ptr<const DwarfFortressData> df, QWidget *parent = nullptr);
	~Dock() override;

private:
	std::unique_ptr<Ui::PreferencesDock> _ui;
	std::shared_ptr<const DwarfFortressData> _df;
	std::unique_ptr<Model> _model;
};

} // namespace Preferences

#endif
