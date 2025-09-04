// Created by Lua (TeamPuzel) on September 4th 2025.
// Copyright (c) 2025 All rights reserved.
#ifndef TOKEN_H
#define TOKEN_H

typedef enum TokenTag {
    Token_LeftParen,
    Token_RightParen,
    Token_LeftBrace,
    Token_RightBrace,
    Token_LeftAngle,
    Token_RightAngle,
    Token_Comma,
    Token_Dot,
    Token_Colon,
    Token_Semicolon,
    Token_Ident,
    Token_String,
    Token_Character,
    Token_Integer,
    Token_Decimal
} TokenTag;

typedef struct Token {
    TokenTag tag;
    union {
        char * ident;
        char * string;
        char * character;
        char * integer;
        char * decimal;
    };
} Token;

Token consume_token(char ** source);

#endif
