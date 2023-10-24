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

#include <Debugger.h>
#include <DllPreloadDebugSession.h>
#include <Handle.h>
#include <ModuleList.h>

#include <algorithm>
#include <string>

#include <windows.h>
#include <shlwapi.h>


// This test suite tests classes Debugger, DebugSession, and DllPreloadDebugSession
DEFINE_TESTSUITE_START(Debugger)

	// Create a new process "notepad.exe" and have it load "calc.exe" into memory
	DEFINE_TEST_START(DllPreloadDebugSession)
	{
		const static wchar_t* k_loadedModule = L"calc.exe";

		wchar_t systemPath[MAX_PATH];
		GetSystemDirectory(systemPath, _countof(systemPath));
		const std::wstring imagePath = std::wstring(systemPath) + L"\\notepad.exe";

		// For later use
		gan::DebugSession::Identifier pid;
		gan::AutoWinHandle process{ };

		{
			gan::DebugSession::CreateProcessParam param { };
			param.imagePath = imagePath.c_str();

			constexpr gan::DllPreloadDebugSession::Option k_option = gan::DllPreloadDebugSession::Option::EndSessionSync;

			gan::Debugger debugger;
			auto sessionWPtr = debugger.AddSession<gan::DllPreloadDebugSession>(param, k_loadedModule, k_option);
			auto session = sessionWPtr.lock();
			ASSERT(session);

			pid = session->GetId();
			process = gan::HandleHelper::Duplicate(session->GetHandle());

			EXPECT(debugger.EnterEventLoop() == gan::Debugger::EventLoopResult::AllDetached);
		}

		// Iterate through loaded module paths
		gan::ModuleList moduleList;
		EXPECT(gan::ModuleEnumerator::Enumerate(pid, moduleList) == gan::ModuleEnumerator::Result::Success);
		const bool loadedInTargetProcess = std::any_of(
			moduleList.begin(),
			moduleList.end(),
			[](gan::ModuleInfo& modInfo) { return StrStrIW(modInfo.imageName.c_str(), k_loadedModule) != nullptr; }
		);
		EXPECT(loadedInTargetProcess);

		EXPECT(TerminateProcess(*process, 0) != FALSE);
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END
