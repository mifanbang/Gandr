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

#include <Types.h>

#include <string>
#include <vector>


// forward declaration
struct tagMODULEENTRY32W;  // in tlhelp32.h


namespace gan
{


struct ModuleInfo
{
	ConstMemAddr baseAddr;
	size_t size;
	std::wstring imageName;  // incl. file extension
	std::wstring imagePath;

	ModuleInfo(const ::tagMODULEENTRY32W& moduleEntry);
};


using ModuleList = std::vector<ModuleInfo>;


// ---------------------------------------------------------------------------
// Class ModuleEnumerator - Module list snapshot taker
// ---------------------------------------------------------------------------

class ModuleEnumerator
{
public:
	enum class Result : uint8_t
	{
		Success,
		SnapshotFailed,  // CreateToolhelp32Snapshot() failed
		Module32Failed,  // A Module32*() function failed
	};

	static Result Enumerate(uint32_t processId, ModuleList& out);
};


}  // namespace gan
