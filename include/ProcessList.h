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

#pragma once

#include <expected>
#include <optional>
#include <string>
#include <vector>


namespace gan
{


struct ProcessInfo
{
	uint32_t pid;
	uint32_t nThread;
	uint32_t pidParent;
	uint32_t basePriority;
	std::wstring imageName;  // incl. file extension
};
using ProcessList = std::vector<ProcessInfo>;


struct ThreadInfo
{
	uint32_t tid;
	uint32_t pidParent;
	uint32_t basePriority;
};
using ThreadList = std::vector<ThreadInfo>;


// ---------------------------------------------------------------------------
// Class ProcessEnumerator - process list snapshot helper
// ---------------------------------------------------------------------------

class ProcessEnumerator
{
public:
	enum class Error
	{
		SnapshotFailed,  // CreateToolhelp32Snapshot() failed
		Process32Failed,  // A Process32*() function failed
	};

	static std::expected<ProcessList, Error> Enumerate();
};


// ---------------------------------------------------------------------------
// Class ProcessEnumerator - thread list snapshot helper
// ---------------------------------------------------------------------------

class ThreadEnumerator
{
public:
	enum class Error
	{
		SnapshotFailed,  // CreateToolhelp32Snapshot() failed
		Thread32Failed,  // A Thread32*() function failed
	};

	static std::expected<ThreadList, Error> Enumerate();
	static std::expected<ThreadList, Error> Enumerate(uint32_t pid);
};



}  // namespace gan
