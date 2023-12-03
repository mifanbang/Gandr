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

#include <PE.h>

#include <cassert>
#include <ranges>
#include <string_view>


namespace
{


// Fill in data in PeHeaders::sectionHeaderList
void SetUpSectionHeaders(gan::ConstMemAddr baseAddr, gan::PeHeaders& headers)
{
	const uint16_t numSections = headers.ntHeaders.fileHeader.NumberOfSections;
	headers.sectionHeaderList.reserve(numSections);

	gan::ConstMemAddr addrSectionHeader =
		baseAddr
		.Offset(headers.dosHeader.e_lfanew)
		.Offset(FIELD_OFFSET(gan::ImageNtHeaders, optHeader64))
		.Offset(headers.ntHeaders.fileHeader.SizeOfOptionalHeader);
	for (uint16_t i = 0; i < numSections; ++i)
	{
		headers.sectionHeaderList.emplace_back(addrSectionHeader.ConstRef<IMAGE_SECTION_HEADER>());
		addrSectionHeader = addrSectionHeader.Offset(sizeof(IMAGE_SECTION_HEADER));
	}
}


// Fill in data in PeHeaders::exportData
void SetUpExportDirectory(gan::ConstMemAddr baseAddr, gan::PeHeaders& headers)
{
	if (headers.ntHeaders.GetNumOfDataDirectories() < IMAGE_DIRECTORY_ENTRY_EXPORT)
		return;  // IMAGE_EXPORT_DIRECTORY doesn't exist in the image

	const auto& addrDir = headers.ntHeaders.GetDataDirectories()[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if (addrDir.Size == 0)
		return;  // Empty export directory

	// Initialize .directory
	const gan::ConstMemAddr addrExportDir = baseAddr.Offset(addrDir.VirtualAddress);
	headers.exportData = { .directory = addrExportDir.ConstRef<IMAGE_EXPORT_DIRECTORY>() };
	const auto& exportDir = headers.exportData->directory;
	assert(exportDir.NumberOfFunctions >= exportDir.NumberOfNames);

	// Initialize .functions
	auto& exportFuncs = headers.exportData->functions;
	exportFuncs.reserve(exportDir.NumberOfFunctions);

	// Export Address Table
	const gan::Range<gan::Rva> exportDataRange{
		.min = addrDir.VirtualAddress,
		.max = addrDir.VirtualAddress + addrDir.Size
	};
	const auto* addrTable = baseAddr.Offset(exportDir.AddressOfFunctions).ConstPtr<gan::Rva>();
	assert(addrTable);
	for (uint32_t i = 0; i < exportDir.NumberOfFunctions; ++i)
	{
		const auto addrFunc = addrTable[i];
		exportFuncs.emplace_back(
			addrFunc,
			static_cast<gan::Ordinal>(exportDir.Base + i)
		);

		// In the case of forwarding, RVA points to a string inside the range of exportDataRange.
		if (exportDataRange.InRange(addrFunc))
			exportFuncs.back().forwarding = true;
	}

	// Export Name Table
	const auto* nameTable = baseAddr.Offset(exportDir.AddressOfNames).ConstPtr<gan::Rva>();
	const auto* ordinalTable = baseAddr.Offset(exportDir.AddressOfNameOrdinals).ConstPtr<gan::Ordinal>();
	assert(nameTable);
	assert(ordinalTable);
	for (uint32_t i = 0; i < exportDir.NumberOfNames; ++i)
	{
		const std::string_view name = baseAddr.Offset(nameTable[i]).ConstPtr<char>();
		const auto unbiasedOrdinal = ordinalTable[i];

		assert(unbiasedOrdinal < exportFuncs.size());
		assert(exportFuncs[unbiasedOrdinal].ordinal == static_cast<gan::Ordinal>(exportDir.Base) + unbiasedOrdinal);
		exportFuncs[unbiasedOrdinal].name = name;
	}
}


}


namespace gan
{


std::optional<uint32_t> PeHeaders::FindSectionByName(uint32_t startIndex, std::u8string_view name) const noexcept
{
	static_assert(sizeof(IMAGE_SECTION_HEADER::Name) == IMAGE_SIZEOF_SHORT_NAME);

	if (startIndex < sectionHeaderList.size()
		&& name.length() <= IMAGE_SIZEOF_SHORT_NAME)
	{
		const auto itr = std::ranges::find_if(
			sectionHeaderList | std::views::drop(startIndex),
			[name] (const auto& sectionHeader) noexcept {
				// sectionHeader.Name isn't guaranteed to be null-terminated
				char8_t sectionNameBuf[IMAGE_SIZEOF_SHORT_NAME + 1]{};
				memcpy(sectionNameBuf, sectionHeader.Name, IMAGE_SIZEOF_SHORT_NAME);
				return std::u8string_view{ sectionNameBuf } == name;
			}
		);
		if (itr != sectionHeaderList.end())
			return std::optional<uint32_t>{ static_cast<uint32_t>(std::distance(sectionHeaderList.begin(), itr)) };
	}
	return std::optional<uint32_t>{ std::nullopt };
}


std::optional<PeHeaders> PeImageHelper::GetLoadedHeaders(ConstMemAddr addr)
{
	assert(addr);

	const auto& dosHeader = addr.ConstRef<IMAGE_DOS_HEADER>();
	PeHeaders headers{
		.dosHeader = dosHeader,
		.ntHeaders = addr.Offset(dosHeader.e_lfanew).ConstRef<ImageNtHeaders>()
	};

	// Only support images for 32-bit and 64-bit x86-based architectures
	const bool isSupported =
		headers.dosHeader.e_magic == IMAGE_DOS_SIGNATURE
		&& headers.ntHeaders.signature == IMAGE_NT_SIGNATURE
		&& (headers.ntHeaders.fileHeader.Machine == IMAGE_FILE_MACHINE_I386 || headers.ntHeaders.fileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
		&& (headers.ntHeaders.optHeader32.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC || headers.ntHeaders.optHeader64.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC);

	// Traverse the image to fill in more data
	std::optional<PeHeaders> resultOpt;
	if (isSupported)
	{
		SetUpSectionHeaders(addr, headers);
		SetUpExportDirectory(addr, headers);

		resultOpt = std::move(headers);
	}

	return resultOpt;
}


}  // namespace gan
