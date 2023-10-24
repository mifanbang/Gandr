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

#include <Hash.h>

#include <Handle.h>

#include <windows.h>
#include <bcrypt.h>  // Win32 API bug: must be included after windows.h

#include <memory>

#pragma comment(lib, "bcrypt.lib")


namespace
{


struct AutoBcryptAlgHandleImpl
{
	using RawHandle = BCRYPT_ALG_HANDLE;
	static void Close(RawHandle handle)	{ ::BCryptCloseAlgorithmProvider(handle, 0); }
};
using AutoBcryptAlgHandle = gan::AutoHandle<AutoBcryptAlgHandleImpl>;


struct AutoBcryptHashHandleImpl
{
	using RawHandle = BCRYPT_HASH_HANDLE;
	static void Close(RawHandle handle) { ::BCryptDestroyHash(handle); }
};
using AutoBcryptHashHandle = gan::AutoHandle<AutoBcryptHashHandleImpl>;


}  // unnamed namespace


namespace gan
{


WinErrorCode Hasher::GetSHA(ConstMemAddr dataAddr, size_t size, Hash<256>& out)
{
	AutoBcryptAlgHandle hProv{ };
	AutoBcryptHashHandle hHash{ };

	// Initialization of service provider
	ULONG numByteRead = 0;
	uint32_t hashObjSize = 1;
	const bool initSucceeded =
		BCRYPT_SUCCESS(::BCryptOpenAlgorithmProvider(
			&hProv.GetRef(),
			BCRYPT_SHA256_ALGORITHM,
			nullptr,
			0
		))
		&& BCRYPT_SUCCESS(::BCryptGetProperty(
			*hProv,
			BCRYPT_OBJECT_LENGTH,
			reinterpret_cast<uint8_t*>(&hashObjSize),
			sizeof(hashObjSize),
			&numByteRead,
			0
		));

	// Hash calculation
	std::unique_ptr<uint8_t[]> hashObj(new uint8_t[hashObjSize]);
	Hash<256> hash { { 0 } };
	const bool hashSucceeded =
		initSucceeded
		&& BCRYPT_SUCCESS(::BCryptCreateHash(
			*hProv,
			&hHash.GetRef(),
			hashObj.get(),
			hashObjSize,
			nullptr,
			0,
			0
		))
		// Win32 API bug: the 2nd param of BCryptHashData should be const as it's pure input
		// REF: https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcrypthashdata
		&& BCRYPT_SUCCESS(::BCryptHashData(
			*hHash,
			dataAddr.ConstCast().Ptr<uint8_t>(),
			static_cast<ULONG>(size),
			0
		))
		&& BCRYPT_SUCCESS(::BCryptFinishHash(
			*hHash,
			reinterpret_cast<uint8_t*>(&hash.data),
			sizeof(hash),
			0
		));
	if (!hashSucceeded)
		return GetLastError();

	out = hash;
	return NO_ERROR;
}


}  // namespace gan

