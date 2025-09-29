// Created by Lua (TeamPuzel) on September 4th 2025.
// Copyright (c) 2025 All rights reserved.
#include "utility/primitive.h"
#include "parse/token.h"
#include "parse/ast.h"
#include "utility/allocator.h"
#include <stdio.h>
// This is the reference unix interface of the strawberry compiler when used as an executable.

// Configuration -------------------------------------------------------------------------------------------------------

// IO layer ------------------------------------------------------------------------------------------------------------
// IO will go through this layer to have one place to change when porting the code, since some
// platforms still have these concepts just with a different API and it would be silly to rewrite all of it for them.

static inline void * c_alloc_alloc(void * self, size_t size) {
    return malloc(size);
}

static inline void * c_alloc_realloc(void * self, void * ptr, size_t size) {
    return realloc(ptr, size);
}

static inline void c_alloc_free(void * self, void * ptr) {
    free(ptr);
}

static inline Allocator * c_alloc() {
    static Allocator alloc = {
        .alloc = c_alloc_alloc,
        .realloc = c_alloc_realloc,
        .free = c_alloc_free
    };
    return &alloc;
}

void panic(const char * reason) {
    fprintf(stderr, "panic: %s", reason);
    exit(ERROR_CODE_PANIC);
}

// This is aliased because we will eventually need some conditional compilation to support unconventional hosts.
typedef FILE * File;

File open_source_file(const char * path) {
    return fopen(path, "r");
}

void close_source_file(File file) {
    fclose(file);
}

typedef struct Source {
    char * start;
    char * end;
} Source;

// Reads the entire file into memory as an allocated cstring.
// This is a temporary measure and eventually this should be a choice. On modern systems this is
// the optimal choice as memory is abundant, but on very old hardware it would be better to stream the data.
Source read_source_file_to_end(Allocator * alloc, File file) {
    if (fseek(file, 0, SEEK_END) != 0) return (Source) { 0 };
    long size = ftell(file);
    if (size < 0) return (Source) { 0 };

    rewind(file);

    char * buffer = Allocator_alloc(alloc, (size_t) size + 1);
    if (!buffer) return (Source) { 0 };

    size_t read = fread(buffer, 1, (size_t)size, file);
    buffer[read] = '\0';

    return (Source) {
        .start = buffer,
        .end = buffer + read
    };
}

void print_token(File file, Token token) {
    switch (token.tag) {
        case Token_Error:
            fprintf(file, "Error(%s)\n", token.data); break;
        case Token_EndOfSource:
            fprintf(file, "EndOfSource\n"); break;
        case Token_Whitespace:
            fprintf(file, "Whitespace(%hu)\n", token.count); break;
        case Token_LineEnd:
            fprintf(file, "LineEnd\n"); break;
        case Token_LeftParen:
            fprintf(file, "LeftParen\n"); break;
        case Token_RightParen:
            fprintf(file, "RightParen\n"); break;
        case Token_LeftBrace:
            fprintf(file, "LeftBrace\n"); break;
        case Token_RightBrace:
            fprintf(file, "RightBrace\n"); break;
        case Token_LeftBracket:
            fprintf(file, "LeftBracket\n"); break;
        case Token_RightBracket:
            fprintf(file, "RightBracket\n"); break;
        case Token_Comma:
            fprintf(file, "Comma\n"); break;
        case Token_Dot:
            fprintf(file, "Dot\n"); break;
        case Token_Colon:
            fprintf(file, "Colon\n"); break;
        case Token_Semicolon:
            fprintf(file, "Semicolon\n"); break;
        case Token_At:
            fprintf(file, "At\n"); break;
        case Token_Pound:
            fprintf(file, "Pound\n"); break;
        case Token_Ident:
            fprintf(file, "Ident(%.*s)\n", token.count, token.data); break;
        case Token_Comment:
            fprintf(file, "Comment(%.*s)\n", token.count, token.data); break;
        case Token_DocumentationComment:
            fprintf(file, "DocumentationComment(%.*s)\n", token.count, token.data); break;
        case Token_String:
            fprintf(file, "String(%.*s)\n", token.count, token.data); break;
        case Token_MultilineString:
            fprintf(file, "MultilineString(%.*s)\n", token.count, token.data); break;
        case Token_Character:
            fprintf(file, "Character(%.*s)\n", token.count, token.data); break;
        case Token_Number:
            fprintf(file, "Number(%.*s)\n", token.count, token.data); break;
    }
}

