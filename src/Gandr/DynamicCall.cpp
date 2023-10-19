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

#include <DynamicCall.h>

#include <Mutex.h>
#include <Types.h>

#include <algorithm>
#include <memory>
#include <vector>

#include <windows.h>



namespace
{



class LibraryManager : public gan::Singleton<LibraryManager>
{
	friend class gan::Singleton<LibraryManager>;

public:
	HMODULE Get(LPCWSTR name)
	{
		auto hModule = ::GetModuleHandleW(name);
		if (hModule == nullptr)
		{
			hModule = ::LoadLibraryW(name);
			if (hModule == nullptr)
				return nullptr;

			// unload library in the future
			m_libUnloadList.ApplyOperation( [hModule](auto& libs) {
				return libs.emplace_back(hModule);
			} );
		}
		return hModule;
	}


private:
	LibraryManager() = default;

	class LibraryUnloadList : public std::vector<HMODULE>
	{
	public:
		~LibraryUnloadList()
		{
			for (auto& item : *this)
				::FreeLibrary(item);
		}
	};

	gan::ThreadSafeResource<LibraryUnloadList> m_libUnloadList;
};



}  // unnamed namespace



namespace gan
{



void* DynamicCallBase::ObtainFunction(const wchar_t* libName, const char* funcName)
{
	auto hModule = LibraryManager::GetInstance().Get(libName);
	if (hModule == nullptr)
		return nullptr;
	return ::GetProcAddress(hModule, funcName);
}



}  // namespace gan
