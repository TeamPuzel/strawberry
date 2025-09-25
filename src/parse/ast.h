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

typedef struct AstExpr {

} AstExpr;

typedef enum AstInlineModifier {
    AstInlineModifier_Unspecified,
    AstInlineModifier_Always,
    AstInlineModifier_Never
} AstInlineModifier;

typedef enum AstVisibilityModifier {
    AstVisibilityModifier_Unspecified,
    AstVisibilityModifier_Public,
    AstVisibilityModifier_ModulePublic,
    AstVisibilityModifier_Private,
    AstVisibilityModifier_Protected
} AstVisibilityModifier;

// A generic declaration parameter.
typedef struct AstGeneric {
    // Determines if the generic declares a constant parameter or type parameter
    bool is_constant;
    // Determines if a default expression is present.
    bool has_default_expr;
    // Determines if the parameter is exported as a constant or alias from the parameterized namespace.
    AstVisibilityModifier visibility;
    // The default expression if present.
    AstExpr default_expr;
    // For a constant parameter it determines the type.
    AstExpr type_expr;
    // For a constant parameter it determines the public argument label.
    const char * label;
    // Determines the inner and export names.
    const char * name;
} AstGeneric;

// A generic list specification associated with a declaration.
typedef struct AstGenericList {
    AstGeneric * parameters;
    size_t count;
} AstGenericList;

// A callable argument.
typedef struct AstArgument {
    // Determines if a default expression is present.
    bool has_default_expr;
    // The default expression if present.
    AstExpr default_expr;
    // Determines the type of the argument.
    AstExpr type_expr;
    // Determines the public argument label.
    const char * label;
    // Determines the argument binding name.
    const char * namel;
} AstArgument;

// An argument list associated with a callable signature.
typedef struct AstArgumentList {
    AstArgument * arguments;
    size_t count;
} AstArgumentList;

typedef struct AstNode AstNode;
struct AstNode {
    enum AstNodeTag {
        AstNode_Compound,
        AstNode_Function,
        AstNode_Type,
        AstNode_Structure,
        AstNode_Enumeration,
        AstNode_Category,
        AstNode_Class,
        AstNode_Binding,
        AstNode_Use,
        AstNode_Module,
        AstNode_Attribute
    } tag;
    union {
        struct {
            AstNode * children;
            u32 count;
        } compound;
        struct {
            const char * name;
            AstVisibilityModifier visibility;
            AstInlineModifier inlining;
            bool is_async;
        } function;
        struct {

        } type;
        struct {

        } structure;
        struct {
            const char * name;
            AstVisibilityModifier visibility;
            AstGenericList generics;
            AstExpr generic;
            bool is_open;
        } enumeration;
        struct {

        } category;
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
