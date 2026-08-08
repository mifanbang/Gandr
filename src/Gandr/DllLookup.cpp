/*
 *  Gandr - another minimalism library for hacking x86-based Windows
 *  Copyright (C) 2020-2026 Mifan Bang <https://debug.tw>.
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

#include <DllLookup.h>

#include <Handle.h>
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
			m_libUnloadList.Do( [hModule](auto* libs) {
				libs->emplace_back(hModule);
			} );
			return hModule;
		}
		return nullptr;
	}

private:
	gan::ThreadSafeResource<std::vector<gan::AutoWinModule>> m_libUnloadList;
};

}  // unnamed namespace


namespace gan
{

void* DllLookup::LoadLibAndGetSymbol(std::wstring_view lib, std::string_view name)
{
	assert(lib.data());
	assert(name.data());

	if (auto hModule = LibraryManager::GetInstance().Get(lib))
		return FromAnyFn(::GetProcAddress(hModule, name.data()));
	return nullptr;
}

}  // namespace gan
