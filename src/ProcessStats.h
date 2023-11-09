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

#ifndef PROCESS_STATS_H
#define PROCESS_STATS_H

#include <fstream>
#include <chrono>

#include <dfs/Process.h>

class ProcessStats: public dfs::ProcessWrapper
{
public:
	ProcessStats(std::unique_ptr<Process> &&p);
	~ProcessStats() override;

	std::error_code stop() override;
	std::error_code cont() override;

	[[nodiscard]] cppcoro::task<std::error_code> read(dfs::MemoryBufferRef buffer) override;
	[[nodiscard]] cppcoro::task<std::error_code> readv(std::span<const dfs::MemoryBufferRef> tasks) override;

private:
	std::size_t _read_count;
	std::size_t _bytes_count;
	std::chrono::steady_clock::duration _total_duration;
	std::chrono::steady_clock::time_point _session_start;
	std::ofstream _output;
};


#endif
