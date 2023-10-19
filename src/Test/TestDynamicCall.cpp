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

#include <DynamicCall.h>

#include <cstdint>

#include <windows.h>



DEFINE_TESTSUITE_START(DynamicCall)

	DEFINE_TEST_START(LoadDllAndFunctions)
	{
		constexpr wchar_t k_glu32[] = L"glu32.dll";  // A DLL not in the Import Address Table of Test.exe

		ASSERT(GetModuleHandleW(k_glu32) == nullptr);

		gan::DynamicCall<const uint8_t* __stdcall (uint32_t)> funcGluGetString(k_glu32, "gluGetString");
		EXPECT(funcGluGetString.IsValid());

		gan::DynamicCall<void ()> funcNotExist(k_glu32, "gluNotAnyOfYourFunctions");
		EXPECT(!funcNotExist.IsValid());

		const auto hModGlu32 = GetModuleHandleW(k_glu32);
		ASSERT(hModGlu32 != nullptr);
		ASSERT(FreeLibrary(hModGlu32) != FALSE);
	}
	DEFINE_TEST_END

	DEFINE_TEST_START(CallFunctions)
	{
		constexpr wchar_t k_kernel32[] = L"kernel32.dll";

		// void return type
		gan::DynamicCall<void __stdcall (const wchar_t*)> funcOutputDebugStringW(k_kernel32, "OutputDebugStringW");
		ASSERT(funcOutputDebugStringW.IsValid());
		funcOutputDebugStringW(L"Hi there debugger\n");

		// non-void return type
		gan::DynamicCall<HANDLE __stdcall ()> funcGetCurrentProcess(k_kernel32, "GetCurrentProcess");
		ASSERT(funcGetCurrentProcess.IsValid());
		ASSERT(funcGetCurrentProcess() == HANDLE(-1));  // GetCurrentProcess() returns a pseudo handle which has the value -1
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END

