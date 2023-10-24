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

#include <DllInjector.h>

#include <Buffer.h>
#include <DynamicCall.h>

#include <cassert>
#include <functional>
#include <memory>

#include <windows.h>


#ifdef _WIN64
	#define GET_CONTEXT_REG(ctx, reg)	(ctx.R##reg)
#else
	#define GET_CONTEXT_REG(ctx, reg)	(ctx.E##reg)
#endif  // defined _WIN64


namespace
{


class InjectionHelper
{
public:
	template <gan::Arch arch>
	static std::unique_ptr<gan::Buffer> GenerateStackFrameAndUpdateContext(CONTEXT&, const wchar_t*);


	template <>
	static std::unique_ptr<gan::Buffer> GenerateStackFrameAndUpdateContext<gan::Arch::IA32>(CONTEXT& context, const wchar_t* remoteDllPath)
	{
		// write faked stack frame
		struct StackFrameForLoadLibraryW32
		{
			// for LoadLibraryW()
			LPCVOID pRetAddrVirtualFree;
			LPCWSTR pDllPath;

			// for VirtualFree()
			LPCVOID pRetAddrOrigin;
			LPCVOID pMemAddr;
			uint32_t size;  // SIZE_T is DWORD on Win32
			uint32_t freeType;
		};

		gan::DynamicCall<decltype(VirtualFree)> funcVirtualFree(L"kernel32", "VirtualFree");
		assert(funcVirtualFree.IsValid());

		auto output = gan::Buffer::Allocate(sizeof(StackFrameForLoadLibraryW32));
		assert(output);
		if (!output)
			return nullptr;

		GET_CONTEXT_REG(context, sp) -= output->GetSize();

		auto bufferData = gan::MemAddr{ output->GetData() };
		bufferData.Ref<StackFrameForLoadLibraryW32>() = {
			// for LoadLibraryW()
			funcVirtualFree.GetAddress(),
			remoteDllPath,

			// for VirtualFree()
			reinterpret_cast<LPVOID>(GET_CONTEXT_REG(context, ip)),
			remoteDllPath,
			0,
			MEM_RELEASE
		};

		SetIPToLoadLibraryW(context);

		return std::move(output);
	}

	template <>
	static std::unique_ptr<gan::Buffer> GenerateStackFrameAndUpdateContext<gan::Arch::Amd64>(CONTEXT& context, const wchar_t* remoteDllPath)
	{
		struct StackFrameForLoadLibraryW64
		{
			// Return address to original IP from LoadLibraryW()
			LPVOID pRetAddrOrigin;
		};

		auto output = gan::Buffer::Allocate(sizeof(StackFrameForLoadLibraryW64));
		assert(output);
		if (!output)
			return nullptr;

		GET_CONTEXT_REG(context, sp) -= output->GetSize();
		GET_CONTEXT_REG(context, cx) = reinterpret_cast<size_t>(remoteDllPath);  // Arg to LoadLibraryW

		gan::MemAddr bufferData{ output->GetData() };
		bufferData.Ref<StackFrameForLoadLibraryW64>() = {
			reinterpret_cast<LPVOID>(GET_CONTEXT_REG(context, ip))
		};

		SetIPToLoadLibraryW(context);

		return std::move(output);
	}


private:
	static void SetIPToLoadLibraryW(CONTEXT& context)
	{
		gan::DynamicCall<decltype(::LoadLibraryW)> funcLoadLibraryW{ L"kernel32", "LoadLibraryW" };
		assert(funcLoadLibraryW.IsValid());

		GET_CONTEXT_REG(context, ip) = reinterpret_cast<size_t>(funcLoadLibraryW.GetAddress());
	}
};


}  // unnamed namespace


namespace gan
{


DllInjectorByContext::DllInjectorByContext(WinHandle hProcess, WinHandle hThread)
	: m_hProcess()
	, m_hThread()
{
	assert(hProcess != nullptr);
	assert(hThread != nullptr);

	HANDLE hCurrentProc = ::GetCurrentProcess();
	HANDLE hDuplicated = INVALID_HANDLE_VALUE;

	constexpr DWORD k_desiredAccessIgnored = 0;  // Because DUPLICATE_SAME_ACCESS is specified
	constexpr BOOL k_handleNotInheritable = FALSE;
	const auto dupProcResult = ::DuplicateHandle(
		hCurrentProc,
		hProcess,
		hCurrentProc,
		&hDuplicated,
		k_desiredAccessIgnored,
		k_handleNotInheritable,
		DUPLICATE_SAME_ACCESS
	);
	assert(dupProcResult);
	m_hProcess = hDuplicated;

	const auto dupThreadResult = ::DuplicateHandle(
		hCurrentProc,
		hThread,
		hCurrentProc,
		&hDuplicated,
		k_desiredAccessIgnored,
		k_handleNotInheritable,
		DUPLICATE_SAME_ACCESS
	);
	assert(dupThreadResult);
	m_hThread = hDuplicated;
}


DllInjectorByContext::Result DllInjectorByContext::Inject(const wchar_t* pDllPath)
{
	constexpr DWORD k_contextFlags{ CONTEXT_INTEGER | CONTEXT_CONTROL };

	CONTEXT context;
	ZeroMemory(&context, sizeof(context));

	// Make a copy of registers of interest
	context.ContextFlags = k_contextFlags;
	if (::GetThreadContext(*m_hThread, &context) == FALSE)
		return Result::GetContextFailed;

	// Allocate buffer in the memory space of target process and write DLL path to it
	size_t dwBufferSize = sizeof(WCHAR) * (wcslen(pDllPath) + 1);
	auto remoteBuffer = reinterpret_cast<LPWSTR>(::VirtualAllocEx(*m_hProcess, nullptr, dwBufferSize, MEM_COMMIT, PAGE_READWRITE));
	const bool isDllPathWritten = (remoteBuffer && ::WriteProcessMemory(*m_hProcess, remoteBuffer, pDllPath, dwBufferSize, nullptr) != 0);
	if (!isDllPathWritten)
		return Result::DLLPathNotWritten;

	// Generate a "synthesized" stack frame and modify registers accordingly
	auto bufferStackFrame = InjectionHelper::GenerateStackFrameAndUpdateContext<BuildArch()>(context, remoteBuffer);
	assert(bufferStackFrame);
	if (!bufferStackFrame)
		return Result::StackFrameNotWritten;

	// Write the stack frame target process memory
	const auto stackFrameWritten = ::WriteProcessMemory(
		*m_hProcess,
		reinterpret_cast<LPVOID>(GET_CONTEXT_REG(context, sp)),
		bufferStackFrame->GetData(),
		bufferStackFrame->GetSize(),
		nullptr
	);
	if (stackFrameWritten == FALSE)
		return Result::StackFrameNotWritten;

	// Manipulate IP (and other registers on AMD64) to fake a function call
	context.ContextFlags = k_contextFlags;
	if (::SetThreadContext(*m_hThread, &context) == FALSE)
		return Result::SetContextFailed;

	return Result::Succeeded;
}


}  // namespace gan
