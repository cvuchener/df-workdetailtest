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

#include "StructuresManager.h"
#include "LogCategory.h"
#include "StandardPaths.h"
#include "DwarfFortressReader.h"

static void StructuresLogger(std::string_view msg)
{
	qCWarning(StructuresLog) << msg;
};

bool StructuresManager::IdLess::operator()(process_id_t lhs, process_id_t rhs) const
{
	return std::ranges::lexicographical_compare(lhs, rhs);
}

StructuresManager::StructuresManager()
{
	for (QDir data_dir: StandardPaths::data_locations()) {
		QDir structs_dir = data_dir.filePath("structures");
		if (!structs_dir.exists())
			continue;
		for (const auto &subdir: structs_dir.entryList({},
					QDir::Dirs | QDir::NoDotAndDotDot,
					QDir::Name  | QDir::Reversed)) {
			QDir struct_dir = structs_dir.filePath(subdir);
			qCInfo(StructuresLog) << "Loading structures from"
				<< struct_dir.absolutePath();
			try {
				auto structures = std::make_unique<dfs::Structures>(
						struct_dir.filesystemAbsolutePath(),
						StructuresLogger);
				if (!DwarfFortressReader::testStructures(*structures)) {
					qCCritical(StructuresLog) << "Incompatible structures";
					continue;
				}
				for (const auto &version: structures->allVersions()) {
					auto [it, inserted] = _structures_by_id.try_emplace(
							version.id,
							StructuresInfo{
								structures.get(),
								&version,
								struct_dir.absolutePath()
							});
					if (inserted) {
						qCInfo(StructuresLog) << "Adding version"
							<< version.version_name
							<< idToString(version.id)
							<< "from" << struct_dir.absolutePath();
					}
					else {
						qCInfo(StructuresLog) << "Version already added"
							<< version.version_name
							<< idToString(version.id)
							<< "from" << it->second.source
							<< "as" << it->second.version->version_name;
					}
				}
				_structures.push_back(std::move(structures));
			}
			catch (std::exception &e) {
				qCCritical(StructuresLog) << "Failed to load structures from"
					<< struct_dir.absolutePath() << e.what();
			}
		}
	}
}

StructuresManager::~StructuresManager()
{
}

const StructuresManager::StructuresInfo *StructuresManager::findVersion(process_id_t id) const
{
	auto it = _structures_by_id.find(id);
	if (it == _structures_by_id.end())
		return nullptr;
	else
		return &it->second;
}

std::string StructuresManager::idToString(process_id_t id)
{
	std::string out;
	out.reserve(2*id.size());
	auto it = std::back_inserter(out);
	for (auto byte: id)
		it = std::format_to(it, "{:02x}", byte);
	return out;
}
