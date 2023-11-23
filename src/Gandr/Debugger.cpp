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

#include <Debugger.h>

#include <windows.h>

#include <cassert>


namespace
{


gan::DebugSession::ContinueStatus CallDebugEventHandler(gan::DebugSession& session, const DEBUG_EVENT& event)
{
	switch (event.dwDebugEventCode)
	{
		case EXCEPTION_DEBUG_EVENT:
		{
			return session.OnExceptionTriggered(event.u.Exception);
		}
		case CREATE_THREAD_DEBUG_EVENT:
		{
			return session.OnThreadCreated(event.u.CreateThread);
		}
		case CREATE_PROCESS_DEBUG_EVENT:
		{
			const auto result = session.OnProcessCreated(event.u.CreateProcessInfo);
			::CloseHandle(event.u.CreateProcessInfo.hFile);
			return result;
		}
		case EXIT_THREAD_DEBUG_EVENT:
		{
			return session.OnThreadExited(event.u.ExitThread);
		}
		case EXIT_PROCESS_DEBUG_EVENT:
		{
			return session.OnProcessExited(event.u.ExitProcess);
		}
		case LOAD_DLL_DEBUG_EVENT:
		{
			const auto result = session.OnDllLoaded(event.u.LoadDll);
			::CloseHandle(event.u.LoadDll.hFile);
			return result;
		}
		case UNLOAD_DLL_DEBUG_EVENT:
		{
			return session.OnDllUnloaded(event.u.UnloadDll);
		}
		case OUTPUT_DEBUG_STRING_EVENT:
		{
			return session.OnStringOutput(event.u.DebugString);
		}
		case RIP_EVENT:
		{
			return session.OnRipEvent(event.u.RipInfo);
		}
		default:
		{
			return gan::DebugSession::ContinueStatus::ContinueThread;  // continue by default
		}
	}
}


}  // unnamed namespace


namespace gan
{


// ---------------------------------------------------------------------------
// class Debugger
// ---------------------------------------------------------------------------

Debugger::Debugger() noexcept
	: m_flagEventLoopExit(false)
{
}

Debugger::~Debugger()
{
	RemoveAllSessions(DebugSession::EndOption::Kill);
}

Debugger::EventLoopResult Debugger::EnterEventLoop()
{
	m_flagEventLoopExit = false;
	while (!m_flagEventLoopExit)
	{
		if (m_sessions.size() == 0)
			return EventLoopResult::AllDetached;

		DEBUG_EVENT dbgEvent{ };
		if (::WaitForDebugEvent(&dbgEvent, INFINITE) == 0)
			return EventLoopResult::ErrorOccurred;

		const auto itr = m_sessions.find(dbgEvent.dwProcessId);
		if (itr == m_sessions.end())
			continue;  // this shouldn't happen though
		auto pSession = itr->second;

		pSession->OnPreEvent({
			.eventCode = dbgEvent.dwDebugEventCode,
			.threadId = dbgEvent.dwThreadId
		});

		const auto contStatus = CallDebugEventHandler(*pSession, dbgEvent);

		::ContinueDebugEvent(
			dbgEvent.dwProcessId,
			dbgEvent.dwThreadId,
			contStatus == DebugSession::ContinueStatus::NotHandled ? DBG_EXCEPTION_NOT_HANDLED : DBG_CONTINUE
		);

		if (contStatus == DebugSession::ContinueStatus::CloseSession)
			RemoveSession(dbgEvent.dwProcessId, DebugSession::EndOption::Detach);
	}

	return EventLoopResult::ExitRequested;
}

bool Debugger::AddSessionInstance(const std::shared_ptr<DebugSession>& pSession)
{
	assert(pSession);
	if (!pSession || !pSession->IsValid())
		return false;

	return m_sessions
		.try_emplace(pSession->GetId(), pSession)
		.second;
}

bool Debugger::RemoveSession(DebugSession::Identifier sessionId, DebugSession::EndOption option)
{
	const auto itr = m_sessions.find(sessionId);
	if (itr == m_sessions.end())
		return false;

	itr->second->End(option);
	m_sessions.erase(itr);
	return true;
}

void Debugger::RemoveAllSessions(DebugSession::EndOption option) noexcept
{
	for (auto& itr : m_sessions)
		itr.second->End(option);
	m_sessions.clear();
}

void Debugger::GetSessionList(IdList& output) const
{
	output.clear();
	for (auto& itr : m_sessions)
		output.push_back(itr.first);
}


}  // namespace gan
