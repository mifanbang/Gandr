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

#include <Memory.h>

#include <Handle.h>

#include <windows.h>


namespace gan
{


std::expected<MemoryRegionList, MemoryRegionEnumerator::Error> MemoryRegionEnumerator::Enumerate(uint32_t pid, Range<ConstMemAddr> addrRange)
{
	MemoryRegionList regions;

	if (addrRange.min > addrRange.max)
		return std::unexpected{ Error::InvalidAddressRange };

	if (AutoWinHandle process{ ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid) };
		process)
	{
		constexpr auto k_initListSize = 64zu;  // Every process likely has >= 32 memory regions
		regions.reserve(k_initListSize);

		::MEMORY_BASIC_INFORMATION memInfo;
		for (auto addr = addrRange.min;
			addr < addrRange.max;
			addr = addr.Offset(ConstMemAddr{ memInfo.BaseAddress } - addr + memInfo.RegionSize))
		{
			if (const auto queryResult = ::VirtualQueryEx(*process, addr.ConstPtr(), &memInfo, sizeof(memInfo));
				queryResult == 0)
			{
				if (::GetLastError() == ERROR_INVALID_PARAMETER)  // Highest memory address accessible hit
					break;
				else
					return std::unexpected{ Error::MemQueryFailed };
			}

			if (memInfo.State != MEM_FREE)
			{
				regions.emplace_back(MemoryRegion{
					.base{ memInfo.BaseAddress },
					.size{ memInfo.RegionSize },
					.state{ memInfo.State },
					.protect{ memInfo.Protect },
					.type{ memInfo.Type }
				});
			}
		}
	}
	else
		return std::unexpected{ Error::OpenProcessFailed };

	return regions;
}


}  // namespace gan