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

#include <ProcessList.h>

#include <algorithm>

#include <shlwapi.h>


DEFINE_TESTSUITE_START(ProcessList)

	DEFINE_TEST_START(FindSelfInProcList)
	{
		const auto procId = GetCurrentProcessId();

		const auto funcMatchSelf = [procId](const auto& procInfo) {
			return procInfo.pid == procId;
		};

		auto procList = gan::ProcessEnumerator{}();
		ASSERT(procList);
		EXPECT(std::ranges::any_of(procList.value(), funcMatchSelf));
	}
	DEFINE_TEST_END


	DEFINE_TEST_START(FindSelfInThreadList)
	{
		const auto procId = GetCurrentProcessId();
		const auto threadId = GetCurrentThreadId();

		const auto funcMatchSelf = [procId, threadId](const auto& threadInfo) {
			return threadInfo.pidParent == procId && threadInfo.tid == threadId;
		};

		// Process-wide enumeration
		{
			auto threadListCurrProc = gan::ThreadEnumerator{}(procId);
			ASSERT(threadListCurrProc);
			EXPECT(std::ranges::any_of(threadListCurrProc.value(), funcMatchSelf));
		}

		// System-wide enumeration
		{
			auto threadListSystem = gan::ThreadEnumerator{}();
			ASSERT(threadListSystem);
			EXPECT(std::ranges::any_of(threadListSystem.value(), funcMatchSelf));
		}
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END
