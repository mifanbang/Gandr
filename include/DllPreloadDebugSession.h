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

#include <DebugSession.h>

#include <string>
#include <string_view>


namespace gan
{


// ---------------------------------------------------------------------------
// class DllPreloadDebugSession - A DebugSession implementation that preloads a DLL at entry point
// ---------------------------------------------------------------------------

class DllPreloadDebugSession : public DebugSession
{
public:
	enum class Option : uint8_t
	{
		EndSessionSync,  // Automatically ends the session when OnDllLoaded() detects loaded module
		EndSessionAsync,  // Automatically ends the session when target process starts calling LoadLibraryW()
		KeepAlive  // Keep the session alive
	};

	DllPreloadDebugSession(const CreateProcessParam& newProcParam, std::wstring_view payloadPath, Option option);

private:
	virtual ContinueStatus OnProcessCreated(const CREATE_PROCESS_DEBUG_INFO& procInfo) override;
	virtual ContinueStatus OnExceptionTriggered(const EXCEPTION_DEBUG_INFO& exceptionInfo) override;
	virtual ContinueStatus OnDllLoaded(const LOAD_DLL_DEBUG_INFO& dllInfo) override;

	WinHandle m_hMainThread;
	std::wstring m_payloadPath;
	Option m_option;
};


}  // namespace gan
