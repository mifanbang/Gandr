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

#include <Types.h>

#include <windows.h>

#include <optional>
#include <string>
#include <vector>


namespace gan
{


using Rva = uint32_t;
using Ordinal = uint16_t;


// Gandr's analog which combines IMAGE_NT_HEADERS32 and IMAGE_NT_HEADERS64
struct ImageNtHeaders
{
	DWORD signature;
	IMAGE_FILE_HEADER fileHeader;
	union
	{
		IMAGE_OPTIONAL_HEADER64 optHeader64;  // 64-bit optional header is bigger
		IMAGE_OPTIONAL_HEADER32 optHeader32;
	};

	constexpr Arch GetArch() const { return fileHeader.Machine == IMAGE_FILE_MACHINE_AMD64 ? Arch::Amd64 : Arch::IA32; }

	// Address getters
#define REQUIRES_SELF(t)	requires std::is_same_v<std::remove_cv_t<t>, ImageNtHeaders>
	template <class T> REQUIRES_SELF(T)
	auto GetDataDirectories(this T self) { return self.GetArch() == Arch::Amd64 ? self.optHeader64.DataDirectory : self.optHeader32.DataDirectory; }
#undef REQUIRES_SELF
};


// REF: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#the-edata-section-image-only
struct ImageExportData
{
	struct ExportedFunction
	{
		Rva rva;  // Relative virtual address
		Ordinal ordinal;
		bool forwarding : 1;
		std::string name;
	};
	using ExportedFunctionList = std::vector<ExportedFunction>;

	IMAGE_EXPORT_DIRECTORY directory;
	ExportedFunctionList functions;
};


// Aliases of Windows SDK types. Just to make PeHeaders easier to read.
using ImageDosHeader = IMAGE_DOS_HEADER;
using ImageSectionHeaderList = std::vector<IMAGE_SECTION_HEADER>;


struct PeHeaders
{
	// Basic part
	ImageDosHeader dosHeader;
	ImageNtHeaders ntHeaders;
	ImageSectionHeaderList sectionHeaderList;

	// Directory data
	ImageExportData exportData;
	
	// Helpers
	// Note: behavior is undefined if the PEHeaders instance isn't loaded by Gandr API, e.g., GetLoadedHeaders().
};


class PeImageHelper
{
public:
	static std::optional<PeHeaders> GetLoadedHeaders(ConstMemAddr addr);

};


}  // namespace gan
