// Created by Lua (TeamPuzel) on September 4th 2025.
// Copyright (c) 2025 All rights reserved.
#ifndef PRIMITIVE_H
#define PRIMITIVE_H
#include <stdlib.h>  // IWYU pragma: export
#include <stddef.h>  // IWYU pragma: export
#include <stdint.h>  // IWYU pragma: export
#include <stdbool.h> // IWYU pragma: export

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define ERROR_CODE_SUCCESS 0
#define ERROR_CODE_FAILURE 1
#define ERROR_CODE_INVALID_USE 2
#define ERROR_CODE_INTERNAL_INVARIANT 3

#define INVARIANT_CHECK

static inline bool memory_eq(const void * lhs, const void * rhs, size_t size) {
    const u8 * ulhs = lhs;
    const u8 * urhs = rhs;
    for (size_t i = 0; i < size; i += 1) if (ulhs[i] != urhs[i]) return false;
    return true;
}

static inline bool string_eq(const char * lhs, const char * rhs) {
    size_t i;
    for (i = 0; lhs[i] != 0 && rhs[i] != 0; i += 1) if (lhs[i] != rhs[i]) return false;
    return lhs[i] == rhs[i];
}

static inline void panic(const char * reason) {
    exit(-1);
}

static inline void invariant(bool condition, const char * reason) {
#ifdef INVARIANT_CHECK
    if (!condition) panic(reason);
#endif
}

#endif
