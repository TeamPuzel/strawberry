// Created by Lua (TeamPuzel) on September 4th 2025.
// Copyright (c) 2025 All rights reserved.
#ifndef STRING_H
#define STRING_H
// Strawberry has to resolve the mess of string encodings in a sensible way. The arbitrary string type
// represents the literal string as encoded by the source file, so it uses the source encoding.
// The design is unclear but ideally subtypes would perform compile validation before coercing from a literal string.

// An arbitrary literal string.
typedef struct String {

} String;

#endif
