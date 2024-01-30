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

#include "DwarfFortress.h"

#include <dfs/LinuxProcess.h>
#include <dfs/WineProcess.h>

#include <filesystem>
#include <charconv>
#include <fstream>

std::unique_ptr<dfs::Process> DwarfFortress::findNativeProcess(const dfproto::workdetailtest::ProcessInfo &info)
{
	auto check_cookie = [&info](dfs::Process &process) {
		uint32_t cookie;
		if (auto err = process.read_sync(info.cookie_address(), cookie))
			throw std::system_error(err);
		if (cookie != info.cookie_value())
			throw std::runtime_error("Cookie mismatch");
	};
	try {
		auto process = std::make_unique<dfs::LinuxProcess>(info.pid());
		check_cookie(*process);
		qCInfo(ProcessLog) << "Process found using available info" << info.pid();
		return process;
	}
	catch (std::exception &e) {
		qCWarning(ProcessLog) << "Invalid process" << info.pid() << e.what();
	}
	for (auto entry: std::filesystem::directory_iterator("/proc")) {
		if (!entry.is_directory())
			continue;
		std::string name = entry.path().filename().native();
		uint32_t pid;
		auto res = std::from_chars(name.data(), name.data()+name.size(), pid);
		if (res.ec != std::errc{} || res.ptr != name.data()+name.size())
			continue;
		if (auto file = std::ifstream(entry.path() / "comm")) {
			std::string comm;
			if (!getline(file, comm))
				continue;
			try {
				std::unique_ptr<dfs::Process> process;
				if (comm == "dwarfort")
					process = std::make_unique<dfs::LinuxProcess>(pid);
				else if (comm == "Dwarf Fortress.")
					process = std::make_unique<dfs::WineProcess>(pid);
				else
					continue;
				check_cookie(*process);
				qCInfo(ProcessLog) << "Process found using comm value" << info.pid() << comm;
				return process;
			}
			catch (std::exception &e) {
				qCWarning(ProcessLog) << "Invalid process" << pid << e.what();
			}
		}
		else
			qCWarning(ProcessLog) << "Failed to open comm file" << pid;
	}
	return nullptr;
}
