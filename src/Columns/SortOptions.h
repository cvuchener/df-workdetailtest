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

#ifndef COLUMNS_SORT_OPTIONS_H
#define COLUMNS_SORT_OPTIONS_H

#include <QMenu>
#include <QAction>
#include <QCoreApplication>

namespace Columns {

template <typename T, typename Enum>
struct SortOptions
{
	T &parent;
	Enum option;
	std::map<Enum, QString> names;

	void makeSortMenu(QMenu *menu)
	{
		menu->addSection(QCoreApplication::translate("sort menu", "Sort by"));
		for (const auto &[value, name]: names) {
			auto action = new QAction(name, menu);
			action->setCheckable(true);
			action->setChecked(option == value);
			menu->addAction(action);
			QObject::connect(action, &QAction::triggered, [this, value]() {
				option = value;
				parent.columnDataChanged(0, parent.count()-1);
			});
		}
	}
};

}

#endif
