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

#ifndef STRUCTURES_MANAGER_H
#define STRUCTURES_MANAGER_H

#include <dfs/Structures.h>

#include <QString>

class StructuresManager
{
public:
	StructuresManager();
	~StructuresManager();

	using process_id_t = std::span<const uint8_t>;
	struct StructuresInfo
	{
		const dfs::Structures *structures;
		const dfs::Structures::VersionInfo *version;
		QString source;
	};

	const auto &allVersions() const { return _structures_by_id; }
	const StructuresInfo *findVersion(process_id_t id) const;

	static std::string idToString(process_id_t);

private:
	std::vector<std::unique_ptr<dfs::Structures>> _structures;
	struct IdLess {
		bool operator()(process_id_t, process_id_t) const;
	};
	std::map<std::span<const uint8_t>, StructuresInfo, IdLess> _structures_by_id;
};

#endif
