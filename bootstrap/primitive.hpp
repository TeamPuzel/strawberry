// The Strawberry Programming Language Toolchain.
// Copyright (c) 2026 Lua (TeamPuzel)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

using f32 = float;
using f64 = double;

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;

using i8 = signed char;
using i16 = signed short;
using i32 = signed int;

using u64 = unsigned long;
using i64 = signed long;

using usize = u64;
using isize = i64;

static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);

static_assert(sizeof(u8) == 1);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(u64) == 8);

static_assert(sizeof(i8) == 1);
static_assert(sizeof(i16) == 2);
static_assert(sizeof(i32) == 4);
static_assert(sizeof(i64) == 8);

// We can assume we are running a 64 bit system as everyone dropped 32 bit support by now and I don't care.
static_assert(sizeof(usize) == sizeof(void*));
static_assert(sizeof(isize) == sizeof(void*));

// This is required by C++20, however it technically isn't in prior versions.
// Since this is C++ after all, best assert that we're not using a weird compiler, just in case.
static_assert(-4 >> 1 == -2, ">> doesn't perform sign extension");

// C++ does not define the representation of signed primitives either.
// Assert that this is a modern and sensible compiler/architecture.
// This is a requirement. Code can rely on this representation from this point on.
static_assert(static_cast<i8>(-1) == ~i8(0), "compiler not using two's complement");
static_assert(static_cast<i32>(-1) == ~i32(0), "compiler not using two's complement");
static_assert(static_cast<i64>(-1) == ~i64(0), "compiler not using two's complement");
