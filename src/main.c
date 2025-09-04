// Created by Lua (TeamPuzel) on September 4th 2025.
// Copyright (c) 2025 All rights reserved.
#include "utility/primitive.h"
#include "parse/token.h"
#include "stdio.h"
// This is the standard unix interface of the strawberry compiler when used as an executable

// Writes the usage guide to standard output.
void print_help() {
    fprintf(stdout,
        "Usage: strawberry [command] [options...]?\n\n"

        "Strawberry is a programming language"

        "Commands:\n"
        "  init              Initialize a Strawberry project in the current directory\n"
        "  build             Build the project from build.str\n"
        "  run               Run the project natively\n"
        "  execute           Run the project with the interpreter\n"
        "  test              Run the project's tests\n"
        "  format            Format source code\n"
        "  version           Output the toolchain version\n"
        "  targets           Output a list of targets included in the build\n"
        "  help [command]?   Output this or command specific help text\n"
        "  commands          Output a list of currently available commands\n"
        "  [custom]          Any other commands declared by the build file if present\n\n"

        "Advanced commands:\n"
        "  tokenize          Output the tokens\n"
        "  parse             Output the parsed tree\n"
        "  ir                Output the strawberry intermediate representation, SIR\n"
    );
}

int main(int argc, char ** argv) {
    if (argc >= 2) {

    } else {
        print_help();
        return ERROR_CODE_INVALID_USE;
    }
}
