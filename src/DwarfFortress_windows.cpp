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

#include "LogCategory.h"

#include <dfs/Win32Process.h>

extern "C" {
#include <windows.h>
}

static void check_cookie(dfs::Process &process, const dfproto::workdetailtest::ProcessInfo &info) {
	uint32_t cookie;
	if (auto err = process.read_sync(info.cookie_address(), cookie))
		throw std::system_error(err);
	if (cookie != info.cookie_value())
		throw std::runtime_error("Cookie mismatch");
};

static QString get_error_string(DWORD error) {
	LPWSTR bufPtr = NULL;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, error, 0, (LPWSTR)&bufPtr, 0, NULL);
	const QString result = bufPtr
		? QString::fromWCharArray(bufPtr).trimmed()
		: QString("Unknown Error %1").arg(error);
	LocalFree(bufPtr);
	return result;
}

struct enum_windows_proc_params
{
	std::unique_ptr<dfs::Process> process;
	const dfproto::workdetailtest::ProcessInfo &info;
};


static BOOL enum_windows_proc(HWND hwnd, LPARAM param)
{
	using namespace std::literals;
	auto p = reinterpret_cast<enum_windows_proc_params *>(param);
	wchar_t classname[32];
	if (!GetClassNameW(hwnd, classname, std::size(classname))) {
		auto err = GetLastError();
		qCCritical(ProcessLog) << "GetClassNameW" << get_error_string(err);
		return true;
	}
	if (classname != L"OpenGL"sv && classname != L"SDL_app"sv)
		return true;
	wchar_t windowname[32];
	if (!GetWindowTextW(hwnd, windowname, std::size(windowname))) {
		auto err = GetLastError();
		if (err != ERROR_SUCCESS)
			qCCritical(ProcessLog) << "GetWindowTextW" << get_error_string(err);
		return true;
	}
	if (windowname != L"Dwarf Fortress"sv)
		return true;
	int pid = 0;
	if (!(pid = GetWindowThreadProcessId(hwnd, nullptr))) {
		auto err = GetLastError();
		qCCritical(ProcessLog) << "GetWindowThreadProcessId" << get_error_string(err);
		return true;
	}
	try {
		auto process = std::make_unique<dfs::Win32Process>(pid);
		check_cookie(*process, p->info);
		qCInfo(ProcessLog) << "Process found from enumeration" << pid;
		p->process = std::move(process);
		return false;
	}
	catch (std::exception &e) {
		qCWarning(ProcessLog) << "Invalid process" << pid << e.what();
		return true;
	}
}


std::unique_ptr<dfs::Process> DwarfFortress::findNativeProcess(const dfproto::workdetailtest::ProcessInfo &info)
{
	try {
		auto process = std::make_unique<dfs::Win32Process>(info.pid());
		check_cookie(*process, info);
		qCInfo(ProcessLog) << "Process found using available info" << info.pid();
		return process;
	}
	catch (std::exception &e) {
		qCWarning(ProcessLog) << "Invalid process" << info.pid() << e.what();
	}
	enum_windows_proc_params params = {
		.process = nullptr,
		.info = info,
	};
	if (!EnumWindows(enum_windows_proc, reinterpret_cast<intptr_t>(&params))) {
		auto err = GetLastError();
		qCCritical(ProcessLog) << "EnumWindows" << get_error_string(err);
		return nullptr;
	}
	return std::move(params.process);
}
