/*
 *  Gandr - another minimalism library for hacking x86-based Windows
 *  Copyright (C) 2020-2023 Mifan Bang <https://debug.tw>.
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


namespace gan
{


ProcessInfo::ProcessInfo(const PROCESSENTRY32W& procEntry)
	: pid(procEntry.th32ProcessID)
	, nThread(procEntry.cntThreads)
	, pidParent(procEntry.th32ParentProcessID)
	, basePriority(procEntry.pcPriClassBase)
	, imageName(procEntry.szExeFile)
{
}


ThreadInfo::ThreadInfo(const THREADENTRY32& threadEntry)
	: tid(threadEntry.th32ThreadID)
	, pidParent(threadEntry.th32OwnerProcessID)
	, basePriority(threadEntry.tpBasePri)
{
}


ProcessEnumerator::Result ProcessEnumerator::Enumerate(ProcessList& out)
{
	constexpr uint32_t k_ignoredParam = 0;

	AutoWinHandle hSnap{ ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, k_ignoredParam) };
	if (!hSnap)
		return Result::SnapshotFailed;

	ProcessList newProcList;
	PROCESSENTRY32W procEntry{ .dwSize = sizeof(procEntry) };

	BOOL proc32Result = ::Process32FirstW(*hSnap, &procEntry);
	while (proc32Result == TRUE)
	{
		newProcList.emplace_back(procEntry);
		proc32Result = ::Process32NextW(*hSnap, &procEntry);
	}

	// Process32Next() ends with returning FALSE and setting error code to ERROR_NO_MORE_FILES
	if (proc32Result == FALSE && ::GetLastError() != ERROR_NO_MORE_FILES)
		return Result::Process32Failed;

	out = std::move(newProcList);
	return Result::Success;
}


ThreadEnumerator::Result ThreadEnumerator::Enumerate(ThreadList& out)
{
	return Enumerate(std::nullopt, out);
}

ThreadEnumerator::Result ThreadEnumerator::Enumerate(std::optional<uint32_t> pid, ThreadList& out)
{
	constexpr uint32_t k_ignoredParam = 0;

	AutoWinHandle hSnap{ ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, k_ignoredParam) };
	if (!hSnap)
		return Result::SnapshotFailed;

	ThreadList resultThreadList;
	THREADENTRY32 threadEntry{ .dwSize = sizeof(threadEntry) };

	BOOL thread32Result = ::Thread32First(*hSnap, &threadEntry);
	while (thread32Result == TRUE)
	{
		if (!pid || *pid == threadEntry.th32OwnerProcessID)
			resultThreadList.emplace_back(threadEntry);
		thread32Result = ::Thread32Next(*hSnap, &threadEntry);
	}

	// Process32Next() ends with returning FALSE and setting error code to ERROR_NO_MORE_FILES
	if (thread32Result == FALSE && ::GetLastError() != ERROR_NO_MORE_FILES)
		return Result::Thread32Failed;

	out = std::move(resultThreadList);
	return Result::Success;
}


}  // namespace gan
