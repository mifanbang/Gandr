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

	DEFINE_TEST_START(MemoryRegionEnumerator)
	{
		const auto currProcId = GetCurrentProcessId();

		auto memoryResionList1 = gan::MemoryRegionEnumerator::Enumerate(currProcId);
		EXPECT(!memoryResionList1.empty());

		gan::ConstMemAddr maxAddr{ reinterpret_cast<const void*>(-1) };
		auto memoryResionList2 = gan::MemoryRegionEnumerator::Enumerate(currProcId, { gan::ConstMemAddr{ }, maxAddr } );
		EXPECT(!memoryResionList2.empty());
		EXPECT(memoryResionList1 == memoryResionList2);

		auto memoryResionList3 = gan::MemoryRegionEnumerator::Enumerate(currProcId, { maxAddr, gan::ConstMemAddr{ } });
		EXPECT(memoryResionList3.empty());
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END
