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

#ifndef DFHACK_PROCESS_H
#define DFHACK_PROCESS_H

#include <dfs/Process.h>

namespace DFHack { class Client; }

class DFHackProcess: public dfs::Process
{
public:
	DFHackProcess(DFHack::Client &client);
	~DFHackProcess() override;

	std::span<const uint8_t> id() const override
	{
		return _id;
	}

	intptr_t base_offset() const override
	{
		return _base_offset;
	}

	std::error_code stop() override;
	std::error_code cont() override;

	[[nodiscard]] cppcoro::task<std::error_code> read(dfs::MemoryBufferRef buffer) override;
	[[nodiscard]] cppcoro::task<std::error_code> readv(std::span<const dfs::MemoryBufferRef> tasks) override;
	void sync(cppcoro::task<> &&task) override;

private:
	DFHack::Client &_client;
	std::vector<uint8_t> _id;
	intptr_t _base_offset;

};

#endif
