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
	static void Close(RawHandle handle) noexcept { ::BCryptCloseAlgorithmProvider(handle, 0); }
};
using AutoBcryptAlgHandle = gan::AutoHandle<AutoBcryptAlgHandleImpl>;

struct AutoBcryptHashHandleImpl
{
	using RawHandle = BCRYPT_HASH_HANDLE;
	static void Close(RawHandle handle) noexcept { ::BCryptDestroyHash(handle); }
};
using AutoBcryptHashHandle = gan::AutoHandle<AutoBcryptHashHandleImpl>;


}  // unnamed namespace


namespace gan
{


std::expected<Hash<256>, WinErrorCode> Hasher::GetSHA(ConstMemAddr dataAddr, size_t size)
{
	constexpr uint32_t k_emptyFlag = 0;

	AutoBcryptAlgHandle hProv{ };
	AutoBcryptHashHandle hHash{ };

	// Initialization of service provider
	constexpr const wchar_t* k_defaultProvider = nullptr;
	ULONG numByteRead{ 0 };
	uint32_t hashObjSize{ };
	const bool initSucceeded =
		BCRYPT_SUCCESS(::BCryptOpenAlgorithmProvider(
			&hProv.GetRef(),
			BCRYPT_SHA256_ALGORITHM,
			k_defaultProvider,
			k_emptyFlag
		))
		&& BCRYPT_SUCCESS(::BCryptGetProperty(
			*hProv,
			BCRYPT_OBJECT_LENGTH,
			reinterpret_cast<uint8_t*>(&hashObjSize),
			sizeof(hashObjSize),
			&numByteRead,
			k_emptyFlag
		));
	if (!initSucceeded)
		return std::unexpected{ ::GetLastError() };

	// Hash calculation
	constexpr uint8_t* k_noSecret = nullptr;
	constexpr uint32_t k_zeroSecretSize = 0;
	auto hashObject = std::make_unique<uint8_t[]>(hashObjSize);
	Hash<256> hash{ };
	const bool hashSucceeded =
		BCRYPT_SUCCESS(::BCryptCreateHash(
			*hProv,
			&hHash.GetRef(),
			hashObject.get(),
			hashObjSize,
			k_noSecret,
			k_zeroSecretSize,
			k_emptyFlag
		))
		// Win32 API bug: the 2nd param of BCryptHashData should be const as it's pure input
		// REF: https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcrypthashdata
		&& BCRYPT_SUCCESS(::BCryptHashData(
			*hHash,
			dataAddr.ConstCast().Ptr<uint8_t>(),
			static_cast<ULONG>(size),
			k_emptyFlag
		))
		&& BCRYPT_SUCCESS(::BCryptFinishHash(
			*hHash,
			hash.data(),
			sizeof(hash),
			k_emptyFlag
		));
	if (!hashSucceeded)
		return std::unexpected{ ::GetLastError() };

	return hash;
}


}  // namespace gan

