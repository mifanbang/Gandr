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


}  // unnamed namespace


DEFINE_TESTSUITE_START(AutoWinHandle)

	DEFINE_TEST_START(AutoClose)
	{
		const auto event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
		ASSERT(event != nullptr);

		{
			gan::AutoWinHandle handle(event);
		}

		ASSERT(!IsHandleValidToSystem(event));
	}
	DEFINE_TEST_END

	DEFINE_TEST_START(MoveConstruct)
	{
		const auto event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
		ASSERT(event != nullptr);

		{
			gan::AutoWinHandle handle1(event);
			ASSERT(handle1.IsValid());

			gan::AutoWinHandle handle2(std::move(handle1));
			EXPECT(!handle1.IsValid());
			EXPECT(handle2.IsValid());
			EXPECT(handle2 == event);
			EXPECT(IsHandleValidToSystem(handle2));
		}

		EXPECT(!IsHandleValidToSystem(event));
	}
	DEFINE_TEST_END

	DEFINE_TEST_START(MoveAssignment)
	{
		const auto event1 = CreateEventW(nullptr, FALSE, FALSE, nullptr);
		const auto event2 = CreateEventW(nullptr, FALSE, FALSE, nullptr);
		ASSERT(event1 != nullptr);
		ASSERT(event2 != nullptr);

		{
			gan::AutoWinHandle handle1(event1);
			ASSERT(handle1.IsValid());

			gan::AutoWinHandle handle2(event2);
			ASSERT(handle2.IsValid());

			handle2 = std::move(handle1);
			EXPECT(!handle1.IsValid());
			EXPECT(handle2.IsValid());
			EXPECT(handle2 == event1);
			EXPECT(IsHandleValidToSystem(handle2));
			EXPECT(!IsHandleValidToSystem(event2));  // The original event of handle2 should be closed by now
		}

		EXPECT(!IsHandleValidToSystem(event1));
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END