void print_ast(File file, Ast ast) {
    for (size_t i = 0; i < ast.decls.count; i += 1) {
        AstDecl * decl = &ast.decls.data[i];

        switch (decl->tag) {
            case AstDecl_Module:
                fprintf(file, "Module { name: %s }\n", decl->data.module.name); break;
            case AstDecl_Use:
                fprintf(file, "Use { name: %s, as: %s }\n",
                    decl->data.use.name, decl->data.use.as ? decl->data.use.as : "null"
                ); break;
            case AstDecl_Structure:
                fprintf(file, "Function {}\n"); break;
            case AstDecl_Enumeration:
                fprintf(file, "Function {}\n"); break;
            case AstDecl_Category:
                fprintf(file, "Function {}\n"); break;
            case AstDecl_Extension:
                fprintf(file, "Function {}\n"); break;
            case AstDecl_Class:
                fprintf(file, "Function {}\n"); break;
            case AstDecl_Constant:
                fprintf(file, "Function {}\n"); break;
            case AstDecl_StaticValue:
                fprintf(file, "Function {}\n"); break;
            case AstDecl_Function:
                fprintf(file, "Function {}\n"); break;
            case AstDecl_TypeAlias:
                fprintf(file, "Function {}\n"); break;
            case AstDecl_CategoryAlias:
                fprintf(file, "Function {}\n"); break;
          break;
        }
    }
}

void print_parser_diagnostics(File file, Source source, AstParseDiagnosticArray diagnostics) {
    for (size_t i = 0; i < diagnostics.count; i += 1) {
        AstParseDiagnostic * diagnostic = &diagnostics.data[i];

        const char * severity_string;
        const char * severity_color;
        switch (diagnostic->severity) {
            case AstParseDiagnosticSeverity_Note:
                severity_string = "note";
                severity_color  = "\033[34m"; // Blue.
                break;
            case AstParseDiagnosticSeverity_Warning:
                severity_string = "warning";
                severity_color  = "\033[33m"; // Yellow.
                break;
            case AstParseDiagnosticSeverity_Error:
                severity_string = "error";
                severity_color  = "\033[31m"; // Red.
                break;
        }

        fprintf(file, "%s:%u:%u: %s%s\033[0m: %s\n\n",
            diagnostic->first_token.source_id,
            diagnostic->first_token.line,
            diagnostic->first_token.span_start,
            severity_color,
            severity_string,
            diagnostic->message
        );
    }
}

// Token stream --------------------------------------------------------------------------------------------------------

// A simple token stream which holds an entire source unit in memory.
typedef struct InMemoryTokenStream { TokenStream interface;
    // The source file span.
    Source source;
    // The unique identifier of the source.
    const char * source_id;
    // The cursor where the next poll will start.
    const char * cursor;
    // The line the cursor is currently within.
    u32 line;
} InMemoryTokenStream;

TokenWithProvenance in_memory_token_stream_next(void * _self) {
    InMemoryTokenStream * self = _self;

    const char * previous_cursor = self->cursor;
    Token token = tokenize(&self->cursor, self->source.end);

    TokenWithProvenance result = {
        .token = token,
        .source_id = self->source_id,
        .line = self->line,
        .span_start = previous_cursor - self->source.start,
        .span_count = self->cursor - previous_cursor
    };

    if (token.tag == Token_LineEnd) self->line += 1;

    return result;
}

InMemoryTokenStream in_memory_token_stream(Source source, const char * source_id) {
    return (InMemoryTokenStream) {
        .interface = {
            .next = in_memory_token_stream_next
        },
        .source = source,
        .source_id = source_id,
        .cursor = source.start,
        .line = 1
    };
}

