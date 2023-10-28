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

#include <DllInjector.h>

#include <intrin.h>
#include <windows.h>

#include <thread>
#include <string_view>


// defined in TestDllInjector.asm
extern "C" void __stdcall Test_DllInjector_NewThreadProc(uint32_t* signal);
extern "C" void __stdcall Test_DllInjector_ExitThreadProc();


using namespace std::literals;


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
			WaitForSignal();

			SuspendThread(m_thread);
			// Set IP to Test_DllInjector_ExitThreadProc which should gracefully and voluntarily exit the thread.
			{
				CONTEXT ctx{ .ContextFlags = CONTEXT_CONTROL };
				GetThreadContext(m_thread, &ctx);
#ifdef _WIN64
				ctx.Rip = reinterpret_cast<size_t>(Test_DllInjector_ExitThreadProc);
#else
				ctx.Eip = reinterpret_cast<size_t>(Test_DllInjector_ExitThreadProc);
#endif  // defined _WIN64
				SetThreadContext(m_thread, &ctx);
			}
			ResumeThread(m_thread);

			for (DWORD exitCode = STILL_ACTIVE; exitCode == STILL_ACTIVE; )
			{
				const auto getCodeResult = GetExitCodeThread(m_thread, &exitCode);
				ASSERT(getCodeResult);
				if (!getCodeResult)
					break;
			}
		}

		void WaitForSignal()
		{
			while (!m_signal)
				;
		}

	DEFINE_TEST_SHARED_END

	DEFINE_TEST_START(Inject)
	{
		constexpr static std::wstring_view k_glu32Dll = L"glu32.dll"sv;

		ASSERT(GetModuleHandleW(k_glu32Dll.data()) == nullptr);

		gan::DllInjectorByContext injector(GetCurrentProcess(), m_thread);
		WaitForSignal();  // Wait for thread initialization

		SuspendThread(m_thread);
		m_signal = 0;  // Reset signal
		injector.Inject(k_glu32Dll);
		ResumeThread(m_thread);

		WaitForSignal();  // Make sure IP has returned to the main loop

		auto hModGlu32 = GetModuleHandleW(k_glu32Dll.data());
		ASSERT(hModGlu32 != nullptr);
		EXPECT(FreeLibrary(hModGlu32) != 0);
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END
