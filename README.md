# Gandr

[![Coverity Scan Build Status](https://scan.coverity.com/projects/29260/badge.svg)](https://scan.coverity.com/projects/mifanbang-gandr)
<br>A minimalism user-mode library for hacking x86-based Windows processes

## Introduction

*Gandr* was originally written as the foundation for a couple of other projects that remove/block ads in certain Windows application. The library features wrapping low-level system operations into clean and modern C++ interface to help accelerate your work of hacking in userland. 32-bit and 64-bit configurations are both supported.

## Features

This is a non-exhaustive list of main features of Gandr:

- Support of modern C++ standards, *e.g.*, move semantics

- A wrapper of [Windows debugging API](https://docs.microsoft.com/en-us/windows/win32/debug/debugging-functions) (classes `Debugger` and `DebugSession`)

- A DLL injector (class `DllPreloadDebugSession` for the highest-level use case) which creates a new process and forces it to load a specific DLL right before any code of the victim executable is run

- An *overly* simplified x86+amd64 instruction length decoder (class `InstructionDecoder`)

- A helper for installing and uninstalling inline hooks (class `Hook`)

## Build Instructions

Visual Studio 2022 is used in the development of Gandr. The solution consists of two parts: a static library, and unit tests.

There is no external dependency required by the solution, so all you need to do is hitting the "Build Solution" button. Compiled and linked binary files can then be found in the folder `bin\[project name]\[platform]\[build configuration]\`.

## Using Gandr

All Gandr headers exposed to users are located in the folder `include\`. You should add the path to this folder as an additional include path in the building environment of your project. You will also need to add paths to Gandr static library binaries (mentioned in the *Build Instructions* section above) as well.

Release builds of Gandr are configured with the `/MT` compiler flag ("Multi-threaded" for the "Runtime Library" property), and Debug builds with `/MDd`. Please make sure your projects are using the same flags.

Additionally, as a bonus, `Test.cpp` and `Test.h` from the project `Test` constitute yet another minimalism unit test framework that might hopefully be helpful to your projects.

## Selected Code Examples

Due to my shameful laziness, there is currently no documentation for the Gandr API. However the test code should provide you with decent examples of how to use most classes.

### Preloading a DLL to a new process

The following example illustrates how easy it is to create a process and force it to preload a DLL before any code in the target executable file is executed.

```cpp
gan::DebugSession::CreateProcessParam param{
    .imagePath = L"victimProcess.exe"
};

// Prepare to preload myHackCode.dll into victimProcess.exe
gan::Debugger debugger;
debugger.AddSession<gan::DllPreloadDebugSession>(
    param,
    L"myHackCode.dll",
    gan::DllPreloadDebugSession::Option::EndSessionSync
);

// Run the debugger event loop. DllPreloadDebugSession will make
// it return once its job is done.
const auto result = debugger.EnterEventLoop();
assert(result == gan::Debugger::EventLoopResult::AllDetached);
```

You can find another example which is slightly more verbose than the one above in the source file `src\Test\TestDebugger.cpp`.

### Installing a hook

Class `Hook` requires both target and hook functions to have the exact same signature. This ensures that both have the same number of parameters, calling convention, and other stuff which may potential crash the process. The following example is a code snippet from the source file `src\Test\TestHook.cpp`. More usage of the class `Hook` can be found in the said file.

```cpp
__declspec(noinline) size_t Add(size_t n1, size_t n2) {
    return n1 + n2;
}
__declspec(noinline) size_t Mul(size_t n1, size_t n2) {
    return n1 * n2;
}

// Add the hook Mul() on Add() so we can bring chaos to the world.
gan::Hook hook { Add, Mul };
const auto result = hook.Install();
assert(result == gan::Hook::OpResult::Hooked);
assert(Add(123, 321) == 444);  // Oh no! This assert will fail!
```

### Instruction length decoding

Class `InstructionDecoder` supports a very limited (but very commonly-used!) set of x86/amd64 instructions in both 32-bit and 64-bit modes. Also, the mode is passed in as an argument which enables you to decode instructions in a mode different from the one your code is built for. The following example is a code snippet from the source file `src\Test\TestInstructionDecoder64.cpp`:

```cpp
// REX.WB mov  r15, 0BBAA785600003412h
const static uint8_t k_inMovImm64ToReg64[] {
    0x49, 0xBF, 0x12, 0x34, 0, 0, 0x56, 0x78, 0xAA, 0xBB
};

gan::InstructionDecoder decoder(
    gan::Arch::Amd64,
    gan::ConstMemAddr{ k_inMovImm64ToReg64 }
);
const auto lengthDetails = decoder.GetNextLength();
assert(lengthDetails);
assert(lengthDetails->lengthOp == 1);
assert(lengthDetails->lengthDisp == 0);
assert(lengthDetails->lengthImm == 8);
assert(lengthDetails->GetLength() == 10);
```

Please do note that `InstructionDecoder` works like a forward iterator: every time its `.GetNextLength()` is called, its internal memory pointer increments to point to the next instruction ready for decoding.

## Copyright

Copyright (C) 2020-2023 Mifan Bang <https://debug.tw>.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
