// Created by Lua (TeamPuzel) on September 4th 2025.
// Copyright (c) 2025 All rights reserved.
#include "token.h"
// Given that this is a leaf C file, the scope of macros is very limited.
// For this reason the manual pointer arithmetic and such is defined as helper macros for better readability.
// The rules are clearly defined so this is a very modular implementation which could easily be rewritten to avoid this.
// That being said just look at it, the procedure is almost no code at all. The macros are not doing much harm.

#define CONSUME(N) *source += N
#define REWIND(N) *source -= N
#define IS(C) (**source == C)
#define IS_NOT(C) (**source != C)
#define IS_IN_RANGE(FROM, TO) (**source >= FROM && **source <= TO)
#define IS_NOT_IN_RANGE(FROM, TO) !(**source >= FROM && **source <= TO)
#define ARE(STR) string_slice_to_literal_match(end - *source, *source, sizeof(STR) - 1, STR)
#define ARE_NOT(STR) !string_slice_to_literal_match(end - *source, *source, sizeof(STR) - 1, STR)
#define OVER (*source >= end)
#define NOT_OVER (*source < end)
#define LEFT (end - *source)
#define YIELD(T) return (Token) { .tag = Token_##T }
#define YIELD_COUNT(T, COUNT) return (Token) { .tag = Token_##T, .count = COUNT }
#define YIELD_SLICE(T, COUNT, DATA) return (Token) { .tag = Token_##T, .count = COUNT, .data = DATA }
#define THROW(REASON) return (Token) { .tag = Token_Error, .data = REASON }

// This is a specialized function which checks a sequence of characters for a match of a prefix.
static inline bool string_slice_to_literal_match(
    u16 count, const char * data, u16 pattern_count, const char * pattern_data
) {
    if (count < pattern_count) return false;
    for (size_t i = 0; i < pattern_count; i += 1) {
        if (data[i] != pattern_data[i]) return false;
    }

    return true;
}

Token tokenize(const char ** source, const char * end) {
    if (OVER) YIELD(EndOfSource);

    if (IS(' ')) {
        u16 count = 0;

        do {
            CONSUME(1);
            count += 1;
        } while (NOT_OVER && IS(' '));

        YIELD_COUNT(Whitespace, count);
    } else if (IS('\n')) {
        CONSUME(1); YIELD(LineEnd);
    } else if (IS('(')) {
        CONSUME(1); YIELD(LeftParen);
    } else if (IS(')')) {
        CONSUME(1); YIELD(RightParen);
    } else if (IS('{')) {
        CONSUME(1); YIELD(LeftBrace);
    } else if (IS('}')) {
        CONSUME(1); YIELD(RightBrace);
    } else if (IS('[')) {
        CONSUME(1); YIELD(LeftBracket);
    } else if (IS(']')) {
        CONSUME(1); YIELD(RightBracket);
    } else if (IS(',')) {
        CONSUME(1); YIELD(Comma);
    } else if (IS('.')) {
        CONSUME(1); YIELD(Dot);
    } else if (IS(':')) {
        CONSUME(1); YIELD(Colon);
    } else if (IS(';')) {
        CONSUME(1); YIELD(Semicolon);
    } else if (ARE("//")) {
        CONSUME(2);

        bool is_documentation;
        if (IS(' ')) {
            CONSUME(1);
            is_documentation = false;
        } else if (ARE("/ ")) {
            CONSUME(2);
            is_documentation = true;
        } else {
            THROW("invalid comment syntax");
        }

        const char * data = *source;
        u16 count = 0;

        while (NOT_OVER && IS_NOT('\n')) {
            CONSUME(1);
            count += 1;
        }

        if (is_documentation) {
            YIELD_SLICE(DocumentationComment, count, data);
        } else {
            YIELD_SLICE(Comment, count, data);
        }
    } else if (ARE("\\\\ ")) {
        CONSUME(3);
        const char * data = *source;
        u16 count = 0;

        while (NOT_OVER && IS_NOT('\n')) {
            CONSUME(1);
            count += 1;
        }

        YIELD_SLICE(MultilineString, count, data);
    } else if (IS('"')) {
        CONSUME(1);
        const char * data = *source;
        u16 count = 0;

        while (NOT_OVER && IS_NOT('"')) {
            if (IS('\n')) THROW("unterminated string");
            CONSUME(1);
            count += 1;
        }

        CONSUME(1);
        YIELD_SLICE(String, count, data);
    } else if (IS('\'')) {
        CONSUME(1);
        const char * data = *source;
        u16 count = 0;

        while (NOT_OVER && IS_NOT('\'')) {
            if (IS('\n')) THROW("unterminated character");
            CONSUME(1);
            count += 1;
        }

        CONSUME(1);
        YIELD_SLICE(String, count, data);
    } else if (IS_IN_RANGE('0', '9')) {
        const char * data = *source;
        u16 count = 0;

        #define IS_VALID_DIGIT \
            (IS_IN_RANGE('0', '9') || IS_IN_RANGE('A', 'Z') || IS_IN_RANGE('a', 'z'))

      num_loop:
        CONSUME(1); count += 1;
        if (OVER) ;
        else if (IS_VALID_DIGIT) goto num_loop;
        else if (IS('_')) {
            CONSUME(1); if (!IS_VALID_DIGIT) THROW("invalid underscore"); REWIND(1);
            goto num_loop;
        }
        else if (IS('.')) {
            CONSUME(1); if (IS_NOT_IN_RANGE('0', '9')) THROW("invalid separator"); REWIND(1);
            goto num_loop;
        }

        YIELD_SLICE(Number, count, data);

        #define IS_VALID_IDENT (                                                                                       \
            IS_NOT_IN_RANGE(0, 31) && IS_NOT(127) && IS_NOT(' ') && IS_NOT('"') && IS_NOT('#') && IS_NOT('\'') &&      \
            IS_NOT('(') && IS_NOT(')') && IS_NOT(',') && IS_NOT('.') && IS_NOT('/') && IS_NOT(':') && IS_NOT(';') &&   \
            IS_NOT('@') && IS_NOT('[') && IS_NOT('\\') && IS_NOT(']') && IS_NOT('{') && IS_NOT('}')                    \
        )
    } else if (IS_VALID_IDENT) {
        const char * data = *source;
        u16 count = 0;

        do {
            CONSUME(1);
            count += 1;
        } while (IS_VALID_IDENT);

        YIELD_SLICE(Ident, count, data);
    } else {
        *source = end;
        THROW("unexpected data");
    }
}
