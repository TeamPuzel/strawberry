// The Strawberry Programming Language Toolchain.
// Copyright (c) 2026 Lua (TeamPuzel)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <ranges>
#include <print>
#include "strc.hpp"
#include "lsp.hpp"
#include "primitive.hpp"

i32 main(i32 argc, char** argv) {
    auto args = std::span(argv, argc)
        | std::views::drop(1)
        | std::views::transform([] (char* arg) { return std::string_view(arg); });

    if (args.size() > 0) {
        auto subcommand = args[0];

        if (subcommand == "run") {
            return str::run::main();
        } else if (subcommand == "test") {
            return str::test::main();
        } else if (subcommand == "serve") {
            return str::lsp::main();
        }
    }

    std::print(
        "Welcome to Strawberry!\n\n"

        "usage: strc [subcommand] [options]\n\n"

        "This is the stage 0 prototype bootstrap C++ compiler,\n"
        "it is only intended for bootstrapping the actual strc.\n\n"

        "subcommands:\n"
        "  run        Run the bootstrap compiler\n"
        "  test       Run unit tests on the bootstrap compiler\n"
        "  serve      Run the bootstrap language server\n\n"

        "options:\n"
        "  There are no options in the bootstrap compiler, it is hardcoded to its only purpose.\n"
    );

    return 0;
}
