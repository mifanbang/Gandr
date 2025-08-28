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

#include <ModuleList.h>

#include <Handle.h>

#include <windows.h>
#include <tlhelp32.h>  // must be included after windows.h, which sucks


namespace
{


HANDLE GetModuleListSnapshop(uint32_t processId) noexcept
{
	constexpr uint32_t k_list32And64Modules = TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32;

	HANDLE snap;
	while ((snap = ::CreateToolhelp32Snapshot(k_list32And64Modules, processId)) == INVALID_HANDLE_VALUE)
	{
		// ERROR_BAD_LENGTH indicates that the target process is still loading modules and we should retry.
		if (::GetLastError() != ERROR_BAD_LENGTH)
			break;
	}
	return snap;
}

gan::ModuleInfo MakeModuleInfo(const MODULEENTRY32W& moduleEntry)
{
	return {
		.base{ gan::ConstMemAddr{ moduleEntry.modBaseAddr } },
		.size{ moduleEntry.modBaseSize },
		.imageName{ moduleEntry.szModule },
		.imagePath{ moduleEntry.szExePath }
	};
}



}  // unnamed namespace


namespace gan
{


std::expected<ModuleList, ModuleEnumerator::Error> ModuleEnumerator::Enumerate(uint32_t processId)
{
	AutoWinHandle hSnap{ GetModuleListSnapshop(processId) };
	if (!hSnap)
		return std::unexpected{ Error::SnapshotFailed };

	ModuleList moduleList;
	MODULEENTRY32W modEntry{ .dwSize = sizeof(modEntry) };

	for (BOOL mod32Result{ ::Module32FirstW(*hSnap, &modEntry) };
		mod32Result;
		mod32Result = ::Module32NextW(*hSnap, &modEntry))
	{
		moduleList.emplace_back(MakeModuleInfo(modEntry));
	}

	// In a success, Module32Next() would end with returning FALSE and setting error code to ERROR_NO_MORE_FILES
	if (moduleList.empty() || ::GetLastError() != ERROR_NO_MORE_FILES)
		return std::unexpected{ Error::Module32Failed };

	return moduleList;
}

std::expected<ModuleList, ModuleEnumerator::Error> ModuleEnumerator::Enumerate(WinHandle process)
{
	return Enumerate(::GetProcessId(process));
}


}  // namespace gan
