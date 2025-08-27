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

#include <Handle.h>
#include <Types.h>

#include <expected>
#include <vector>


namespace gan
{


enum class MemoryState
{
	Commit = 12,
	Reserve = 13,
	Free = 16,
	_Count
};
using MemoryStateFlags = Flags<MemoryState, WinDword>;

enum class MemoryProtect
{
	NoAccess = 0,
	ReadOnly = 1,
	ReadWrite = 2,
	WriteCopy = 3,
	Execute = 4,
	ExecuteRead = 5,
	ExecuteReadWrite = 6,
	ExecuteWriteCopy = 7,
	Guard = 8,
	NoCache = 9,
	WriteCombine = 10,
	TargetsInvalid = 30,
	TargetsNoUpdate = 30,
	_Count
};
using MemoryProtectFlags = Flags<MemoryProtect, WinDword>;

enum class MemoryType
{
	Private = 17,
	Mapped = 18,
	Image = 24,
	_Count
};
using MemoryTypeFlags = Flags<MemoryType, WinDword>;

struct MemoryRegion
{
	ConstMemAddr base;
	ConstMemAddr allocBase;
	size_t size;
	MemoryStateFlags state;
	MemoryProtectFlags protect;
	MemoryTypeFlags type;

	constexpr std::strong_ordering operator<=>(const MemoryRegion&) const = default;
};
using MemoryRegionList = std::vector<MemoryRegion>;


class MemoryRegionEnumerator
{
public:
	enum class Error
	{
		InaccessibleProcess,
		MemQueryFailed,
		InvalidAddressRange
	};

	static constexpr ConstMemRange k_maxRange{ ConstMemAddr{ }, ConstMemAddr{ }.Offset(-1) };

	// All regions containing the range [start, end-1]
	static std::expected<MemoryRegionList, Error> Enumerate(uint32_t pid, ConstMemRange addrRange = k_maxRange);
	static std::expected<MemoryRegionList, Error> Enumerate(WinHandle process, ConstMemRange addrRange = k_maxRange);
};


}  // namespace gan
