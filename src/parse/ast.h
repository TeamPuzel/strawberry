// Created by Lua (TeamPuzel) on September 4th 2025.
// Copyright (c) 2025 All rights reserved.
#ifndef AST_H
#define AST_H

typedef enum DeclTag : int {
    Decl_Struct,
    Decl_Enum,
    Decl_Fun,
} DeclTag;

typedef struct Decl {
    DeclTag tag;
} Decl;

#endif
