// Created by Lua (TeamPuzel) on September 4th 2025.
// Copyright (c) 2025 All rights reserved.
#include "ast.h"

static inline bool string_slice_to_literal_eq(
    u16 count, const char * data, u16 literal_count, const char * pattern_data
) {
    if (count != literal_count) return false;

    for (size_t i = 0; i < count; i += 1) {
        if (data[i] != pattern_data[i]) return false;
    }

    return true;
}

#define TOKEN_DATA_EQ(TOKEN, STR) string_slice_to_literal_eq(TOKEN.count, TOKEN.data, sizeof(STR) - 1, STR)

typedef struct ParseContext {
    Allocator * alloc;
    TokenStream * tokens;
    Ast ast;
    AstParseDiagnosticArray diagnostics;
    TokenWithProvenanceArray buffer;
} ParseContext;

static void parse_expr(ParseContext * ctx);
static void parse_module(ParseContext * ctx);
static void parse_use(ParseContext * ctx);

#define YIELD return (AstParseResult) { .ast = context.ast, .diagnostics = context.diagnostics }

#define NEXT TokenStream_next(ctx->tokens)
#define PULL TokenWithProvenanceArray_append(&ctx->buffer, ctx->alloc, TokenStream_next(ctx->tokens))
#define CLEAR TokenWithProvenanceArray_clear(&ctx->buffer)
#define AT(INDEX) ctx->buffer.data[INDEX]

#define DIAGNOSTIC_RANGE(SEVERITY, FROM, TO, MESSAGE)                                                                  \
AstParseDiagnosticArray_append(&ctx->diagnostics, ctx->alloc, (AstParseDiagnostic) {                                   \
    .severity = AstParseDiagnosticSeverity_##SEVERITY, .first_token = FROM, .last_token = TO, .message = MESSAGE       \
})
#define DIAGNOSTIC(SEVERITY, TOKEN, MESSAGE) DIAGNOSTIC_RANGE(SEVERITY, TOKEN, TOKEN, MESSAGE)

#define DIAGNOSTIC_LAST_TOKEN_UNEXPECTED DIAGNOSTIC(Error, AT(ctx->diagnostics.count), "Unexpected token")

AstParseResult parse_syntax_tree(Allocator * alloc, TokenStream * tokens) {
    ParseContext context = {
        .alloc = alloc,
        .tokens = tokens,
        .ast = {
            .decls = { 0 }
        },
        .diagnostics = { 0 }
    };
    ParseContext * ctx = &context;

    while (true) {
        PULL;
        switch (AT(0).token.tag) {
            case Token_LineEnd:
                CLEAR; continue;

            case Token_Whitespace:
                CLEAR; continue;

            case Token_Comment:
                CLEAR; continue;

            // case Token_DocumentationComment:
            // case Token_At:
            case Token_Ident:
                if (TOKEN_DATA_EQ(AT(0).token, "module")) {
                    AstDeclArray_append(&ctx->ast.decls, ctx->alloc, (AstDecl) {
                        .tag = AstDecl_Module, .data = { .module = {
                            .name = "null"
                        } }
                    });
                } else if (TOKEN_DATA_EQ(AT(0).token, "use")) {

                }
                CLEAR; continue;

            case Token_EndOfSource:
                goto end;

            default:
                DIAGNOSTIC_LAST_TOKEN_UNEXPECTED; YIELD;
        }
    }

  end:
    TokenWithProvenanceArray_free(&ctx->buffer, ctx->alloc);
    YIELD;
}
