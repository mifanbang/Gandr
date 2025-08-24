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

#include <Types.h>

#include <vector>


namespace gan
{


struct MemoryRegion
{
	ConstMemAddr base;
	size_t size;
	uint32_t state;
	uint32_t protect;
	uint32_t type;

	constexpr std::strong_ordering operator<=>(const MemoryRegion& other) const noexcept = default;
};
using MemoryRegionList = std::vector<MemoryRegion>;


class MemoryRegionEnumerator
{
public:
	static MemoryRegionList Enumerate(uint32_t pid)
	{
		return Enumerate(pid, { ConstMemAddr{ }, ConstMemAddr{ reinterpret_cast<const void*>(-1) } });
	}

	// All regions containing the range [start, end-1]
	static MemoryRegionList Enumerate(uint32_t pid, Range<ConstMemAddr> range);
};


}  // namespace gan
