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

#include <DllInjector.h>

#include <thread>

#include <intrin.h>
#include <windows.h>



// defined in TestDllInjector.asm
extern "C" void __stdcall Test_DllInjector_NewThreadProc(uint32_t* signal);



DEFINE_TESTSUITE_START(DllInjectorByContext)

	DEFINE_TEST_SHARED_START

		HANDLE m_thread { nullptr };
		volatile uint32_t m_signal { 0 };

		DEFINE_TEST_SETUP
		{
			constexpr size_t k_useDefaultStackSize = 0;
			constexpr uint32_t k_runImmediately = 0;
			m_thread = CreateThread(
				nullptr,
				k_useDefaultStackSize,
				reinterpret_cast<LPTHREAD_START_ROUTINE>(Test_DllInjector_NewThreadProc),
				const_cast<uint32_t*>(&m_signal),
				k_runImmediately,
				nullptr
			);
			return m_thread != nullptr;
		}

		DEFINE_TEST_TEARDOWN
		{
			const uint32_t k_exitCodeSuccess = 0;
			TerminateThread(m_thread, k_exitCodeSuccess);
		}

		void WaitForSignal()
		{
			while (!m_signal)
				;
		}

	DEFINE_TEST_SHARED_END

	DEFINE_TEST_START(Inject)
	{
		ASSERT(GetModuleHandleW(L"glu32.dll") == nullptr);

		gan::DllInjectorByContext injector(GetCurrentProcess(), m_thread);
		WaitForSignal();  // Wait for thread initialization

		SuspendThread(m_thread);
		m_signal = 0;  // Reset signal
		injector.Inject(L"glu32.dll");
		ResumeThread(m_thread);

		WaitForSignal();  // Make sure IP has returned to the main loop

		auto hModGlu32 = GetModuleHandleW(L"glu32.dll");
		ASSERT(hModGlu32 != nullptr);
		EXPECT(FreeLibrary(hModGlu32) != 0);
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END

