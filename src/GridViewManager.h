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

#ifndef GRID_VIEW_MANAGER_H
#define GRID_VIEW_MANAGER_H

#include <map>
#include <memory>
#include <ranges>

#include <QString>
#include <QJsonDocument>

class GridViewModel;
class DwarfFortress;

class GridViewManager
{
public:
	GridViewManager();
	~GridViewManager();

	std::unique_ptr<GridViewModel> makeGridView(QStringView name, DwarfFortress &df);

	auto gridviews() const { return _gridviews | std::views::keys; }

private:
	std::map<QString, QJsonDocument, std::less<>> _gridviews;
};

#endif
