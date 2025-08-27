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


std::expected<MemoryRegionList, MemoryRegionEnumerator::Error> MemoryRegionEnumerator::Enumerate(uint32_t pid, ConstMemRange addrRange)
{
	return Enumerate(::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid), addrRange);
}

std::expected<MemoryRegionList, MemoryRegionEnumerator::Error> MemoryRegionEnumerator::Enumerate(WinHandle process, ConstMemRange addrRange)
{
	MemoryRegionList regions;

	if (addrRange.min > addrRange.max)
		return std::unexpected{ Error::InvalidAddressRange };

	if (AutoWinHandle processDup{ gan::HandleHelper::Duplicate(process) })
	{
		constexpr auto k_initListSize = 64zu;  // Every process likely has >= 32 memory regions
		regions.reserve(k_initListSize);

		::MEMORY_BASIC_INFORMATION memInfo{ };
		for (auto addr = addrRange.min;
			addr < addrRange.max;
			addr = addr.Offset(ConstMemAddr{ memInfo.BaseAddress } - addr + memInfo.RegionSize))
		{
			if (const auto queryResult = ::VirtualQueryEx(*processDup, addr.ConstPtr(), &memInfo, sizeof(memInfo));
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
					.allocBase{ memInfo.AllocationBase },
					.size{ memInfo.RegionSize },
					.state{ memInfo.State },
					.protect{ memInfo.Protect },
					.type{ memInfo.Type }
					});
			}
		}
	}
	else
	{
		return std::unexpected{ Error::InaccessibleProcess };
	}

	return regions;
}


static_assert(MemoryStateFlags{ MemoryState::Commit } == MEM_COMMIT);
static_assert(MemoryStateFlags{ MemoryState::Free } == MEM_FREE);
static_assert(MemoryStateFlags{ MemoryState::Reserve } == MEM_RESERVE);

static_assert(MemoryProtectFlags{ MemoryProtect::Execute } == PAGE_EXECUTE);
static_assert(MemoryProtectFlags{ MemoryProtect::ExecuteRead } == PAGE_EXECUTE_READ);
static_assert(MemoryProtectFlags{ MemoryProtect::ExecuteReadWrite } == PAGE_EXECUTE_READWRITE);
static_assert(MemoryProtectFlags{ MemoryProtect::ExecuteWriteCopy } == PAGE_EXECUTE_WRITECOPY);
static_assert(MemoryProtectFlags{ MemoryProtect::NoAccess } == PAGE_NOACCESS);
static_assert(MemoryProtectFlags{ MemoryProtect::ReadOnly } == PAGE_READONLY);
static_assert(MemoryProtectFlags{ MemoryProtect::ReadWrite } == PAGE_READWRITE);
static_assert(MemoryProtectFlags{ MemoryProtect::TargetsInvalid } == PAGE_TARGETS_INVALID);
static_assert(MemoryProtectFlags{ MemoryProtect::TargetsNoUpdate } == PAGE_TARGETS_NO_UPDATE);
static_assert(MemoryProtectFlags{ MemoryProtect::Guard } == PAGE_GUARD);
static_assert(MemoryProtectFlags{ MemoryProtect::NoCache } == PAGE_NOCACHE);
static_assert(MemoryProtectFlags{ MemoryProtect::WriteCombine } == PAGE_WRITECOMBINE);

static_assert(MemoryTypeFlags{ MemoryType::Image } == MEM_IMAGE);
static_assert(MemoryTypeFlags{ MemoryType::Mapped } == MEM_MAPPED);
static_assert(MemoryTypeFlags{ MemoryType::Private } == MEM_PRIVATE);


}  // namespace gan