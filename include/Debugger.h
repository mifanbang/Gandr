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

#include <memory>
#include <unordered_map>
#include <string>



namespace gan
{



// ---------------------------------------------------------------------------
// Class Debugger - Managing DebugSession objects and rounting events to them
// ---------------------------------------------------------------------------

class Debugger
{
public:
	enum class EventLoopResult : uint8_t
	{
		AllDetached,
		ExitRequested,  // Returning due to SetMainLoopExitFlag() being called
		ErrorOccurred
	};

	using IdList = std::vector<DebugSession::Identifier>;


	Debugger();
	~Debugger();

	Debugger(const Debugger&) = delete;
	Debugger& operator = (const Debugger&) = delete;

	EventLoopResult EnterEventLoop();

	template <typename T, typename... Arg>
	std::weak_ptr<T> AddSession(Arg&&... arg)
	{
		std::shared_ptr<DebugSession> pSession = std::make_shared<T>(std::forward<Arg>(arg)...);
		return AddSessionInstance(pSession) ? std::static_pointer_cast<T>(pSession) : nullptr;
	}

	bool RemoveSession(DebugSession::Identifier sessionId, DebugSession::EndOption option);
	void RemoveAllSessions(DebugSession::EndOption option);
	void GetSessionList(IdList& output) const;


private:
	void RequestEventLoopExit()
	{
		m_flagEventLoopExit = true;
	}

	bool AddSessionInstance(const std::shared_ptr<DebugSession>& pSession);


	using SessionMap = std::unordered_map<DebugSession::Identifier, std::shared_ptr<DebugSession>>;
	SessionMap m_sessions;

	bool m_flagEventLoopExit;  // flag for main loop
};



}  // namespace gan
