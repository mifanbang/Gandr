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

#include <Hook.h>
#include <PE.h>

#include <cstdint>
#include <string_view>

#include <windows.h>
#include <commctrl.h>
#include <psapi.h>
#include <xinput.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "xinput.lib")


using namespace std::literals;


DEFINE_TESTSUITE_START(Hook_General)

	DEFINE_TEST_SHARED_START

		// Zero() enforces runtime evaluation to prevent compiler from treating Add() as constexpr.
		static size_t Zero() { return reinterpret_cast<size_t>(GetModuleHandleA("ThisModuleMustNotExistOrWeAreScrewed")); }

		__declspec(noinline) static size_t Add(size_t n1, size_t n2) { return Zero() ? 0 : n1 + n2; }
		__declspec(noinline) static size_t Mul(size_t n1, size_t n2) { return Zero() ? 0 : n1 * n2; }

	DEFINE_TEST_SHARED_END


	DEFINE_TEST_START(DoubleInstallation)
	{
		gan::Hook hook_1 { Add, Mul };
		ASSERT(hook_1.Install() == gan::Hook::OpResult::Hooked);

		gan::Hook hook_2 { Add, Mul };
		ASSERT(hook_2.Install() == gan::Hook::OpResult::AddressInUse);

		ASSERT(hook_1.Uninstall() == gan::Hook::OpResult::Unhooked);
	}
	DEFINE_TEST_END

	DEFINE_TEST_START(InstallAndUninstall)
	{
		gan::Hook hook { Add, Mul };

		EXPECT(Add(123, 321) == 444);
		ASSERT(hook.Install() == gan::Hook::OpResult::Hooked);
		EXPECT(Add(123, 321) == 39483);
		ASSERT(hook.Uninstall() == gan::Hook::OpResult::Unhooked);
		EXPECT(Add(123, 321) == 444);
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END


DEFINE_TESTSUITE_START(Hook_Kernel32)

	DEFINE_TEST_SHARED_START

		HMODULE m_hMod { nullptr };

		static FARPROC __stdcall _GetProcAddress(HMODULE hModule, LPCSTR lpProcName)
		{
			if (strcmp(lpProcName, "GetProcAddress") == 0)
				return reinterpret_cast<FARPROC>(_GetProcAddress);
			else
				return gan::Hook::GetTrampoline(GetProcAddress)(hModule, lpProcName);
		}

		DEFINE_TEST_SETUP
		{
			m_hMod = GetModuleHandleA("kernel32");
			return m_hMod != nullptr;
		}

	DEFINE_TEST_SHARED_END


	DEFINE_TEST_START(GetProcAddress)
	{
		gan::Hook hook { GetProcAddress, _GetProcAddress };

		ASSERT(hook.Install() == gan::Hook::OpResult::Hooked);
		EXPECT(GetProcAddress(m_hMod, "GetProcAddress") == reinterpret_cast<FARPROC>(_GetProcAddress));
		ASSERT(hook.Uninstall() == gan::Hook::OpResult::Unhooked);
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END



DEFINE_TESTSUITE_START(Hook_Advapi32)

	DEFINE_TEST_SHARED_START

		static BOOL __stdcall _GetUserNameW(LPWSTR lpBuffer, LPDWORD pcbBuffer)
		{
			const auto result = gan::Hook::GetTrampoline(GetUserNameW)(lpBuffer, pcbBuffer);
			if (result != 0)
				lpBuffer[0] = '?';
			return result;
		}
		
	DEFINE_TEST_SHARED_END


	DEFINE_TEST_START(GetUserNameW)
	{
		wchar_t user[64] { '\0' };
		DWORD length = _countof(user);
		gan::Hook hook { GetUserNameW, _GetUserNameW };

		ASSERT(hook.Install() == gan::Hook::OpResult::Hooked);
		EXPECT(GetUserNameW(user, &length) && user[0] == '?');
		ASSERT(hook.Uninstall() == gan::Hook::OpResult::Unhooked);
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END



DEFINE_TESTSUITE_START(Hook_User32)

	DEFINE_TEST_SHARED_START

		constexpr static wchar_t k_overridenClassName[] = L"button";

		static int __stdcall _GetClassInfoW(HINSTANCE hInstance, LPCWSTR lpClassName, LPWNDCLASSW lpWndClass)
		{
			return gan::Hook::GetTrampoline(GetClassInfoW)(hInstance, k_overridenClassName, lpWndClass);
		}
		
	DEFINE_TEST_SHARED_END

	DEFINE_TEST_START(GetClassInfoW)
	{
		gan::Hook hook { GetClassInfoW, _GetClassInfoW };

		ASSERT(hook.Install() == gan::Hook::OpResult::Hooked);

		WNDCLASSW wndClass;
		ZeroMemory(&wndClass, sizeof(wndClass));
		EXPECT(GetClassInfoW(nullptr, L"static", &wndClass) != FALSE);
		EXPECT(wndClass.lpszClassName != nullptr);
		if (wndClass.lpszClassName != nullptr)
			EXPECT(wcsstr(wndClass.lpszClassName, k_overridenClassName) != nullptr);

		ASSERT(hook.Uninstall() == gan::Hook::OpResult::Unhooked);
	}
	DEFINE_TEST_END;

DEFINE_TESTSUITE_END



DEFINE_TESTSUITE_START(Hook_Gdi32_AllFunctions)

	DEFINE_TEST_SHARED_START

		static int __stdcall Dummy() { return 0; }
		
	DEFINE_TEST_SHARED_END

	DEFINE_TEST_START(AllFunctions)
	{
		MODULEINFO modInfo{ };
		auto hMod = ::LoadLibraryW(L"gdi32.dll");
		ASSERT(hMod);
		ASSERT(::GetModuleInformation(::GetCurrentProcess(), hMod, &modInfo, sizeof(modInfo)));

		const gan::ConstMemAddr modBaseAddr{ modInfo.lpBaseOfDll };
		auto peHeaders = gan::PeImageHelper::GetLoadedHeaders(modBaseAddr);
		ASSERT(peHeaders);
		ASSERT(peHeaders->exportData);

		// Calculate memory range for the code section. This will be used to determine
		// whether the exported symbol is a function.
		const auto indexCodeSection = peHeaders->FindSectionByName(0, u8".text"sv);
		ASSERT(indexCodeSection);
		const auto& codeSection = peHeaders->sectionHeaderList[*indexCodeSection];
		const gan::Range<gan::Rva> codeSectionRange{
			.min = codeSection.VirtualAddress,
			.max = codeSection.VirtualAddress + codeSection.Misc.VirtualSize
		};

		std::vector<gan::Hook> installedHooks;
		for (const auto& exportedFunc : peHeaders->exportData->functions)
		{
			// Skip forwarding and non-code exports
			if (exportedFunc.forwarding
				|| !codeSectionRange.InRange(exportedFunc.rva))
			{
				continue;
			}

			void* procAddr = modBaseAddr.Offset(exportedFunc.rva).ConstCast().Ptr<>();
			gan::Hook hook {
				reinterpret_cast<FARPROC>(procAddr),
				reinterpret_cast<FARPROC>(Dummy)
			};
			const auto installResult = hook.Install();
			EXPECT(installResult == gan::Hook::OpResult::Hooked || installResult == gan::Hook::OpResult::AddressInUse);
			if (installResult == gan::Hook::OpResult::Hooked)
				installedHooks.emplace_back(std::move(hook));
		}

		for (auto& hook : installedHooks)
		{
			const auto uninstallResult = hook.Uninstall();
			EXPECT(uninstallResult == gan::Hook::OpResult::Unhooked);
		}
	}
	DEFINE_TEST_END;

DEFINE_TESTSUITE_END



DEFINE_TESTSUITE_START(Hook_Comctl32)

	DEFINE_TEST_SHARED_START

		constexpr static LANGID k_expectedLang = MAKELANGID(LANG_FAEROESE, SUBLANG_NEUTRAL);

		static LANGID __stdcall _GetMUILanguage()
		{
			return gan::Hook::GetTrampoline(GetMUILanguage)() + 1;
		}

		DEFINE_TEST_SETUP
		{
			InitMUILanguage(k_expectedLang);
			return true;
		}

	DEFINE_TEST_SHARED_END

	DEFINE_TEST_START(GetMUILanguage)
	{
		gan::Hook hook { GetMUILanguage, _GetMUILanguage };

		ASSERT(hook.Install() == gan::Hook::OpResult::Hooked);
		EXPECT(GetMUILanguage() == k_expectedLang + 1);
		ASSERT(hook.Uninstall() == gan::Hook::OpResult::Unhooked);
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END



DEFINE_TESTSUITE_START(Hook_Winsock32)

	DEFINE_TEST_SHARED_START

		static int __stdcall _gethostname(char* name, int namelen)
		{
			const auto result = gan::Hook::GetTrampoline(gethostname)(name, namelen);
			if (result == 0)
				name[0] = '?';
			return result;
		}

		DEFINE_TEST_SETUP
		{
			WSADATA wsaData;
			return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
		}

	DEFINE_TEST_SHARED_END


	DEFINE_TEST_START(gethostname)
	{
		char name[128] { '\0' };
		gan::Hook hook { gethostname, _gethostname };

		ASSERT(hook.Install() == gan::Hook::OpResult::Hooked);
		EXPECT(gethostname(name, _countof(name)) == 0);
		EXPECT(name[0] == '?');
		ASSERT(hook.Uninstall() == gan::Hook::OpResult::Unhooked);
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END



DEFINE_TESTSUITE_START(Hook_XInput)

	DEFINE_TEST_SHARED_START

		constexpr static BYTE k_expectedTrigger = 0x87;

		static DWORD __stdcall _XInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState) noexcept
		{
			gan::Hook::GetTrampoline(XInputGetState)(dwUserIndex, pState);
			pState->Gamepad.bLeftTrigger = k_expectedTrigger;
			return NO_ERROR;
		}
		
	DEFINE_TEST_SHARED_END


	DEFINE_TEST_START(XInputGetState)
	{
		XINPUT_STATE state;
		gan::Hook hook { XInputGetState, _XInputGetState };
		ZeroMemory(&state, sizeof(state));

		ASSERT(hook.Install() == gan::Hook::OpResult::Hooked);
		EXPECT(XInputGetState(3, &state) == 0);
		EXPECT(state.Gamepad.bLeftTrigger == k_expectedTrigger);
		ASSERT(hook.Uninstall() == gan::Hook::OpResult::Unhooked);
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END

