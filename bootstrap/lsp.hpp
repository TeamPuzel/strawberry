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

#pragma once

#include <iostream>
#include <print>
#include "strc.hpp"
#include "coding.hpp"
#include "primitive.hpp"

namespace str::lsp {
    struct Position final {
        u32 line = 0;
        u32 character = 0;
    };

    struct Range final {
        Position start;
        Position end;
    };

    struct Diagnostic final {
        enum Severity : u32 {
            Error = 1,
            Warning = 2,
            Info = 3
        };

        u32 severity;
        std::string message;
        Range range;
    };

    struct PublishDiagnosticsParams final {
        std::string uri;
        std::vector<Diagnostic> diagnostics;
    };

    struct PublishDiagnosticsNotification final {
        std::string jsonrpc = "2.0";
        std::string method = "textDocument/publishDiagnostics";
        PublishDiagnosticsParams params;
    };

    struct TextDocumentSyncOptions final {
        [[=str::coding::rename("openClose")]]
        bool open_close = true;
        u32 change = 1;
        bool save = true;
    };

    struct InitResult final {
        struct Capabilities final {
            [[=str::coding::rename("textDocumentSync")]]
            TextDocumentSyncOptions text_document_sync;
        } capabilities;
    };

    struct InitializeResponse final {
        std::string jsonrpc = "2.0";
        u32 id = 0;
        InitResult result;
    };

    struct BaseMessage final {
        std::string method;
        u32 id = 0;
    };

    struct DidOpenParams final {
        struct Document final {
            std::string uri;
            std::string text;
        };

        [[=str::coding::rename("textDocument")]]
        Document text_document;
    };

    struct DidOpenNotification final {
        DidOpenParams params;
    };

    struct DidSaveNotification final {
        struct Params final {
            struct Document final {
                std::string uri;
            };

            [[=str::coding::rename("textDocument")]]
            Document text_document;
        } params;
    };

    struct ShutdownResponse final {
        std::string jsonrpc = "2.0";
        u32 id = 0;
        std::optional<std::monostate> result = std::nullopt;
    };

    inline void send(std::string_view message) {
        std::cerr << "[strc] sending: " << message << "\n";

        std::print(stdout, "Content-Length: {}\r\n\r\n{}", message.size(), message);
        std::fflush(stdout);
    }

    inline auto to_lsp_diagnostic(str::Diagnostic const& diagnostic) -> Diagnostic {
        Diagnostic lsp_diagnostic;

        switch (diagnostic.severity) {
            case str::Diagnostic::Severity::Error:   lsp_diagnostic.severity = Diagnostic::Severity::Error;   break;
            case str::Diagnostic::Severity::Warning: lsp_diagnostic.severity = Diagnostic::Severity::Warning; break;
            case str::Diagnostic::Severity::Info:    lsp_diagnostic.severity = Diagnostic::Severity::Info;    break;
            case str::Diagnostic::Severity::Runtime: lsp_diagnostic.severity = Diagnostic::Severity::Info;    break;
        }

        lsp_diagnostic.message = diagnostic.reason;

        std::visit(overloaded {
            [&] (str::Provenance::Span const& span) {
                lsp_diagnostic.range.start.line = span.start.line > 0 ? span.start.line - 1 : 0;

                lsp_diagnostic.range.start.character =
                    (span.start.column >= span.start.count)
                        ? span.start.column - span.start.count
                        : 0;

                lsp_diagnostic.range.end.line = span.end.line > 0 ? span.end.line - 1 : 0;

                lsp_diagnostic.range.end.character = span.end.column;
            },
            [&] (str::Provenance::Source const& source) {
                lsp_diagnostic.range.start.line = 0;
                lsp_diagnostic.range.start.character = 0;
                lsp_diagnostic.range.end.line = 0;
                lsp_diagnostic.range.end.character = 0;
            }
        }, diagnostic.provenance.data);

        return lsp_diagnostic;
    }

    inline void publish_diagnostics(
        std::string_view uri,
        std::span<const str::Diagnostic> diagnostics,
        str::coding::Json& json
    ) {
        auto source_units = str::run::collect_all_source_units();

        PublishDiagnosticsNotification notification;
        notification.params.uri = std::string(uri);

        for (auto const& diagnostic : diagnostics) {
            std::string_view diagnostic_source;

            if (std::holds_alternative<str::Provenance::Span>(diagnostic.provenance.data)) {
                diagnostic_source = std::get<str::Provenance::Span>(diagnostic.provenance.data).start.source;
            } else {
                diagnostic_source = std::get<str::Provenance::Source>(diagnostic.provenance.data).source;
            }

            if (uri.ends_with(diagnostic_source) or diagnostic_source.ends_with(uri)) {
                notification.params.diagnostics.push_back(to_lsp_diagnostic(diagnostic));
            }
        }

        send(json.encode(notification));
    }

    /// The entry point for the serve subcommand.
    inline i32 main() {
        std::cin.tie(nullptr);
        std::ios_base::sync_with_stdio(false);

        str::coding::Json json;

        while (true) {
            try {
                std::string line; std::getline(std::cin, line);
                if (not std::cin) break;

                if (line.starts_with("Content-Length: ")) {
                    u32 length = std::stoul(line.substr(16));

                    while (std::getline(std::cin, line)) {
                        if (line.empty() or line == "\r") break;
                    }

                    std::string content(length, ' ');
                    std::cin.read(content.data(), length);

                    std::cerr << "[strc] receiving: " << content << std::endl;
                    auto base = json.decode<BaseMessage>(content);
                    std::cerr << "[strc] decoded as: " << json.encode(base) << std::endl;

                    if (base.method == "initialize") {
                        InitializeResponse response;
                        response.id = base.id;
                        send(json.encode(response));
                    } else if (base.method == "textDocument/didOpen") {
                        auto request = json.decode<DidOpenNotification>(content);
                        std::string_view uri = request.params.text_document.uri;

                        auto source_units = str::run::collect_all_source_units();
                        auto modules = str::run::parse_modules(source_units);

                        if (not modules) {
                            publish_diagnostics(uri, modules.error(), json);
                        } else {
                            auto sir = str::evaluate(std::move(modules.value()), std::move(source_units));
                            publish_diagnostics(uri, sir.get_diagnostics(), json);
                        }
                    } else if (base.method == "textDocument/didSave") {
                        auto request = json.decode<DidSaveNotification>(content);
                        std::string_view uri = request.params.text_document.uri;

                        auto source_units = str::run::collect_all_source_units();
                        auto modules = str::run::parse_modules(source_units);

                        if (not modules) {
                            publish_diagnostics(uri, modules.error(), json);
                        } else {
                            auto sir = str::evaluate(std::move(modules.value()), std::move(source_units));
                            publish_diagnostics(uri, sir.get_diagnostics(), json);
                        }
                    } else if (base.method == "initialized") {
                        // pass
                    } else if (base.method == "shutdown") {
                        ShutdownResponse response { .id = base.id };
                        send(json.encode(response));
                    } else if (base.method == "exit") {
                        break;
                    } else if (base.method == "$/cancelRequest") {
                        // pass
                    } else {
                        std::cerr << "[strc] unknown method: " << base.method << std::endl;
                    }
                }
            } catch (coding::Json::Error& error) {
                std::cerr << "[strc] json error: " << error.what() << std::endl;
            }
        }

        return 0;
    }
}
