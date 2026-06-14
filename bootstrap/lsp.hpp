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

    struct InitResult final {
        struct Capabilities final {
            [[=str::coding::rename("textDocumentSync")]]
            u32 text_document_sync = 1;
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

    struct DidChangeParams final {
        struct Document final {
            std::string uri;
        };

        struct Change final {
            std::string text;
        };

        [[=str::coding::rename("textDocument")]]
        Document text_document;
        [[=str::coding::rename("contentChanges")]]
        std::vector<Change> content_changes;
    };

    struct DidChangeNotification final {
        DidChangeParams params;
    };

    inline void send(std::string_view message) {
        std::cout
            << "Content-Length: " << message.size()
            << "\r\n\r\n"
            << message
            << std::flush;
    }

    inline auto to_lsp_diagnostic(str::Diagnostic const& diag) -> Diagnostic {
        Diagnostic lsp_diag;

        switch (diag.severity) {
            case str::Diagnostic::Severity::Error:   lsp_diag.severity = Diagnostic::Severity::Error; break;
            case str::Diagnostic::Severity::Warning: lsp_diag.severity = Diagnostic::Severity::Warning; break;
            case str::Diagnostic::Severity::Info:    lsp_diag.severity = Diagnostic::Severity::Info; break;
            case str::Diagnostic::Severity::Runtime: lsp_diag.severity = Diagnostic::Severity::Error; break;
        }

        lsp_diag.message = diag.reason;

        std::visit(overloaded {
            [&] (str::Provenance::Span const& span) {
                // LSP Positions are 0-indexed, Token lines and columns are 1-indexed.
                lsp_diag.range.start.line = span.start.line > 0 ? span.start.line - 1 : 0;
                lsp_diag.range.start.character = span.start.column > 0 ? span.start.column - 1 : 0;
                lsp_diag.range.end.line = span.end.line > 0 ? span.end.line - 1 : 0;
                lsp_diag.range.end.character = (span.end.column > 0 ? span.end.column - 1 : 0) + span.end.count;
            },
            [&] (str::Provenance::Source const& source) {
                lsp_diag.range.start.line = 0;
                lsp_diag.range.start.character = 0;
                lsp_diag.range.end.line = 0;
                lsp_diag.range.end.character = 0;
            }
        }, diag.provenance.data);

        return lsp_diag;
    }

    inline void publish_diagnostics(std::string_view uri, std::string_view text, str::coding::Json& json) {
        // Collect disk workspace units and parse URI.
        auto source_units = str::run::collect_all_source_units();

        std::string file_path = std::string(uri);
        if (file_path.starts_with("file://")) file_path = file_path.substr(7);

        // Overlay the latest LSP memory buffer into the target file unit.
        bool found_in_fs = false;
        for (auto& unit : source_units) {
            if (file_path.ends_with(unit.source) or unit.source.ends_with(file_path)) {
                unit.text = text;
                found_in_fs = true;
                break;
            }
        }

        // Add the buffer as a new source unit if it doesn't currently exist on disk.
        if (not found_in_fs) {
            source_units.emplace_back(std::string(text), file_path);
        }

        // Execute compiler pipeline.
        std::vector<str::Diagnostic> compiler_diags;
        auto modules = str::run::parse_modules(source_units);

        if (not modules) {
            compiler_diags = std::move(modules.error());
        } else {
            auto sir = str::evaluate(std::move(modules.value()), std::move(source_units));
            for (auto const& diag : sir.get_diagnostics()) {
                compiler_diags.push_back(diag);
            }
        }

        // Filter for active file and build the notification.
        PublishDiagnosticsNotification notification;
        notification.params.uri = std::string(uri);

        for (auto const& diag : compiler_diags) {
            std::string_view diag_source;
            if (std::holds_alternative<str::Provenance::Span>(diag.provenance.data)) {
                diag_source = std::get<str::Provenance::Span>(diag.provenance.data).start.source;
            } else {
                diag_source = std::get<str::Provenance::Source>(diag.provenance.data).source;
            }

            if (file_path.ends_with(diag_source) || diag_source.ends_with(file_path)) {
                notification.params.diagnostics.push_back(to_lsp_diagnostic(diag));
            }
        }

        send(json.encode(notification));
    }

    inline i32 main() {
        std::cin.tie(nullptr);
        std::ios_base::sync_with_stdio(false);

        str::coding::Json json;

        while (true) {
            std::string line; std::getline(std::cin, line);
            if (not std::cin) break;

            try {
                if (line.starts_with("Content-Length: ")) {
                    u32 length = std::stoul(line.substr(16));

                    std::getline(std::cin, line);

                    std::string content(length, ' ');
                    std::cin.read(content.data(), length);

                    auto base = json.decode<BaseMessage>(content);

                    if (base.method == "initialize") {
                        InitializeResponse response;
                        response.id = base.id;
                        send(json.encode(response));
                    } else if (base.method == "textDocument/didOpen") {
                        auto request = json.decode<DidOpenNotification>(content);

                        publish_diagnostics(
                            request.params.text_document.uri,
                            request.params.text_document.text,
                            json
                        );
                    } else if (base.method == "textDocument/didChange") {
                        auto request = json.decode<DidChangeNotification>(content);

                        if (not request.params.content_changes.empty()) {
                            publish_diagnostics(
                                request.params.text_document.uri,
                                request.params.content_changes.front().text,
                                json
                            );
                        }
                    }
                }
            } catch (str::Diagnostic const& diagnostic) {
                std::cerr << "[strc] LSP Decode Error: " << diagnostic.reason << std::endl;
            }
        }

        return 0;
    }
}
