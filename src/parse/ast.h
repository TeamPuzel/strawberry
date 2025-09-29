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
    // A unique identifier representing the source unit of the token.
    const char * source_id;
    // The line number in the source unit.
    u32 line;
    // The start of the span within the source unit.
    u32 span_start;
    // The count of characters in the span.
    u32 span_count;
} TokenWithProvenance;
DEFINE_ARRAY(TokenWithProvenance)

// An interface for pulling in tokens as needed.
typedef struct TokenStream {
    TokenWithProvenance (*next)(void * self);
} TokenStream;

static inline TokenWithProvenance TokenStream_next(void * self) {
    return ((TokenStream *) self)->next(self);
}

typedef struct AstDecl AstDecl;
DECLARE_ARRAY(AstDecl)
struct AstDecl {
    enum AstDeclTag {
        AstDecl_Module,
        AstDecl_Use,
        AstDecl_Structure,
        AstDecl_Enumeration,
        AstDecl_Category,
        AstDecl_Extension,
        AstDecl_Class,
        AstDecl_Constant,
        AstDecl_StaticValue,
        AstDecl_Function,
        AstDecl_TypeAlias,
        AstDecl_CategoryAlias
    } tag;
    union {
        struct {
            // The name of the declared module.
            const char * name;
        } module;
        struct {
            // The name of the imported declaration.
            const char * name;
            // If present, renames the imported declaration.
            const char * as;
        } use;
        struct {

        } structure;
        struct {

        } enumeration;
        struct {

        } category;
        struct {

        } extension;
        struct {

        } class;
        struct {

        } constant;
        struct {

        } static_value;
        struct {

        } function;
        struct {

        } type_alias;
        struct {

        } category_alias;
    } data;
};
IMPLEMENT_ARRAY(AstDecl)

// The top level structure containing a syntax tree.
typedef struct Ast {
    AstDeclArray decls;
} Ast;

typedef struct AstParseDiagnostic {
    enum AstParseDiagnosticSeverity {
        AstParseDiagnosticSeverity_Note,
        AstParseDiagnosticSeverity_Warning,
        AstParseDiagnosticSeverity_Error
    } severity;
    // Initial token of the diagnostic span.
    TokenWithProvenance first_token;
    // Final token of the diagnostic span.
    TokenWithProvenance last_token;
    // The diagnostic message.
    const char * message;
} AstParseDiagnostic;
DEFINE_ARRAY(AstParseDiagnostic)

typedef struct AstParseResult {
    Ast ast;
    AstParseDiagnosticArray diagnostics;
} AstParseResult;

// The standard parsing procedure. It operates on a stream of tokens, assembling the tree and pulling in new tokens
// as needed. Once tokens run out the tree is returned. It's eager because the way the language is designed there
// is little value in making this a coroutine.
//
// The allocator is used to allocate nodes of the tree. They can be freed individually but the recommended
// approach is to use arenas.
AstParseResult parse_syntax_tree(Allocator * alloc, TokenStream * tokens);

#endif
