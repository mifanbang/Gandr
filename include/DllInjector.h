/*
 *  Gandr - another minimalism library for hacking x86-based Windows
 *  Copyright (C) 2020-2025 Mifan Bang <https://debug.tw>.
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

#include <Handle.h>

#include <string>
#include <string_view>


namespace gan
{


// ---------------------------------------------------------------------------
// Class DLLInjectorByContext - DLL injection by setting context of a thread
// ---------------------------------------------------------------------------

class DllInjectorByContext
{
public:
	enum class Result : uint8_t
	{
		Succeeded,

		GetContextFailed,
		DLLPathNotWritten,
		StackFrameNotWritten,
		SetContextFailed
	};


	DllInjectorByContext(WinHandle hProcess, WinHandle hThread);

	// m_hProcess and m_hThread will be closed in destructor
	DllInjectorByContext(const DllInjectorByContext&) = delete;
	DllInjectorByContext& operator=(const DllInjectorByContext&) = delete;

	Result Inject(std::wstring_view dllPath);


private:
	AutoWinHandle m_hProcess;
	AutoWinHandle m_hThread;
	std::wstring m_dllPath;
};



}  // namespace gan
