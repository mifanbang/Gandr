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

#include "Test.h"

#include <PE.h>

#include <optional>
#include <psapi.h>
#include <windows.h>

#include <algorithm>
#include <ranges>
#include <string_view>


using namespace std::literals;


namespace
{


std::pair<HMODULE, void*> GetModuleInfo(const wchar_t* modName)
{
	auto hMod = ::GetModuleHandleW(modName);

	MODULEINFO modInfo{ };
	if (hMod)
		::GetModuleInformation(::GetCurrentProcess(), hMod, &modInfo, sizeof(modInfo));

	return std::make_pair(hMod, modInfo.lpBaseOfDll);
}


gan::Rva SearchFunctionRvaByName(const gan::ImageExportData::ExportedFunctionList& exportData, std::string_view name)
{
	const auto itr = std::ranges::find_if(
		exportData,
		[name] (const auto& exportFunc) { return name == exportFunc.name; }
	);
	return itr != exportData.end() ? itr->rva : 0;
}


}  // unnamed namespace


DEFINE_TESTSUITE_START(PE)

	DEFINE_TEST_START(TestExeHasNoExport)
	{
		auto [hMod, baseAddr] = GetModuleInfo(L"Test.exe");
		ASSERT(hMod);
		ASSERT(baseAddr);

		const gan::ConstMemAddr modBaseAddr{ baseAddr };
		auto peHeaders = gan::PeImageHelper::GetLoadedHeaders(modBaseAddr);
		ASSERT(peHeaders);
		EXPECT(!peHeaders->exportData);
	}
	DEFINE_TEST_END

	DEFINE_TEST_START(Export_Kernel32_GetCurrentThreadId)
	{
		auto [hMod, baseAddr] = GetModuleInfo(L"kernel32");
		ASSERT(hMod);
		ASSERT(baseAddr);

		const gan::ConstMemAddr modBaseAddr{ baseAddr };
		auto peHeaders = gan::PeImageHelper::GetLoadedHeaders(modBaseAddr);
		ASSERT(peHeaders);
		ASSERT(peHeaders->exportData);

		constexpr static auto k_funcName = "GetCurrentThreadId"sv;
		const auto addrLoadedFunction = ::GetProcAddress(hMod, k_funcName.data());
		ASSERT(addrLoadedFunction);
		EXPECT(
			addrLoadedFunction ==
				modBaseAddr
				.Offset(SearchFunctionRvaByName(peHeaders->exportData->functions, k_funcName))
				.ConstPtr<void>()
		);
	}
	DEFINE_TEST_END

	DEFINE_TEST_START(Export_User32_CreateWindowExW)
	{
		auto [hMod, baseAddr] = GetModuleInfo(L"user32");
		ASSERT(hMod);
		ASSERT(baseAddr);

		const gan::ConstMemAddr modBaseAddr{ baseAddr };
		auto peHeaders = gan::PeImageHelper::GetLoadedHeaders(modBaseAddr);
		ASSERT(peHeaders);
		ASSERT(peHeaders->exportData);

		constexpr static auto k_funcName = "CreateWindowExW"sv;
		const auto addrLoadedFunction = ::GetProcAddress(hMod, k_funcName.data());
		ASSERT(addrLoadedFunction);
		EXPECT(
			addrLoadedFunction ==
				modBaseAddr
				.Offset(SearchFunctionRvaByName(peHeaders->exportData->functions, k_funcName))
				.ConstPtr<void>()
		);
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END
