
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

#include "GridViewManager.h"

#include "GridViewModel.h"
#include "StandardPaths.h"
#include "LogCategory.h"

#include <QJsonDocument>

GridViewManager::GridViewManager()
{
	QStringList name_filter = {"*.json"};
	for (QDir data_dir: StandardPaths::data_locations()) {
		QDir dir = data_dir.filePath("gridviews");
		for (const auto &fi: dir.entryInfoList(name_filter, QDir::Files)) {
			QFile file(fi.filePath());
			if (!file.open(QIODeviceBase::ReadOnly)) {
				qCCritical(GridViewLog) << "Failed to open" << fi.filePath();
				continue;
			}
			QJsonParseError error;
			auto doc = QJsonDocument::fromJson(file.readAll(), &error);
			if (error.error != QJsonParseError::NoError) {
				qCCritical(GridViewLog) << "Failed to parse json from" << fi.filePath() << ":" << error.errorString();
				continue;
			}
			auto [it, inserted] = _gridviews.try_emplace(
					fi.baseName(),
					GridViewModel::Parameters::fromJson(doc));
			if (inserted)
				qCInfo(GridViewLog) << "Added gridview" << fi.baseName()
					<< "from" << fi.absoluteFilePath();
			else
				qCInfo(GridViewLog) << "Ignoring gridview" << fi.baseName()
					<< "from" << fi.absoluteFilePath();
		}
	}
}

GridViewManager::~GridViewManager()
{
}

const GridViewModel::Parameters &GridViewManager::find(QStringView name) const
{
	auto it = _gridviews.find(name);
	if (it == _gridviews.end())
		throw std::invalid_argument("Gridview name not found");
	return it->second;
}
