// Created by Lua (TeamPuzel) on September 4th 2025.
// Copyright (c) 2025 All rights reserved.
#ifndef TOKEN_H
#define TOKEN_H
#include "../utility/primitive.h"

// The enumeration of all token types for tagging a union of their associated data.
// All tokens are defined very elegantly except for the opening angle bracket as technically it's a
// contextual use of the same token otherwise allowed as an operator.
typedef enum TokenTag : i16 {
    // A special tag indicating that an error occured during
    Token_Error = -1,
    // A special tag indicating that there is nothing left to tokenize.
    Token_EndOfSource,
    // Whitespace and its length, included as a token because the language has prefix, suffix and infix operators
    // resolved with it. The language is also very strict and often uses this information to reject or warn about
    // style mistakes like weird or uneven spacing, or multiple spaces in an argument list.
    // They language generally allows breaking things up into multiple lines in a consistent way but guards
    // it heavily so as not to support non standard code. Strawberry code is meant to look consistent,
    // no matter who writes it, no opening scopes on a separate line wasting vertical space, deal with it.
    // Tabs can't be used to align things on uneven columns so they are not considered whitespace.
    // Tabs have no token of their own because they are an error.
    Token_Whitespace,
    // This token represents the end of a line, mainly used to infer statements and avoid semicolons.
    Token_LineEnd,
    // A left parenthesis.
    Token_LeftParen,
    // A right parenthesis.
    Token_RightParen,
    // A left curly brace.
    Token_LeftBrace,
    // A right curly brace.
    Token_RightBrace,
    // A left square bracket.
    Token_LeftBracket,
    // A right square bracket.
    Token_RightBracket,
    // A left angle bracket.
    // This is resolved uniquely. A leading angle bracket directly after another construct
    // For this reason sequences of the left angle bracket are reserved in suffix operator context.
    // A solution to make this cleaner would be to use square brackets for generics like some newer languages
    // but the issue with that is that you'd want types to be able to provide subscripts statically. I see no
    // reason clean code would use suffix brackets like that anyway so for consistency and avoidance of ambiguity
    // all types of brackets are disallowed in suffix and prefix operators, and only allowed in
    Token_LeftAngle,
    // A right angle bracket.
    Token_RightAngle,
    // A comma of any kind.
    Token_Comma,
    // A dot of any kind.
    Token_Dot,
    // A colon of any kind.
    Token_Colon,
    // The semicolon is not used much and is generally bad practice but it is included as a way
    // of combining multiple statements on a single line, so it is reserved as its own token.
    Token_Semicolon,
    // Any sequence of symbols which does not fall into other token categories.
    // Notably this includes keywords, because keywords are not reserved, they only take precedence contextually.
    // It is up to the syntax tree assembly to resolve keywords with least possible pollution.
    // This way the language can have many and intuitively named keywords without annoying the user.
    Token_Ident,
    // A comment is anything from a double slash until the line ends. There are no block comments.
    // Comments on consecutive lines are tokenized individually and it is up to the syntax tree assembly
    // to resolve consecutive comments. This matters for documentation comments.
    Token_Comment,
    // A documentation comment is a comment with three slashes. They can only be attached to a declaration
    // and there can be no empty lines between them and the declaration.
    Token_DocumentationComment,
    // A string is a source span between two double quotes. The parser resolves backslash-doublequote escapes.
    Token_String,
    // A multiline string of two back-slashes. They are tokenized individually and it is up to the syntax tree assembly
    // to resolve consecutive multiline string tokens of even indentation into a single literal, and the semantics
    // are a match for comments.
    Token_MultilineString,
    // A character is a source span between two single quotes. It can be many characters in the source
    // and it is up to the concrete type we lower into to validate it correctly.
    Token_Character,
    // A number of yet undetermined syntax, it starts when a leading digit is found after whitespace
    // and continues while we keep finding digits. A special case is made for dots and underscores where
    // they are included as long as a number follows.
    // This is especially notable for integers since we want to be able to use the dot method syntax on literals
    // so this method resolves the potential ambiguity.
    // Characters are also allowed within the number itself but not directly after a dot for method ambiguity reasons.
    Token_Number
} TokenTag;

// The token struct.
// Tokens are very simple and unambiguous, and so is the language grammar built from them.
//
// The lifetime of tokens is tied to the source they refer to because they can sometimes include
// slices of the source code.
//
// It's not the smallest thing ever but these are generally consumed in a chain of iterators.
// Generally by virtue of C ABI rules this type will be the size of two pointers
// making it fit in two registers on modern architectures like arm64.
// It does not include provenance information as that depends on the environment, and the caller
// can take care of that themselves.
//
// The layout is somewhat complicated as it reuses fields rather than being a union. Unlike sensible languages
// like Swift, C struct layout does not take advantage of tail padding. This way the layout is efficient,
// currently optimizing for 32 and 64 bit pointer sizes, where the Token will be the size of two pointers.
// This limits the slices to 16 bit counts but that is not an issue. Strawberry is not intended to be
// generated, only written by hand. The language is supposed to be expressive enough not to need generated code.
// Some configuration could be added here in the future to allow more capability.
typedef struct Token {
    // The union tag of the token.
    TokenTag tag;
    // The count of the data slice for Ident, String, Character, Integer and Decimal.
    // For Whitespace it instead indicates the count of whitespace it describes.
    u16 count;
    // The data slice referring to the source. Only initialized for Ident, String, Character, Integer and Decimal.
    // For Error it points to a terminated static string of the error reason.
    const char * data;
} Token;

// The standard tokenizing procedure. The input is a string with the precondition that it does
// not end in the middle of a token. How the caller ensures this is not important but not doing so
// could result in incorrect results. The end pointer refers to the byte past the end of the string.
// It does not need to be a terminated cstring so the end pointer does not need to be valid
// and will never be dereferenced in the implementation.
//
// The source pointer will be advanced forward to point to the first byte after the resolved token.
//
// Returned tokens which contain data are pointing back into the source string, so if the caller is
// not tokenizing with the entire source in memory but instead streaming the source, it is important that
// the data is allocated differently. Arenas are a recommended allocation strategy, allowing one to drop
// all tokens at the same time.
Token tokenize(const char ** source, const char * end);

#endif
