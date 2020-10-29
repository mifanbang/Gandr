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

#include <Hash.h>

#include <Handle.h>

#include <memory>

#include <windows.h>
#include <bcrypt.h>  // Must be included after windows.h

#pragma comment(lib, "bcrypt.lib")



namespace gan
{



WinErrorCode Hasher::GetSHA(const void* data, size_t size, Hash<256>& out)
{
	AutoHandle hProv(BCRYPT_ALG_HANDLE(nullptr), [](auto prov) { ::BCryptCloseAlgorithmProvider(prov, 0); });
	AutoHandle hHash(BCRYPT_HASH_HANDLE(nullptr), ::BCryptDestroyHash);

	// Initialization of service provider
	bool isSuccessful = true;
	ULONG numByteRead = 0;
	uint32_t hashObjSize = 1;
	isSuccessful = isSuccessful && BCRYPT_SUCCESS(::BCryptOpenAlgorithmProvider(&hProv.GetRef(), BCRYPT_SHA256_ALGORITHM, nullptr, 0));
	isSuccessful = isSuccessful && BCRYPT_SUCCESS(::BCryptGetProperty(hProv, BCRYPT_OBJECT_LENGTH, gan::MemAddr(&hashObjSize), sizeof(hashObjSize), &numByteRead, 0));

	// Hash calculation
	std::unique_ptr<uint8_t[]> hashObj(new uint8_t[hashObjSize]);
	Hash<256> hash { { 0 } };
	isSuccessful = isSuccessful && BCRYPT_SUCCESS(::BCryptCreateHash(hProv, &hHash.GetRef(), hashObj.get(), hashObjSize, nullptr, 0, 0));
	isSuccessful = isSuccessful && BCRYPT_SUCCESS(::BCryptHashData(hHash, gan::MemAddr(data), static_cast<ULONG>(size), 0));
	isSuccessful = isSuccessful && BCRYPT_SUCCESS(::BCryptFinishHash(hHash, gan::MemAddr(&hash.data), sizeof(hash), 0));
	if (!isSuccessful)
		return GetLastError();

	::CopyMemory(&out, &hash, sizeof(hash));
	return NO_ERROR;
}



}  // namespace gan

