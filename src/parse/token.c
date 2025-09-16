// Created by Lua (TeamPuzel) on September 4th 2025.
// Copyright (c) 2025 All rights reserved.
#include "token.h"
// Given that this is a leaf C file, the scope of macros is very limited.
// For this reason the manual pointer arithmetic and such is defined as helper macros for better readability.
// The rules are clearly defined so this is a very modular implementation which could easily be rewritten to avoid this.

#define CONSUME(N) *source += N
#define IS_CHAR(C) **cursor == C
#define YIELD(T) return (Token) { .tag = Token_##T }
#define YIELD_COUNT(T, COUNT) return (Token) { .tag = Token_##T, .count = COUNT }
#define YIELD_SLICE(T, COUNT, DATA) return (Token) { .tag = Token_##T, .count = COUNT, .data = DATA }
#define THROW(REASON) return (Token) { .tag = Token_Error, .data = REASON }

Token tokenize(const char ** source, const char * end) {
    if (*source >= end) YIELD(EndOfSource);

    const char ** cursor = source;

    if (IS_CHAR('\t')) THROW("Tab");

    if (IS_CHAR('\n')) {
        CONSUME(1);
        YIELD(LineEnd);
    }

    if (IS_CHAR(' '))

    *source = end;
    THROW("Unhandled Case");
}
