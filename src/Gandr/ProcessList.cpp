/*
 *  Gandr - another minimalism library for hacking x86-based Windows
 *  Copyright (C) 2020-2025 Mifan Bang <https://debug.tw>.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <ProcessList.h>

#include <Handle.h>

#include <windows.h>
#include <tlhelp32.h>  // must be included after windows.h, which sucks


namespace
{


constexpr gan::ProcessInfo MakeProcessInfo(const PROCESSENTRY32W& procEntry)
{
	ABOVE_NORMAL_PRIORITY_CLASS;
	return {
		.pid{ procEntry.th32ProcessID },
		.nThread{ procEntry.cntThreads },
		.pidParent{ procEntry.th32ParentProcessID },
		.basePriority{ static_cast<uint32_t>(procEntry.pcPriClassBase) },  // Base priority defined in [0, 31], REF: https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities
		.imageName{ procEntry.szExeFile }
	};
}

constexpr gan::ThreadInfo MakeThreadInfo(const THREADENTRY32& threadEntry)
{
	return {
		.tid{ threadEntry.th32ThreadID },
		.pidParent{ threadEntry.th32OwnerProcessID },
		.basePriority{ static_cast<uint32_t>(threadEntry.tpBasePri) }  // Base priority defined in [0, 31], REF: https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities
	};
}


}  // unnamed namespace


namespace gan
{


std::expected<ProcessList, ProcessEnumerator::Error> ProcessEnumerator::operator()()
{
	constexpr uint32_t k_ignoredParam = 0;

	AutoWinHandle hSnap{ ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, k_ignoredParam) };
	if (!hSnap)
		return std::unexpected{ Error::SnapshotFailed };

	ProcessList procList;
	PROCESSENTRY32W procEntry{ .dwSize = sizeof(procEntry) };

	for (BOOL proc32Result{ ::Process32FirstW(*hSnap, &procEntry) };
		proc32Result;
		proc32Result = ::Process32NextW(*hSnap, &procEntry))
	{
		procList.emplace_back(MakeProcessInfo(procEntry));
	}

	// In a success, Process32Next() would end with returning FALSE and setting error code to ERROR_NO_MORE_FILES
	if (procList.empty() || ::GetLastError() != ERROR_NO_MORE_FILES)
		return std::unexpected{ Error::Process32Failed };

	return procList;
}

std::expected<ThreadList, ThreadEnumerator::Error> ThreadEnumerator::operator()()
{
	constexpr uint32_t k_allProcesses = 0;
	return (*this)(k_allProcesses);
}

std::expected<ThreadList, ThreadEnumerator::Error> ThreadEnumerator::operator()(uint32_t pid)
{
	AutoWinHandle hSnap{ ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, pid) };
	if (!hSnap)
		return std::unexpected{ Error::SnapshotFailed };

	ThreadList threadList;
	THREADENTRY32 threadEntry{ .dwSize = sizeof(threadEntry) };

	for (BOOL thread32Result{ ::Thread32First(*hSnap, &threadEntry) };
		thread32Result;
		thread32Result = ::Thread32Next(*hSnap, &threadEntry))
	{
		threadList.emplace_back(MakeThreadInfo(threadEntry));
	}

	// In a success, Process32Next() would end with returning FALSE and setting error code to ERROR_NO_MORE_FILES
	if (threadList.empty() || ::GetLastError() != ERROR_NO_MORE_FILES)
		return std::unexpected{ Error::Thread32Failed };

	return threadList;
}


}  // namespace gan
