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

#include "IconProvider.h"

#include "StandardPaths.h"

#include <QImageReader>

IconProvider::IconProvider()
{
	struct {
		QStringList dirs = StandardPaths::data_locations();
		QIcon operator()(const QString &subdir, const QString &base_name) const
		{
			QStringList name_filter = {base_name + ".*"};
			for (const auto &data_dir: dirs) {
				QDir dir = data_dir + "/icons/" + subdir;
				for (const auto &file: dir.entryList(name_filter, QDir::Files)) {
					auto path = dir.filePath(file);
					if (!QImageReader::imageFormat(path).isEmpty())
						return QIcon(path);
				}
			}
			qWarning() << "Icon not found:" << subdir << "/" << base_name;
			return {};
		}
	} icon_locator;
	for (std::size_t i = 0; i < df::work_detail_icon::Count; ++i)
	{
		auto name = QString::fromLocal8Bit(to_string(static_cast<df::work_detail_icon_t>(i)));
		_workdetails[i] = icon_locator("workdetails", name);
	}
}

IconProvider::~IconProvider()
{
}

QIcon IconProvider::workdetail(df::work_detail_icon_t icon) const
{
	auto i = static_cast<std::size_t>(icon);
	if (i < _workdetails.size())
		return _workdetails[i];
	else
		return {};
}
