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

#include <DllPreloadDebugSession.h>

#include <Breakpoint.h>
#include <DllInjector.h>
#include <Handle.h>

#include <windows.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")


namespace gan
{


DllPreloadDebugSession::DllPreloadDebugSession(const CreateProcessParam& newProcParam, const wchar_t* pPayloadPath, Option option)
	: DebugSession(newProcParam)
	, m_hMainThread(INVALID_HANDLE_VALUE)
	, m_payloadPath(pPayloadPath)
	, m_option(option)
{
}


DebugSession::ContinueStatus DllPreloadDebugSession::OnProcessCreated(const CREATE_PROCESS_DEBUG_INFO& procInfo)
{
	m_hMainThread = procInfo.hThread;

	// Install a hardware breakpoint at entry point
	HWBreakpoint::Enable(m_hMainThread, ConstMemAddr{ procInfo.lpStartAddress }, HWBreakpointSlot::DR0);

	return ContinueStatus::ContinueThread;
}


DebugSession::ContinueStatus DllPreloadDebugSession::OnExceptionTriggered(const EXCEPTION_DEBUG_INFO& exceptionInfo)
{
	switch (exceptionInfo.ExceptionRecord.ExceptionCode)
	{
		case EXCEPTION_SINGLE_STEP:  // Hardware breakpoint triggered
		{
			// Uninstall the hardware breakpoint at entry point
			HWBreakpoint::Disable(m_hMainThread, HWBreakpointSlot::DR0);

			DllInjectorByContext injector(GetHandle(), m_hMainThread);
			injector.Inject(m_payloadPath.c_str());

			return m_option == Option::EndSessionAsync ?
				ContinueStatus::CloseSession :
				ContinueStatus::ContinueThread;
		}

		case EXCEPTION_BREAKPOINT:  // Expecting this breakpoint triggered by Windows Debug API upon attaching to the process
		{
			// Do nothing
			break;
		}

		default:
		{
			return ContinueStatus::NotHandled;  // Forward if exception is other than a breakpoint
		}
	}

	return ContinueStatus::ContinueThread;
}


DebugSession::ContinueStatus DllPreloadDebugSession::OnDllLoaded(const LOAD_DLL_DEBUG_INFO& dllInfo)
{
	if (m_option == Option::EndSessionSync)
	{
		wchar_t dllPath[MAX_PATH];
		const uint32_t getPathResult = ::GetFinalPathNameByHandleW(dllInfo.hFile, dllPath, _countof(dllPath), FILE_NAME_NORMALIZED);
		if (getPathResult <= _countof(dllPath)
			&& ::StrStrIW(dllPath, m_payloadPath.c_str()) != nullptr)
		{
			return ContinueStatus::CloseSession;
		}
	}

	return ContinueStatus::ContinueThread;
}


}  // namespace gan
