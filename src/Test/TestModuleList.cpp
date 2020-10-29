/*
 *  Gandr - another minimalism library for hacking x86-based Windows
 *  Copyright (C) 2020 Mifan Bang <https://debug.tw>.
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

#include "Test.h"

#include <ModuleList.h>

#include <algorithm>

#include <shlwapi.h>
#include <windows.h>



namespace
{
	bool SearchModInList(const gan::ModuleList& modList, const wchar_t* modName)
	{
		const auto funcMatchMod = [modName](const gan::ModuleInfo& modInfo) {
			return StrStrIW(modInfo.imageName.c_str(), modName) != nullptr;
		};
		return std::find_if(modList.begin(), modList.end(), funcMatchMod) != modList.end();
	}
}  // unnamed namespace



DEFINE_TESTSUITE_START(ModuleList)

	DEFINE_TEST_START(CheckModulesOfCurrentProcess)
	{
		gan::ModuleList moduleList;
		ASSERT(gan::ModuleEnumerator::Enumerate(GetCurrentProcessId(), moduleList) == gan::ModuleEnumerator::Result::Success);

		EXPECT(SearchModInList(moduleList, L"Test.exe"));
		EXPECT(SearchModInList(moduleList, L"kernel32.dll"));
	}
	DEFINE_TEST_END



DEFINE_TESTSUITE_END

