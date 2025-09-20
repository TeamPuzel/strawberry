// Created by Lua (TeamPuzel) on September 4th 2025.
// Copyright (c) 2025 All rights reserved.
#include "ast.h"

#define NEW_NODE Allocator_alloc(alloc, sizeof(AstNode))
#define PULL buf[buf_count] = tokens->next(tokens); buf_count += 1
#define CLEAR buf_count = 0

Ast parse_syntax_tree(Allocator * alloc, TokenStream * tokens) {
    TokenWithProvenance buf[32];
    size_t buf_count = 0;

    AstNode * root = NEW_NODE;
    root->tag = AstNode_Compound;

    PULL;

    return (Ast) { .root = root };
}
