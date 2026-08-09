/*
 *  Gandr - another minimalism library for hacking x86-based Windows
 *  Copyright (C) 2020-2026 Mifan Bang <https://debug.tw>.
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

#include <Handle.h>

#include <windows.h>


namespace
{


bool IsHandleValidToSystem(HANDLE handle)
{
	// For some reasons VS2019 will mess up debug registers if the assertion below uses
	// CloseHandle(), probably due to NtClose() throwing an exception upon invalid handles.
	// GetHandleInformation() should work fine for the purpose of checking handle validity.
	DWORD dummy;
	return GetHandleInformation(handle, &dummy) != FALSE;  // Returning FALSE means the handle is invalid.
}


HANDLE CreateNamelessEvent()
{
	constexpr LPSECURITY_ATTRIBUTES k_noAttr{ nullptr };
	constexpr BOOL k_autoReset{ FALSE };
	constexpr BOOL k_initiallyUnset{ FALSE };
	constexpr wchar_t* k_namelessEvent{ nullptr };

	return CreateEventW(k_noAttr, k_autoReset, k_initiallyUnset, k_namelessEvent);
}


}  // unnamed namespace


DEFINE_TESTSUITE_START(AutoWinHandle)

	DEFINE_TEST_START(AutoClose)
	{
		const auto event = CreateNamelessEvent();
		ASSERT(event != nullptr);
		{
			gan::AutoWinHandle handle(event);
			EXPECT(handle);
		}
		ASSERT(!IsHandleValidToSystem(event));
	}
	DEFINE_TEST_END

	DEFINE_TEST_START(MoveConstruct)
	{
		const auto event = CreateNamelessEvent();
		ASSERT(event != nullptr);
		{
			gan::AutoWinHandle handle1(event);
			ASSERT(handle1);

			gan::AutoWinHandle handle2(std::move(handle1));
#pragma warning(suppress : 26800)  // Suppressing "Use of a moved from object", as checking its content is the intention
			EXPECT(!handle1);
			EXPECT(handle2);
			EXPECT(handle2 == event);
			EXPECT(IsHandleValidToSystem(*handle2));
		}
		EXPECT(!IsHandleValidToSystem(event));
	}
	DEFINE_TEST_END

	DEFINE_TEST_START(MoveAssignment)
	{
		const auto event1 = CreateNamelessEvent();
		ASSERT(event1 != nullptr);
		const auto event2 = CreateNamelessEvent();
		if (event2 == nullptr)
			CloseHandle(event1);  // Avoid leaking
		ASSERT(event2 != nullptr);
		{
			gan::AutoWinHandle handle1(event1);
			gan::AutoWinHandle handle2(event2);
			ASSERT(handle1);
			ASSERT(handle2);

			handle2 = std::move(handle1);
#pragma warning(suppress : 26800)  // Suppressing "Use of a moved from object", as checking its content is the intention
			EXPECT(!handle1);
			EXPECT(handle2);
			EXPECT(handle2 == event1);
			EXPECT(IsHandleValidToSystem(*handle2));
			EXPECT(!IsHandleValidToSystem(event2));  // The original event of handle2 should be closed by now
		}
		EXPECT(!IsHandleValidToSystem(event1));
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END
