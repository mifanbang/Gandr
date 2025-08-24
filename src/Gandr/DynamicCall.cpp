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

#include <DynamicCall.h>

#include <Mutex.h>
#include <Types.h>

#include <windows.h>

#include <algorithm>
#include <cassert>
#include <vector>


namespace
{


class LibraryManager : public gan::Singleton<LibraryManager>
{
	friend class gan::Singleton<LibraryManager>;

public:
	HMODULE Get(std::wstring_view lib)
	{
		if (auto hModule = ::GetModuleHandleW(lib.data()))
			return hModule;

		if (auto hModule = ::LoadLibraryW(lib.data()))
		{
			// FreeLibrary on destructor
			m_libUnloadList.ApplyOperation(
				[hModule](auto& libs) { return libs.emplace_back(hModule); }
			);
			return hModule;
		}
		return nullptr;
	}

private:
	LibraryManager() = default;

	~LibraryManager()
	{
		m_libUnloadList.ApplyOperation( [](auto& libs) noexcept {
			for (auto item : libs)
				::FreeLibrary(item);
		} );
	}

	LibraryManager(const LibraryManager&) = delete;
	LibraryManager(LibraryManager&&) = delete;
	LibraryManager& operator=(const LibraryManager&) = delete;
	LibraryManager& operator=(LibraryManager&&) = delete;

	gan::ThreadSafeResource<std::vector<HMODULE>> m_libUnloadList;
};


}  // unnamed namespace


namespace gan
{


void* DynamicCall::LoadLibAndGetProc(std::wstring_view lib, std::string_view func)
{
	assert(lib.data());
	assert(func.data());

	if (auto hModule = LibraryManager::GetInstance().Get(lib))
		return ::GetProcAddress(hModule, func.data());
	return nullptr;
}


}  // namespace gan
