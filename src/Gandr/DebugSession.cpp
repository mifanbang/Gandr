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

#include <DebugSession.h>

#include <Buffer.h>
#include <Types.h>

#include <cstdio>
#include <format>
#include <memory>
#include <string>


namespace gan
{


// ---------------------------------------------------------------------------
// class DebugSession
// ---------------------------------------------------------------------------

DebugSession::DebugSession(const CreateProcessParam& newProcParam)
	: m_pid(0)
	, m_hProc(INVALID_HANDLE_VALUE)
{
	// Paramater lpCommandLine to CreateProcessW has a max length of 32,767 characters.
	// Allocate on the heap since stack is too small.
	// REF: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw
	constexpr size_t k_maxArgLength = 32767;
	auto cmdlineBuffer = Buffer::Allocate(k_maxArgLength * sizeof(wchar_t));
	if (!cmdlineBuffer)
		return;

	// Prepare command line buffer if needed
	auto const cmdLinePtr = newProcParam.args ?
		reinterpret_cast<wchar_t*>(cmdlineBuffer->GetData()) :
		nullptr;
	if (cmdLinePtr)
	{
		if constexpr (UseStdFormat())
			std::format_to_n(
				cmdLinePtr,
				k_maxArgLength,
				L"\"{}\" {}",
				newProcParam.imagePath,
				newProcParam.args
			);
		else
			::swprintf(
				cmdLinePtr,
				k_maxArgLength,
				L"\"%s\" %s",
				newProcParam.imagePath,
				newProcParam.args
			);
	}

	STARTUPINFO si = [](const auto* startInfo) {
		// MSVC still doesn't seem to apply copy elision on ternary conditional operator,
		// so the plain if-else is used.
		if (startInfo)
			return STARTUPINFO{ *startInfo };
		else
			return STARTUPINFO{ };
	} (newProcParam.startUpInfo);

	constexpr auto k_notInheritable = nullptr;
	constexpr BOOL k_dontLetChildInherit = FALSE;
	constexpr auto k_useParentsEnv = nullptr;
	PROCESS_INFORMATION procInfo;
	if (::CreateProcessW(
		newProcParam.imagePath,
		cmdLinePtr,
		k_notInheritable,
		k_notInheritable,
		k_dontLetChildInherit,
		DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS,
		k_useParentsEnv,
		newProcParam.currentDir,
		&si,
		&procInfo))
	{
		m_pid = procInfo.dwProcessId;
		m_hProc = procInfo.hProcess;
		::CloseHandle(procInfo.hThread);
	}
}


DebugSession::~DebugSession()
{
	End(EndOption::Kill);
}


void DebugSession::End(EndOption option) noexcept
{
	if (IsValid())
	{
		::DebugActiveProcessStop(m_pid);

		if (option == EndOption::Kill)
			::TerminateProcess(m_hProc, 0);

		m_pid = 0;
		::CloseHandle(m_hProc);
	}
}


}  // namespace gan
