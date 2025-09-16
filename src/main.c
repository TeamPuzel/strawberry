// Created by Lua (TeamPuzel) on September 4th 2025.
// Copyright (c) 2025 All rights reserved.
#include "utility/primitive.h"
#include "parse/token.h"
#include "utility/allocator.h"
#include "stdio.h"
#include <stdio.h>
// This is the reference unix interface of the strawberry compiler when used as an executable.

// Configuration -------------------------------------------------------------------------------------------------------

// IO layer ------------------------------------------------------------------------------------------------------------
// IO will go through this layer to have one place to change when porting the code, since some
// platforms still have these concepts just with a different API and it would be silly to rewrite all of it for them.

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
        case Token_LeftAngle:
            fprintf(file, "LeftAngle\n"); break;
        case Token_RightAngle:
            fprintf(file, "RightAngle\n"); break;
        case Token_Comma:
            fprintf(file, "Comma\n"); break;
        case Token_Dot:
            fprintf(file, "Dot\n"); break;
        case Token_Colon:
            fprintf(file, "Colon\n"); break;
        case Token_Semicolon:
            fprintf(file, "Semicolon\n"); break;
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

// Command interface ---------------------------------------------------------------------------------------------------
// Constants and arguments are defined here

#define VERSION_STRING "0.0.1 untagged reference"

// Arguments are stored globally here.
static struct Args {
    const char * command;

    const char * source_path;
    const char * output_path;
} args = {
    .source_path = "main.str",
    .output_path = "out"
};

typedef struct Args Args;

// Writes the usage guide to standard output.
void print_help() {
    // TODO(?, optimization): Keep these snippets of text interface as one big string in the binary
    // and write by slicing from it, to avoid duplication of strings.
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

        "Advanced commands:\n"
        "  tokenize          Output the tokens\n"
        "  parse             Output the parsed tree\n"
        "  ir                Output the strawberry intermediate representation, SIR\n"
    );
}

int main(int argc, char ** argv) {
    Allocator alloc = c_alloc();

    if (argc >= 2) {
        args.command = argv[1];

        if (string_eq(args.command, "init")) {

        } else if (string_eq(args.command, "build")) {

        } else if (string_eq(args.command, "run")) {

        } else if (string_eq(args.command, "execute")) {

        } else if (string_eq(args.command, "test")) {

        } else if (string_eq(args.command, "format")) {

        } else if (string_eq(args.command, "version")) {
            printf(VERSION_STRING "\n");
        } else if (string_eq(args.command, "targets")) {

        } else if (string_eq(args.command, "help")) {

        } else if (string_eq(args.command, "commands")) {

        } else if (string_eq(args.command, "tokenize")) {
            File entry = open_source_file(args.source_path);
            Source source = read_source_file_to_end(&alloc, entry);
            close_source_file(entry);

            const char * cursor = source.start;

            while (true) {
                Token token = tokenize(&cursor, source.end);
                print_token(stdout, token);
                if (token.tag == Token_EndOfSource) break;
            }

            Allocator_free(&alloc, source.start);
        } else if (string_eq(args.command, "parse")) {

        } else if (string_eq(args.command, "ir")) {

        }
    } else {
        print_help();
        return ERROR_CODE_INVALID_USE;
    }
    return ERROR_CODE_SUCCESS;
}
