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

#include "ProcessStats.h"

#include "DwarfFortress.h"

ProcessStats::ProcessStats(std::unique_ptr<Process> &&p):
	ProcessWrapper(std::move(p)),
	_output("read_stats.dat")
{
}

ProcessStats::~ProcessStats()
{
}

std::error_code ProcessStats::stop()
{
	_read_count = 0;
	_bytes_count = 0;
	_total_duration = decltype(_total_duration)::zero();
	_session_start = std::chrono::steady_clock::now();
	return ProcessWrapper::stop();
}

std::error_code ProcessStats::cont()
{
	auto ret = ProcessWrapper::cont();
	using namespace std::chrono;
	auto session_end = steady_clock::now();
	qInfo(ProcessLog) << "ReadSession Stats";
	qInfo(ProcessLog) << "read count:" << _read_count;
	qInfo(ProcessLog) << "bytes read:" << _bytes_count;
	qInfo(ProcessLog) << "read duration (ms):"
		<< duration_cast<milliseconds>(_total_duration).count();
	qInfo(ProcessLog) << "bandwidth (MB/s):"
		<< _bytes_count/duration<double>(_total_duration).count()/1024.0/1024.0;
	qInfo(ProcessLog) << "session duration (ms):"
		<< duration_cast<milliseconds>(session_end - _session_start).count();
	return ret;
}

[[nodiscard]] cppcoro::task<std::error_code> ProcessStats::read(dfs::MemoryBufferRef buffer)
{
	auto start = std::chrono::steady_clock::now();
	auto res = co_await process().read(buffer);
	auto end = std::chrono::steady_clock::now();
	auto duration = end - start;
	_output << buffer.data.size() << "\t"
		<< std::chrono::duration<double, std::micro>(duration).count() << "\t"
		<< buffer.data.size()/std::chrono::duration<double>(duration).count()/1024.0/1024.0 << "\n";
	++_read_count;
	++_bytes_count += buffer.data.size();
	_total_duration += duration;
	co_return res;
}

[[nodiscard]] cppcoro::task<std::error_code> ProcessStats::readv(std::span<const dfs::MemoryBufferRef> tasks)
{
	auto start = std::chrono::steady_clock::now();
	auto res = co_await process().readv(tasks);
	auto end = std::chrono::steady_clock::now();
	auto duration = end - start;
	std::size_t len = 0;
	for (const auto &t: tasks)
		len += t.data.size();
	_output << len << "\t"
		<< std::chrono::duration<double, std::micro>(duration).count() << "\t"
		<< len/std::chrono::duration<double>(duration).count()/1024.0/1024.0 << "\n";
	++_read_count;
	++_bytes_count += len;
	_total_duration += duration;
	co_return res;
}
