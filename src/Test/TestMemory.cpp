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

#include "Test.h"

#include <Memory.h>
#include <Types.h>

#include <algorithm>

#include <windows.h>


DEFINE_TESTSUITE_START(Memory)

	DEFINE_TEST_START(EnumMemory)
	{
		const auto currProcId = GetCurrentProcessId();

		auto memoryRegionList1 = gan::MemoryRegionEnumerator{}(currProcId);
		ASSERT(memoryRegionList1);
		EXPECT(!memoryRegionList1->empty());

		gan::ConstMemAddr maxAddr{ reinterpret_cast<const void*>(-1) };

		// Custom memory range
		{
			constexpr gan::MemoryStateFlags k_state{ gan::MemoryState::Reserve };
			constexpr gan::MemoryProtectFlags k_protect{ gan::MemoryProtect::ReadWrite };
			gan::MemAddr newPage{ VirtualAlloc(nullptr, 0x1'0000, k_state, k_protect) };
			EXPECT(newPage);

			auto memoryRegionList2 = gan::MemoryRegionEnumerator{}(currProcId, { gan::ConstMemAddr{ }, maxAddr });
			ASSERT(memoryRegionList2);
			const auto lookForNewPage = [newPage](const auto& region) {
				return region.allocBase == newPage
					&& region.allocBase == region.base
					&& region.state == k_state;
			};
			EXPECT(std::ranges::none_of(*memoryRegionList1, lookForNewPage));
			EXPECT(std::ranges::any_of(*memoryRegionList2, lookForNewPage));
		}

		// Custom memory range (0x0000'0000 - 0x0000'0001)
		{
			auto memoryRegionList3 = gan::MemoryRegionEnumerator{}(currProcId, { gan::ConstMemAddr{ nullptr }, gan::ConstMemAddr{ nullptr }.Offset(1) });
			ASSERT(memoryRegionList3);
			EXPECT(memoryRegionList3->empty());
		}

		// Invalid memory address range (min > max)
		{
			auto memoryRegionList4 = gan::MemoryRegionEnumerator{}(currProcId, { maxAddr, gan::ConstMemAddr{ } });
			ASSERT(!memoryRegionList4);
			EXPECT(memoryRegionList4.error() == gan::MemoryRegionEnumerator::Error::InvalidAddressRange);
		}
	}
	DEFINE_TEST_END

	DEFINE_TEST_START(EnumMemoryInvalidProcess)
	{
		auto memoryRegionList1 = gan::MemoryRegionEnumerator{}(nullptr);
		EXPECT(!memoryRegionList1);

		auto memoryRegionList2 = gan::MemoryRegionEnumerator{}(1);
		EXPECT(!memoryRegionList2);
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END
