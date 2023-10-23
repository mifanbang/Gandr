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

#pragma once

#include <string>
#include <vector>


// forward declaration
struct tagPROCESSENTRY32W;  // in tlhelp32.h


namespace gan
{


struct ProcessInfo
{
	uint32_t pid;
	uint32_t nThread;
	uint32_t pidParent;
	uint32_t basePriority;
	std::wstring imageName;  // incl. file extension

	ProcessInfo(const ::tagPROCESSENTRY32W& procEntry);
};


using ProcessList = std::vector<ProcessInfo>;


// ---------------------------------------------------------------------------
// Class ProcessEnumerator - process list snapshot taker
// ---------------------------------------------------------------------------

class ProcessEnumerator
{
public:
	enum class Result : uint8_t
	{
		Success,
		SnapshotFailed,  // CreateToolhelp32Snapshot() failed
		Process32Failed,  // A Process32*() function failed
	};

	static Result Enumerate(ProcessList& out);
};


}  // namespace gan
