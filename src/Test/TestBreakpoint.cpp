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

#pragma once

#include "Test.h"

#include <Breakpoint.h>

#include <windows.h>



namespace
{

__declspec(thread) bool t_isBpHit;

}  // unnamed namespace



DEFINE_TESTSUITE_START(Breakpoint)

	DEFINE_TEST_SHARED_START

		constexpr static DWORD k_indexIsBpHit = TLS_MINIMUM_AVAILABLE;
		constexpr static DWORD k_indexIsBpCleared = TLS_MINIMUM_AVAILABLE - 1;

		PVOID m_handle { nullptr };

		static LONG __stdcall MyVectoredExceptionHandler(_EXCEPTION_POINTERS* ExceptionInfo)
		{
			t_isBpHit = true;
			ExceptionInfo->ContextRecord->Dr7 = 0;  // Remove all BP's.
			return EXCEPTION_CONTINUE_EXECUTION;
		}

		DEFINE_TEST_SETUP
		{
			m_handle = AddVectoredExceptionHandler(TRUE, MyVectoredExceptionHandler);
			return m_handle != nullptr;
		}

		DEFINE_TEST_TEARDOWN
		{
			RemoveVectoredExceptionHandler(m_handle);
		}

	DEFINE_TEST_SHARED_END


	DEFINE_TEST_START(InstallBP)
	{
		ASSERT(gan::HWBreakpoint::Enable(GetCurrentThread(), gan::ConstMemAddr{ GetCurrentProcess }, gan::HWBreakpointSlot::DR3));
		t_isBpHit = false;
		GetCurrentProcess();
		EXPECT(t_isBpHit);
		ASSERT(gan::HWBreakpoint::Disable(GetCurrentThread(), gan::HWBreakpointSlot::DR3));
	}
	DEFINE_TEST_END

	DEFINE_TEST_START(UninstallBP)
	{
		ASSERT(gan::HWBreakpoint::Enable(GetCurrentThread(), gan::ConstMemAddr{ GetCurrentProcess }, gan::HWBreakpointSlot::DR3));
		ASSERT(gan::HWBreakpoint::Disable(GetCurrentThread(), gan::HWBreakpointSlot::DR3));
		t_isBpHit = false;
		GetCurrentProcess();
		EXPECT(!t_isBpHit);
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END


