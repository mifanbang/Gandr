;
;  Gandr - another minimalism library for hacking x86-based Windows
;  Copyright (C) 2020-2023 Mifan Bang <https://debug.tw>.
;
;  This program is free software: you can redistribute it and/or modify
;  it under the terms of the GNU General Public License as published by
;  the Free Software Foundation, either version 3 of the License, or
;  (at your option) any later version.
;
;  This program is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License
;  along with this program.  If not, see <https://www.gnu.org/licenses/>.
;

IFDEF _WIN64

	_TEXT SEGMENT

	; IP can be assigned with LoadLibraryW anytime within the loop starting
	; at LoopStart, so a nonvolatile register, e.g., RBX, must be used or
	; the MOV instruction is going to cause access violation.
	Test_DllInjector_NewThreadProc PROC PUBLIC
	option PROLOGUE:NONE, EPILOGUE:NONE
		push	rbx
		sub 	rsp, 20h
		mov 	rbx, rcx
	LoopStart:
		mov 	dword ptr [rbx], 1
		jmp 	LoopStart
	Test_DllInjector_NewThreadProc ENDP

	Test_DllInjector_ExitThreadProc PROC PUBLIC
	option PROLOGUE:NONE, EPILOGUE:NONE
		add 	rsp, 20h
		pop 	rbx
		xor 	rax, rax
		ret
	Test_DllInjector_ExitThreadProc ENDP

	_TEXT ENDS

ELSE

	.MODEL FLAT, STDCALL

	_TEXT SEGMENT

	Test_DllInjector_NewThreadProc PROC PUBLIC signal:DWORD
	option PROLOGUE:NONE, EPILOGUE:NONE
		push	ebx
		mov 	ebx, dword ptr signal
	LoopStart:
		mov 	dword ptr [ebx], 1
		jmp 	LoopStart
	Test_DllInjector_NewThreadProc ENDP

	Test_DllInjector_ExitThreadProc PROC
	option PROLOGUE:NONE, EPILOGUE:NONE
		pop 	ebx
		xor 	eax, eax
		ret 	4
	Test_DllInjector_ExitThreadProc ENDP

	_TEXT ENDS

ENDIF


END
