# Gandr

[![Coverity Scan Build Status](https://scan.coverity.com/projects/29260/badge.svg)](https://scan.coverity.com/projects/mifanbang-gandr)
<br>A minimalism user-mode library for hacking x86-based Windows processes

## Introduction

Gandr was originally written as the foundation of [Purifier](https://github.com/mifanbang/Purifier), an ad-remover for older Windows versions of Skype. The library aims to wrap Win32 API into simple yet modern C++ interface and accelerate your work to mess up target processes in userland. Both 32-bit and 64-bit targets are supported.

## Features

This is a non-exhaustive list of main features of Gandr:

- Support of modern C++ standards, e.g., move semantics

- A wrapper of [Windows debugging API](https://docs.microsoft.com/en-us/windows/win32/debug/debugging-functions) (classes `Debugger` and `DebugSession`)

- A DLL injector (class `DllPreloadDebugSession` for the highest-level use case) which creates a new process and forces it to load a specific DLL right before any code of the victim executable is run

- An *overly* simple x86+amd64 instruction length decoder (class `InstructionDecoder`)

- A helper for installing and uninstalling inline hooks (class `Hook`)

## Building Gandr

Visual Studio 2022 is used for the development of Gandr. The solution consists of two parts: a static library, and unit tests.

There is no external dependency required by the solution, so all you need to do is hitting the "Build Solution" button. Compiled and linked binary files can then be found in the folder `bin\[project name]\[platform]\[build configuration]\`.

## Using Gandr

All Gandr headers exposed to users are located in the folder `include\`. You should add the path to this folder as an additional include path in the building environment of your application. You will also need to add paths to Gandr static library binaries (mentioned in the *Build the Code* section above) as well.

Due to my shameful laziness, there is currently no documentation for the Gandr API. However the test code should provide you with decent examples of how to use most classes.

Additionally, as a bonus, `Test.cpp` and `Test.h` from the project `Test` constitute yet another minimalism unit test framework that might hopefully be helpful to your project.

## Selected Code Examples

### Preloading a DLL to a New Process

The following example illustrates how easy it is to create a process and force it to preload a DLL before any code in the target executable file is executed. You can find a slightly more verbose example in the test code `TestDebugger.cpp`.

```cpp
gan::DebugSession::CreateProcessParam param;
param.imagePath = L"victimProcess.exe";

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

### Installing a Hook

Class `Hook` requires function signatures of the target and the hook to exactly match. This ensures that both have the same number of parameters, calling convention, and other stuff which may potential crash the process. Please note that compiler's inlining optimization might break the example below. For more examples of `Hook` usage, see test code `TestHook.cpp`.

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

## Copyright

Copyright (C) 2020-2023 Mifan Bang <https://debug.tw>.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
