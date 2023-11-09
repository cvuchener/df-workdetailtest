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

#include "DFHackProcess.h"

#include <QCoroFuture>
#include <QCoroTask>

#include <dfhack-client-qt/Client.h>
#include <dfhack-client-qt/Function.h>
#include <dfhack-client-qt/Core.h>
#include "llmemreader.pb.h"

static const DFHack::Core Core;

static const struct {
	DFHack::Function<
		dfproto::EmptyMessage,
		dfproto::llmemoryreader::Info
	> GetInfo = {"llmemreader", "GetInfo"};
	DFHack::Function<
		dfproto::llmemoryreader::ReadRawIn,
		dfproto::llmemoryreader::ReadRawOut
	> ReadRaw = {"llmemreader", "ReadRaw"};
	DFHack::Function<
		dfproto::llmemoryreader::ReadRawVIn,
		dfproto::llmemoryreader::ReadRawVOut
	> ReadRawV = {"llmemreader", "ReadRawV"};
} llmemoryreader;

static uint8_t parse_hexdigit(char c)
{
	if (c >= '0' && c <= '9')
		return c-'0';
	else if (c >= 'A' && c <= 'F')
		return 10+c-'A';
	else if (c >= 'a' && c <= 'f')
		return 10+c-'a';
	else
		throw std::invalid_argument("not a hexadecimal digit");
}

DFHackProcess::DFHackProcess(DFHack::Client &client):
	_client(client)
{
	auto [reply, notifications] = llmemoryreader.GetInfo(_client);
	reply.waitForFinished();
	auto info = reply.result();
	if (!info)
		throw std::system_error(info.cr);
	if (info->has_pe()) {
		uint32_t pe = info->pe();
		_id.resize(sizeof(pe));
		std::memcpy(_id.data(), &pe, sizeof(pe));
		std::ranges::reverse(_id);
	}
	else if (info->has_md5()) {
		Q_ASSERT(info->md5().size() == 32);
		_id.resize(16);
		for (std::size_t i = 0; i < 16; ++i)
			_id[i] = parse_hexdigit(info->md5()[2*i]) * 16
				+ parse_hexdigit(info->md5()[2*i+1]);
	}
	else {
		throw std::runtime_error("Missing PE timestamp/MD5 sum");
	}
	_base_offset = info->base_offset();
}

DFHackProcess::~DFHackProcess()
{
}

std::error_code DFHackProcess::stop()
{
	auto [reply, notifications] = Core.suspend(_client);
	reply.waitForFinished();
	return reply.result().cr;
}

std::error_code DFHackProcess::cont()
{
	auto [reply, notifications] = Core.resume(_client);
	reply.waitForFinished();
	return reply.result().cr;
}

[[nodiscard]] cppcoro::task<std::error_code> DFHackProcess::read(dfs::MemoryBufferRef buffer)
{
	auto args = llmemoryreader.ReadRaw.args();
	args.set_address(buffer.address);
	args.set_length(buffer.data.size());
	auto [reply, notifications] = llmemoryreader.ReadRaw(_client, args);
	auto r = co_await qCoro(reply).waitForFinished();
	if (!r)
		co_return r.cr;
	if (r->has_data() && r->data().size() == buffer.data.size()) {
		std::memcpy(buffer.data.data(), r->data().data(), buffer.data.size());
		co_return std::error_code{};
	}
	else {
		qWarning() << "read error:" << r->error_message();
		co_return DFHack::CommandResult::Failure;
	}
}

[[nodiscard]] cppcoro::task<std::error_code> DFHackProcess::readv(std::span<const dfs::MemoryBufferRef> tasks)
{
	auto args = llmemoryreader.ReadRawV.args();
	for (const auto &task: tasks) {
		auto in = args.add_list();
		in->set_address(task.address);
		in->set_length(task.data.size());
	}
	auto [reply, notifications] = llmemoryreader.ReadRawV(_client, args);
	auto r = co_await qCoro(reply).waitForFinished();
	if (!r)
		co_return r.cr;
	for (std::size_t i = 0; i < tasks.size(); ++i) {
		auto out = r->list(i);
		auto &task = tasks[i];
		if (out.has_data() && out.data().size() == task.data.size()) {
			std::memcpy(task.data.data(), out.data().data(), task.data.size());
		}
		else {
			qWarning() << "read error:" << out.error_message();
			co_return DFHack::CommandResult::Failure;
		}
	}
	co_return std::error_code{};
}

void DFHackProcess::sync(cppcoro::task<> &&task)
{
	QCoro::waitFor(std::move(task));
}
