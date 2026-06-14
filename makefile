# The Strawberry Programming Language Toolchain.
# Copyright (c) 2026 Lua (TeamPuzel)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

.PHONY: configure clangd build bootstrap test zed-extension compile-commands

BOOTSTRAP_COMPILER = $(STRAWBERRY_LLVM)/bin/clang++
BOOTSTRAP_INCLUDE  = $(STRAWBERRY_LLVM)/include/c++/v1
BOOTSTRAP_LIB      = $(STRAWBERRY_LLVM)/lib

BOOTSTRAP_SRC      = bootstrap/main.cpp
BOOTSTRAP_BIN      = build/strc

BOOTSTRAP_FLAGS = \
	-std=c++26 -Wall -g -Wunused -Wpedantic -Wno-logical-op-parentheses \
	-nostdlib++ -nostdinc++ -freflection -freflection-latest -fexpansion-statements \
	$(BOOTSTRAP_LIB)/libc++.a $(BOOTSTRAP_LIB)/libc++abi.a $(BOOTSTRAP_LIB)/libunwind.a \
	-isystem $(BOOTSTRAP_INCLUDE) \
	-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE \
	-ftrivial-auto-var-init=pattern -O3 -Wno-deprecated-declarations

ifeq ($(shell uname -s), Darwin)
	BOOTSTRAP_FLAGS += -isysroot $(shell xcrun --show-sdk-path)
endif

all: bootstrap

clangd:
	@echo "CompileFlags:" > .clangd
	@echo "  CompilationDatabase: build" >> .clangd

configure: clangd
	@mkdir -p build

build:
	@$(BOOTSTRAP_COMPILER) $(BOOTSTRAP_FLAGS) $(BOOTSTRAP_SRC) -o $(BOOTSTRAP_BIN)

bootstrap: build
	@$(BOOTSTRAP_BIN) run

test: build
	@$(BOOTSTRAP_BIN) test

define ZED_EXTENSION_TOML
id = "strawberry"
name = "Strawberry"
description = "🍓 Strawberry support."
version = "0.1.0"
schema_version = 1
authors = [
    "Lua (TeamPuzel) <puffed-01.blips@icloud.com>"
]

[language_servers.strc]
name = "strc"
language = "Strawberry"

[grammars.strawberry]
repository = "file://$(CURDIR)"
rev = "main"
path = "editor/tree-sitter-strawberry"
endef
export ZED_EXTENSION_TOML

zed-extension:
	@echo "$$ZED_EXTENSION_TOML" > editor/zed-strawberry/extension.toml

define COMPILE_COMMANDS
[{
    "directory": "$(dir $(abspath $(BOOTSTRAP_BIN)))",
    "command": "$(BOOTSTRAP_COMPILER) $(BOOTSTRAP_FLAGS) $(abspath $(BOOTSTRAP_SRC)) -o $(abspath $(BOOTSTRAP_BIN))",
    "file": "$(abspath $(BOOTSTRAP_SRC))",
    "output": "$(abspath $(BOOTSTRAP_BIN))"
}]
endef
export COMPILE_COMMANDS

compile-commands:
	@echo "$$COMPILE_COMMANDS" > build/compile_commands.json
