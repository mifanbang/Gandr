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

#include "Test.h"

#include <ProcessList.h>

#include <algorithm>

#include <shlwapi.h>


DEFINE_TESTSUITE_START(ProcessList)

	DEFINE_TEST_START(FindSelfInProcList)
	{
		gan::ProcessList procList;
		ASSERT(gan::ProcessEnumerator::Enumerate(procList) == gan::ProcessEnumerator::Result::Success);

		const auto funcMatchSelf = [](const auto& procInfo) {
			return StrStrIW(procInfo.imageName.c_str(), L"Test.exe") != nullptr;
		};
		ASSERT(std::find_if(procList.begin(), procList.end(), funcMatchSelf) != procList.end());
	}
	DEFINE_TEST_END


	DEFINE_TEST_START(FindSelfInThreadList)
	{
		const auto procId = GetCurrentProcessId();
		const auto threadId = GetCurrentThreadId();

		gan::ThreadList threadList;
		ASSERT(gan::ThreadEnumerator::Enumerate(procId, threadList) == gan::ThreadEnumerator::Result::Success);

		const auto funcMatchSelf = [threadId](const auto& threadInfo) {
			return threadInfo.tid == threadId;
		};
		ASSERT(std::find_if(threadList.begin(), threadList.end(), funcMatchSelf) != threadList.end());
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END