// Command interface ---------------------------------------------------------------------------------------------------
// Constants and arguments are defined here

#define VERSION_STRING "0.0.1 untagged reference"

typedef enum OptimizationMode : u8 {
    OptimizationMode_None,
    OptimizationMode_Size,
    OptimizationMode_Fast
} OptimizationMode;

// Arguments are stored globally here.
static struct Args {
    const char * command;

    const char * source_path;
    const char * output_path;

    OptimizationMode optimization_mode;
} args = {
    .source_path = "main.str",
    .output_path = "out",
    .optimization_mode = OptimizationMode_None
};

typedef struct Args Args;

// Writes the usage guide to standard output.
void print_help(void) {
    fprintf(stdout,
        "Usage: strawberry [command] [options...]? [root]?\n\n"

        "Strawberry is a meta programming language focused on\n"
        "extreme generic programming without using any runtime primitives\n\n"

        "Commands:\n"
        "  init              Initialize a Strawberry project in the current directory\n"
        "  build             Build the project from build.str\n"
        "  run               Run the project natively\n"
        "  execute           Run the project with the interpreter\n"
        "  test              Run the project's tests\n"
        "  format            Format source code\n"
        "  lsp               Run the language server\n"
        "  version           Output the toolchain version\n"
        "  targets           Output a list of targets included in the build\n"
        "  help [command]?   Output this or command specific help text\n"
        "  commands          Output a list of currently available custom commands\n"
        "  [custom]          Any other commands declared by the build file if present\n\n"

        "Debug commands:\n"
        "  tokenize          Output the tokens\n"
        "  parse             Output the parsed tree\n"
        "  ir                Output the strawberry intermediate representation, SIR\n"
    );
}

int main(int argc, char ** argv) {
    Allocator * alloc = c_alloc();

    if (argc >= 2) {
        args.command = argv[1];

        if (string_eq(args.command, "init")) {
            fprintf(stdout, "unimplemented");
        } else if (string_eq(args.command, "build")) {
            fprintf(stdout, "unimplemented");
        } else if (string_eq(args.command, "run")) {
            fprintf(stdout, "unimplemented");
        } else if (string_eq(args.command, "execute")) {
            fprintf(stdout, "unimplemented");
        } else if (string_eq(args.command, "test")) {
            fprintf(stdout, "unimplemented");
        } else if (string_eq(args.command, "format")) {
            fprintf(stdout, "unimplemented");
        } else if (string_eq(args.command, "version")) {
            fprintf(stdout, VERSION_STRING "\n");
        } else if (string_eq(args.command, "targets")) {
            fprintf(stdout, "unimplemented");
        } else if (string_eq(args.command, "help")) {
            fprintf(stdout, "unimplemented");
        } else if (string_eq(args.command, "commands")) {
            fprintf(stdout, "unimplemented");
        } else if (string_eq(args.command, "tokenize")) {
            File entry = open_source_file(args.source_path);
            Source source = read_source_file_to_end(alloc, entry);
            close_source_file(entry);

            const char * cursor = source.start;

            while (true) {
                Token token = tokenize(&cursor, source.end);
                print_token(stdout, token);
                if (token.tag == Token_EndOfSource) break;
            }

            Allocator_free(&alloc, source.start);
        } else if (string_eq(args.command, "parse")) {
            File entry = open_source_file(args.source_path);
            Source source = read_source_file_to_end(alloc, entry);
            close_source_file(entry);

            InMemoryTokenStream stream = in_memory_token_stream(source, args.source_path);
            AstParseResult ast_result = parse_syntax_tree(alloc, (TokenStream *) &stream);

            print_parser_diagnostics(stderr, source, ast_result.diagnostics);
            print_ast(stdout, ast_result.ast);

            Allocator_free(&alloc, source.start);
        } else if (string_eq(args.command, "ir")) {
            fprintf(stdout, "unimplemented");
        }

        return ERROR_CODE_SUCCESS;
    } else {
        print_help();
        return ERROR_CODE_INVALID_USE;
    }
}
