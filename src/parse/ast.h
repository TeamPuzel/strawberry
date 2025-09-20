// Created by Lua (TeamPuzel) on September 4th 2025.
// Copyright (c) 2025 All rights reserved.
#ifndef AST_H
#define AST_H
#include "../utility/primitive.h"
#include "../utility/array.h"
#include "../utility/allocator.h"
#include "token.h"

// A bundle of a token and user-facing provenance information.
typedef struct TokenWithProvenance {
    // The token itself.
    Token token;
    // An identifier representing the origin of the token.
    const char * source_id;
    // The starting index of the token origin.
    u32 source_offset;
    // The unit count describing the origin span.
    u32 source_count;
} TokenWithProvenance;

// An interface for pulling in tokens as needed.
typedef struct TokenStream {
    TokenWithProvenance (*next)(void * self);
} TokenStream;

typedef struct AstExpression {
    int dummy;
} AstExpression;

typedef enum AstInlineModifier {
    AstInlineModifier_Unspecified,
    AstInlineModifier_Inline,
    AstInlineModifier_Never
} AstInlineModifier;

typedef enum AstVisibilityModifier {
    AstVisibilityModifier_Unspecified,
    AstVisibilityModifier_Public
} AstVisibilityModifier;

typedef enum AstNodeTag {
    AstNode_Compound,
    AstNode_Function,
    AstNode_Type,
    AstNode_Structure,
    AstNode_Enumeration,
    AstNode_Trait,
    AstNode_Class,
    AstNode_Binding,
    AstNode_Use,
    AstNode_Module,
    AstNode_Attribute
} AstNodeTag;

typedef struct AstNode AstNode;
struct AstNode {
    AstNodeTag tag;
    union AstNodeData {
        struct {
            AstNode * children;
            u32 count;
        } compound;
        struct {
            const char * name;
            AstVisibilityModifier visibility;
            AstInlineModifier inlining;
        } function;
        struct {

        } type;
        struct {

        } structure;
        struct {

        } enumeration;
        struct {

        } trait;
        struct {

        } class;
        struct {

        } binding;
        struct {

        } use;
        struct {

        } module;
        struct {

        } attribute;
    } data;
};

// The top level structure containing a syntax tree.
typedef struct Ast {
    AstNode * root;
} Ast;

// The standard parsing procedure. It operates on a stream of tokens, assembling the tree and pulling in new tokens
// as needed. Once tokens run out the tree is returned. It's eager because the way the language is designed there
// is little value in making this a coroutine.
//
// The allocator is used to allocate nodes of the tree. They can be freed individually but the recommended
// approach is to use arenas.
Ast parse_syntax_tree(Allocator * alloc, TokenStream * tokens);

#endif
