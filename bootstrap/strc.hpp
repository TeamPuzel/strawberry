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

#include <exception>
#include <string_view>
#include <variant>
#include <format>
#include <optional>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <functional>
#include <ranges>
#include <meta>
#include <print>
#include <expected>
#include <filesystem>
#include <ostream>
#include <iostream>
#include <cstdio>
#include "primitive.hpp"

namespace str {
    /// C++ nonsense for discount pattern matching with `std::visit`.
    template <typename... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template <typename... Ts> overloaded(Ts...) -> overloaded<Ts...>;

    /// Utility for `std::scope_exit` which still hasn't materialized in clang.
    template <typename Fn> class ScopeExit final {
        Fn fn;

      public:
        constexpr ScopeExit(Fn fn) noexcept : fn(std::move(fn)) {}

        constexpr ScopeExit(ScopeExit const&) = delete;
        constexpr ScopeExit(ScopeExit&&) = delete;
        constexpr ScopeExit& operator=(ScopeExit const&) = delete;
        constexpr ScopeExit& operator=(ScopeExit&&) = delete;

        constexpr ~ScopeExit() { fn(); }
    };

    /// A single valid token of the Strawberry language.
    class Token final {
      public:
        /// An erroneous token produced as provenance when the token stream throws a diagnostic.
        struct Error final {};
        /// Encodes `\n`
        struct NewLine final {};
        /// Encodes `(`
        struct ParenLeft final {};
        /// Encodes `)`
        struct ParenRight final {};
        /// Encodes `{`
        struct BraceLeft final {};
        /// Encodes `}`
        struct BraceRight final {};
        /// Encodes `[`
        struct BracketLeft final {};
        /// Encodes `]`
        struct BracketRight final {};
        /// Encodes `|`
        struct Pipe final {};
        /// Encodes `,`
        struct Comma final {};
        /// Encodes `:`
        struct Colon final {};
        /// Encodes `;`
        struct SemiColon final {};
        /// Encodes `@`
        struct At final {};
        /// Encodes `#`
        struct Pound final {};
        /// Encodes `'`
        struct Tick final {};
        /// Encodes `.`
        struct Dot final {};
        /// Encodes `->`
        struct Arrow final {};

        /// A semantic comment such as documentation.
        struct Comment final {
            std::string_view selector;
            std::string_view content;
        };

        /// A number literal.
        struct Number final {
            std::string_view content;
        };

        /// A single line string.
        struct String final {
            std::string_view content;
        };

        /// A single line chunk of a multiline string.
        struct MultilineString final {
            std::string_view content;
        };

        /// A pure identifier, used for a wide range of constructs including keywords.
        struct Identifier final {
            std::string_view content;
            bool leading_whitespace;
            bool trailing_whitespace;
        };

        /// A symbolic identifier, mainly used by operators except for `and` `or` and `not`.
        struct Symbolic final {
            std::string_view content;
            bool leading_whitespace;
            bool trailing_whitespace;
        };

        using Data = std::variant<
            Error,
            NewLine,
            ParenLeft,
            ParenRight,
            BraceLeft,
            BraceRight,
            BracketLeft,
            BracketRight,
            Pipe,
            Comma,
            Colon,
            SemiColon,
            At,
            Pound,
            Tick,
            Dot,
            Arrow,
            Comment,
            Number,
            String,
            MultilineString,
            Identifier,
            Symbolic
        >;

        Data data;
        std::string_view source;
        u32 position;
        u32 count;
        u32 line;
        u32 column;

        template <typename T> auto is() const -> bool {
            return std::holds_alternative<T>(data);
        }

        template <typename T> auto is(auto&& predicate) const -> bool {
            if (std::holds_alternative<T>(data)) {
                return std::invoke(predicate, std::as_const(std::get<T>(data)));
            } else {
                return false;
            }
        }

        template <typename T> auto get() const -> T {
            return std::get<T>(data);
        }

        template <typename T> auto get_as() const -> std::optional<T> {
            if (std::holds_alternative<T>(data)) {
                return std::get<T>(data);
            } else {
                return std::nullopt;
            }
        }

        /// For a symbolic or pure identifier it answers the content, otherwise `std::nullopt`
        auto identifier_content() const -> std::optional<std::string_view> {
            return std::visit(overloaded {
                [] (Token::Identifier tok) -> std::optional<std::string_view> { return tok.content; },
                [] (Token::Symbolic tok) -> std::optional<std::string_view> { return tok.content; },
                [] (auto tok) -> std::optional<std::string_view> { return std::nullopt; }
            }, data);
        }

      private:
        friend class TokenStream;

        constexpr Token(
            std::string_view source,
            u32 position,
            u32 count,
            u32 line,
            u32 column,
            Data data
        ) : data(data)
          , source(source)
          , position(position)
          , count(count)
          , line(line)
          , column(column)
        {}
    };

    /// A unified domain describing where in the source unit text (if anywhere) a construct originates.
    class Provenance final {
      public:
        /// An exact token span.
        struct Span final {
            Token start;
            Token end;
        };

        /// A generic source unit provenance uncertain about the exact origin.
        struct Source final {
            std::string_view source;
        };

        // TODO: Third provenance class, `Synthetic`.
        // There needs to be a way to easily specify synthetic provenance pointing between tokens
        // for use with synthesized code not present in source text.
        using Data = std::variant<Span, Source>;

        Data data;

        constexpr Provenance(Token start, Token end) noexcept : data(Span(start, end)) {}
        constexpr Provenance(Token token) noexcept : Provenance(token, token) {}

        constexpr explicit Provenance(std::string_view source) noexcept : data(Source(source)) {}

        /// Utility for merging provenances together if possible.
        constexpr Provenance(Provenance start, Provenance end) noexcept : Provenance(merge(start, end)) {}

      private:
        static constexpr auto merge(Provenance start, Provenance end) -> Provenance {
            if (std::holds_alternative<Span>(start.data) and std::holds_alternative<Span>(end.data)) {
                return Provenance(
                    std::get<Provenance::Span>(start.data).start,
                    std::get<Provenance::Span>(end.data).end
                );
            } else {
                return start;
            }
        }
    };

    /// A compile diagnostic, used to communicate issues during the compilation process.
    ///
    /// Diagnostics are all associated with provenance information identifying as precisely as possible
    /// where they come from.
    class Diagnostic final : std::exception {
      public:
        /// Determines the general cause of the diagnostic and how fatal it is for compilation.
        enum class Severity {
            /// Error severity represents an unrecoverable erroneous state preventing the program from compiling.
            ///
            /// It should use the color red when presented visually.
            Error,
            /// Warning severity represents warnings, recoverable issues that potentially require attention.
            ///
            /// It should use the color yellow when presented visually.
            Warning,
            /// Info severity represents less critical diagnostics such as linting.
            ///
            /// It should use the color blue when presented visually.
            Info,
            /// Runtime severity is a special severity intended for surfacing runtime diagnostics when
            /// operating in some sort of managed debug mode, such as runtime concurrency checks.
            ///
            /// It should use the color purple when presented visually.
            Runtime
        } severity;

        Provenance provenance;
        std::string reason;
        std::optional<std::string> help;

        /// Constructs a diagnostic with a reason.
        constexpr Diagnostic(Severity severity, Provenance provenance, std::string reason) noexcept
            : severity(severity), provenance(provenance), reason(std::move(reason)) {}

        /// Constructs a diagnostic with a reason and helpful description.
        constexpr Diagnostic(Severity severity, Provenance provenance, std::string reason, std::string help) noexcept
            : severity(severity), provenance(provenance), reason(std::move(reason)), help(std::move(help)) {}

        auto what() const noexcept -> char const* override {
            return reason.c_str();
        }

        /// Convenience for a diagnostic with error severity.
        static constexpr Diagnostic error(Provenance provenance, std::string reason) noexcept {
            return Diagnostic(Severity::Error, provenance, std::move(reason));
        }

        /// Convenience for a diagnostic with warning severity.
        static constexpr Diagnostic warning(Provenance provenance, std::string reason) noexcept {
            return Diagnostic(Severity::Warning, provenance, std::move(reason));
        }

        /// Convenience for a diagnostic with info severity.
        static constexpr Diagnostic info(Provenance provenance, std::string reason) noexcept {
            return Diagnostic(Severity::Info, provenance, std::move(reason));
        }

        /// Convenience for a diagnostic with runtime severity.
        static constexpr Diagnostic runtime(Provenance provenance, std::string reason) noexcept {
            return Diagnostic(Severity::Runtime, provenance, std::move(reason));
        }

        /// Convenience for a diagnostic with error severity, including help text.
        static constexpr Diagnostic error(Provenance provenance, std::string reason, std::string help) noexcept {
            return Diagnostic(Severity::Error, provenance, std::move(reason), std::move(help));
        }

        /// Convenience for a diagnostic with warning severity, including help text.
        static constexpr Diagnostic warning(Provenance provenance, std::string reason, std::string help) noexcept {
            return Diagnostic(Severity::Warning, provenance, std::move(reason), std::move(help));
        }

        /// Convenience for a diagnostic with info severity, including help text.
        static constexpr Diagnostic info(Provenance provenance, std::string reason, std::string help) noexcept {
            return Diagnostic(Severity::Info, provenance, std::move(reason), std::move(help));
        }

        /// Convenience for a diagnostic with runtime severity, including help text.
        static constexpr Diagnostic runtime(Provenance provenance, std::string reason, std::string help) noexcept {
            return Diagnostic(Severity::Runtime, provenance, std::move(reason), std::move(help));
        }
    };
}

template <> struct std::formatter<str::Token, char> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() and *it != '}') throw std::format_error("invalid format args for str::Token");
        return it;
    }

    constexpr auto format(str::Token const& token, std::format_context& ctx) const {
        using namespace str;
        return std::visit(overloaded {
            [&ctx] (Token::Error _)        { return std::format_to(ctx.out(), "Error");        },
            [&ctx] (Token::NewLine _)      { return std::format_to(ctx.out(), "NewLine");      },
            [&ctx] (Token::ParenLeft _)    { return std::format_to(ctx.out(), "ParenLeft");    },
            [&ctx] (Token::ParenRight _)   { return std::format_to(ctx.out(), "ParenRight");   },
            [&ctx] (Token::BraceLeft _)    { return std::format_to(ctx.out(), "BraceLeft");    },
            [&ctx] (Token::BraceRight _)   { return std::format_to(ctx.out(), "BraceRight");   },
            [&ctx] (Token::BracketLeft _)  { return std::format_to(ctx.out(), "BracketLeft");  },
            [&ctx] (Token::BracketRight _) { return std::format_to(ctx.out(), "BracketRight"); },
            [&ctx] (Token::Pipe _)         { return std::format_to(ctx.out(), "Pipe");         },
            [&ctx] (Token::Comma _)        { return std::format_to(ctx.out(), "Comma");        },
            [&ctx] (Token::Colon _)        { return std::format_to(ctx.out(), "Colon");        },
            [&ctx] (Token::SemiColon _)    { return std::format_to(ctx.out(), "SemiColon");    },
            [&ctx] (Token::At _)           { return std::format_to(ctx.out(), "At");           },
            [&ctx] (Token::Pound _)        { return std::format_to(ctx.out(), "Pound");        },
            [&ctx] (Token::Tick _)         { return std::format_to(ctx.out(), "Tick");         },
            [&ctx] (Token::Dot _)          { return std::format_to(ctx.out(), "Dot");          },
            [&ctx] (Token::Arrow _)        { return std::format_to(ctx.out(), "Arrow");        },

            [&ctx] (Token::Comment tok) {
                return std::format_to(ctx.out(), "Comment(selector: {}, content: {})", tok.selector, tok.content);
            },
            [&ctx] (Token::Number tok) {
                return std::format_to(ctx.out(), "Number(content: {})", tok.content);
            },
            [&ctx] (Token::String tok) {
                return std::format_to(ctx.out(), "String(content: {})", tok.content);
            },
            [&ctx] (Token::MultilineString tok) {
                return std::format_to(ctx.out(), "MultilineString(content: {})", tok.content);
            },
            [&ctx] (Token::Identifier tok) {
                return std::format_to(ctx.out(), "Identifier(content: {})", tok.content);
            },
            [&ctx] (Token::Symbolic tok) {
                return std::format_to(
                    ctx.out(), "Symbolic(content: {}, leading: {}, trailing: {})",
                    tok.content,
                    tok.leading_whitespace,
                    tok.trailing_whitespace
                );
            }
        }, token.data);
    }
};

namespace str {
    /// A validated stream of Strawberry tokens with very powerful pattern matching templates and other utilities.
    /// All tokens it produces are bound by the lifetime of the provided text and source views it operates on.
    class TokenStream final {
        std::string_view text;
        std::string_view source;
        u32 index = 0;
        u32 line = 1;
        u32 column = 1;
        u32 consumed_count = 0;
        std::optional<Token> last_token;
        std::vector<Token> provenance_stack;

      public:
        /// Answers a view of the text of the source unit being tokenized.
        auto get_text() const -> std::string_view {
            return text;
        }

        /// Answers the source identity of the source unit being tokenized.
        auto get_source() const -> std::string_view {
            return source;
        }

      private:
        void unchecked_span_push() {
            std::optional token = peek();
            if (not token) throw Diagnostic::error(failure_provenance(), "unexpected end of stream");
            provenance_stack.push_back(*token);
        }

        auto unchecked_span_pop() -> Provenance {
            if (not last_token) std::logic_error("attempted to obtain provenance span without consuming tokens");

            Token start = provenance_stack.back(); provenance_stack.pop_back();
            Token end = *last_token;

            return { start, end };
        }

        friend constexpr auto tokenize(std::string_view text, std::string_view source) -> TokenStream;

        constexpr TokenStream(std::string_view text, std::string_view source)
            : text(text), source(source){}

      public:
        /// A scope guard ensuring that
        class ProvenanceScope final {
            friend class TokenStream;

            TokenStream* tokens;
            i32 exceptions;

            explicit ProvenanceScope(TokenStream* tokens) : tokens(tokens), exceptions(std::uncaught_exceptions()) {}

          public:
            ProvenanceScope(ProvenanceScope const&) = delete;
            ProvenanceScope(ProvenanceScope&&) = delete;
            ProvenanceScope& operator=(ProvenanceScope const&) = delete;
            ProvenanceScope& operator=(ProvenanceScope&&) = delete;

            ~ProvenanceScope() noexcept(false) {
                if (tokens and exceptions == std::uncaught_exceptions())
                    throw std::logic_error("unconsumed provenance stack scope");
            }

            /// Balances the span answering the resulting provenance at the current point in time.
            [[nodiscard]] auto take() -> Provenance {
                auto result = tokens->unchecked_span_pop();
                tokens = nullptr;
                return result;
            }
        };

        /// A utility for checked provenance scopes. Because unbalanced provenance stack usage would completely
        /// break all provenance information, and forgetting to discard one is also erroneous, the answered
        /// provenance scope will throw unless its `take` sink method is used to obtain or explicitly discard the span.
        ///
        /// Start tracking a span with a provenance scope
        [[nodiscard]] auto span() -> ProvenanceScope {
            unchecked_span_push();
            return ProvenanceScope(this);
        }

      private:
        /// Consumes a specified count of characters.
        void consume(u32 count = 1) {
            index += count;
            column += count;
            consumed_count += count;
        }

        /// Rewinds a specified count of characters.
        void rewind(u32 count = 1) {
            index -= count;
            column -= count;
            consumed_count -= count;
        }

        /// Answers the current character. A bounds check is performed on access.
        auto at() const -> char {
            return text.at(index);
        }

        /// Answers a character at an index in the text,
        /// or if the access was out of bounds it answers `std::nullopt` instead.
        auto at(usize index) const -> std::optional<char> {
            if (index < text.size()) {
                return text[index];
            } else {
                return std::nullopt;
            }
        }

        /// Takes a count of characters and aswers a string view corresponding to the captured character span,
        /// or less if the stream ended early.
        auto take(usize count) -> std::string_view {
            auto result = text.substr(index, count);
            consume(count);
            return result;
        }

        /// Takes character until the sentinel value is matched or the stream ends,
        /// then answers a string view corresponding to the captured character span.
        auto take_until(char sentinel) -> std::string_view {
            usize count = 0;
            for (usize i = index; i < text.size() and text[i] != sentinel; i += 1) count += 1;
            return take(count);
        }

        /// Takes characters while the predicate answers true for the current character or the stream ends,
        /// then answers a string view corresponding to the captured character span.
        ///
        /// A very useful utility for all tokens with a content span.
        auto take_until(auto&& predicate) -> std::string_view {
            usize count = 0;
            for (usize i = index; i < text.size() and not std::invoke(predicate, text[i]); i += 1) count += 1;
            return take(count);
        }

        /// Answers true if the provided character matches the current character exactly.
        /// Answers false if it doesn't or if the stream is finished.
        auto is(char pattern) const -> bool {
            return text.at(index) == pattern;
        }

        /// Answers true if the provided pattern matches exactly the current and upcoming characters.
        /// If the stream is finished it answers false.
        auto are(std::string_view pattern) const -> bool {
            for (u32 i = 0; i < pattern.size(); i += 1)
                if (i + index >= text.size() or text.at(i + index) != pattern[i])
                    return false;
            return true;
        }

        /// Answers true if the current character is within the provided inclusive range.
        /// Used to check for alphanumeric ranges.
        auto in_range(char from, char to) const -> bool {
            auto value = text.at(index);
            return value >= from and value <= to;
        }

        /// Answers true if the current character is valid for identifiers.
        auto valid_ident() const -> bool {
            return
                not in_range(0, 31) and not is(127) and
                not is(' ') and not is('"') and not is('#') and not is('\'') and
                not is('(') and not is(')') and not is(',') and not is(':') and
                not is(';') and not is('@') and not is('[') and not is('\\') and
                not is(']') and not is('{') and not is('}') and not is('`');
        }

        /// Answers true if the current character is valid for number literals.
        auto valid_digit() const -> bool {
            return in_range('0', '9') or in_range('A', 'Z') or in_range('a', 'z');
        }

        /// Answers true if the current character is valid for symbolic identifiers.
        auto valid_symbolic_ident() const -> bool {
            return
                valid_ident() and
                not in_range('0', '9') and not in_range('A', 'Z') and not in_range('a', 'z') and
                not is('_');
        }

        /// Answers true if the current character is valid for pure identifiers.
        auto valid_pure_ident() const -> bool {
            return in_range('0', '9') or in_range('A', 'Z') or in_range('a', 'z') or is('_');
        }

        /// Discard processed characters without yielding a token.
        void no_yield() {
            consumed_count = 0;
        }

        /// Consume and merge automatically derived provenance information. Token provenance
        /// is their tail and a count of characters they consumed, so they are actually counted backwards.
        ///
        /// This is the primary and only correct way to maintain state while yielding a new token from the stream
        /// safely, tokens shouldn't be created otherwise without reason.
        template <typename T, typename... Arg> [[nodiscard]] auto yield(Arg... arg) -> Token {
            auto token = Token(
                source,
                index - 1,
                consumed_count,
                line,
                column - 1,
                Token::Data(T(std::forward<Arg>(arg)...))
            );

            consumed_count = 0;
            last_token = token;

            return token;
        }

        /// Counts a line in the internal provenance tracking state.
        void end_line() {
            line += 1;
            column = 1;
        }

      public:
        /// Provenance for diagnostics in cases where the parser fails to match anything,
        /// pointing at the next token or last token if the stream ended.
        auto fallthrough_provenance() -> Provenance {
            if (auto next = peek()) {
                return *next;
            } else {
                return failure_provenance();
            }
        }

        /// Provenance for an unexpected end of stream condition.
        auto failure_provenance() const -> Provenance {
            if (last_token) {
                return *last_token;
            } else {
                return Provenance(source);
            }
        }

        /// Provenance for the last token.
        auto last_provenance() const -> Provenance {
            if (last_token) {
                return *last_token;
            } else {
                return Provenance(source);
            }
        }

        /// Generic provenance for the source unit itself, with unspecified location.
        auto generic_provenance() const -> Provenance {
            return Provenance(source);
        }

        /// Answers generic provenance for the purpose of throwing in unimplemented branches of the parser.
        auto todo() -> Diagnostic {
            return Diagnostic::error(fallthrough_provenance(), "todo");
        }

        /// Answers true if there are no more tokens left in the stream.
        auto finished() const -> bool {
            return index >= text.size();
        }

        /// This is the primary interface of the token stream. While there are still characters left in the stream
        /// it will keep returning them, afterwards it just returns `std::nullopt`.
        ///
        /// The parser shouldn't call this directly, instead there are many helper mathods
        /// for matching patterns on the next token of the stream both destructively or nondestructively.
        /// A syntax highlighter could however use this method to iterate through the tokens for a line of code,
        /// because the Strawberry grammar does not have any multiline tokens.
        ///
        /// The method also automatically discards non semantic comments and throws diagnostics
        /// for many definite errors, including use of tabs or unterminated tokens.
        [[nodiscard]] auto next() -> std::optional<Token> { start:
            // Tail recursion is not guaranteed in C++ so we use gotos to do it manually.
            if (finished()) return std::nullopt;

            if (is(' ')) {
                while (not finished() and is(' ')) consume();
                no_yield(); goto start;
            } else if (is('\n')) {
                consume(); end_line(); return yield<Token::NewLine>();
            } else if (is('(')) {
                consume(); return yield<Token::ParenLeft>();
            } else if (is(')')) {
                consume(); return yield<Token::ParenRight>();
            } else if (is('{')) {
                consume(); return yield<Token::BraceLeft>();
            } else if (is('}')) {
                consume(); return yield<Token::BraceRight>();
            } else if (is('[')) {
                consume(); return yield<Token::BracketLeft>();
            } else if (is(']')) {
                consume(); return yield<Token::BracketRight>();
            } else if (is('|')) {
                consume(); return yield<Token::Pipe>();
            } else if (is(',')) {
                consume(); return yield<Token::Comma>();
            } else if (is(':')) {
                consume(); return yield<Token::Colon>();
            } else if (is(';')) {
                consume(); return yield<Token::SemiColon>();
            } else if (is('@')) {
                consume(); return yield<Token::At>();
            } else if (is('#')) {
                consume(); return yield<Token::Pound>();
            } else if (is('\'')) {
                consume(); return yield<Token::Tick>();
            } else if (is('.') and not are("..")) {
                consume(); return yield<Token::Dot>();
            } else if (are("->")) {
                consume(); return yield<Token::Arrow>();
            } else if (are("//")) {
                consume(2);

                auto selector = take_until([] (char c) { return c == ' ' or c == '\n'; });

                if (not finished() and is(' ')) consume();

                auto content = take_until('\n');

                // Discard non semantic comments from the stream.
                if (not selector.empty()) {
                    return yield<Token::Comment>(selector, content);
                } else {
                    no_yield(); goto start;
                }
            } else if (are("\\\\")) {
                consume(2);

                auto content = take_until('\n');

                return yield<Token::MultilineString>(content);
            } else if (is('"')) {
                consume();

                auto content = take_until([] (char c) { return c == '"' or c == '\n'; });

                if (not finished() and is('"')) {
                    consume();
                } else {
                    throw Diagnostic::error(yield<Token::Error>(), "unterminated string");
                }

                return yield<Token::String>(content);
            } else if (is('`')) {
                usize escape_count = 0;

                while (not finished() and is('`')) {
                    escape_count += 1;
                    consume();
                }

                std::string sentinel(escape_count, '`');

                usize count = 0;
                while (not finished() and not are(sentinel)) {
                    count += 1;
                    consume();
                }

                if (finished()) {
                    throw Diagnostic::error(yield<Token::Error>(), "unterminated string");
                }

                rewind(count);
                auto content = take(count);

                consume(escape_count);

                return yield<Token::String>(content);
            } else if (in_range('0', '9')) {
                usize count = 0;
                bool has_decimal = false;

              num_loop:
                consume();
                count += 1;

                if (finished()) {
                    // pass
                } else if (valid_digit()) {
                    goto num_loop;
                } else if (is('_')) {
                    consume();
                    if (finished() or not valid_digit()) throw Diagnostic::error(yield<Token::Error>(), "invalid underscore");
                    rewind();

                    goto num_loop;
                } else if (is('.')) {
                    consume();

                    if (finished()) throw Diagnostic::error(yield<Token::Error>(), "unterminated number");

                    // If the number does not continue then the dot is an operator or member access.
                    //
                    // Strawberry does not have float literals, only decimal literals, so it is very
                    // straightforward to disambiguate. The tokenizer is minimally strict to allow
                    // incomplete or invalid literals to tokenize such as for syntax highlighting,
                    // and it is the evaluator where they are actually parsed and validated.
                    if (in_range('0', '9') and not has_decimal) {
                        has_decimal = true;
                        goto num_loop;
                    } else {
                        rewind();
                    }
                }

                rewind(count);
                auto content = take(count);

                return yield<Token::Number>(content);
            } else if (valid_symbolic_ident()) {
                usize count = 0;

                bool leading_whitespace = at(index - 1)
                    .transform([] (char c) { return c == ' ' or c == '\n'; })
                    .value_or(true);

                do {
                    count += 1;
                    consume();
                } while (not finished() and valid_symbolic_ident());

                bool trailing_whitespace = at(index)
                    .transform([] (char c) { return c == ' ' or c == '\n' or c == ',' or c == '>' or c == ')'; })
                    .value_or(true);

                rewind(count);
                auto content = take(count);

                return yield<Token::Symbolic>(content, leading_whitespace, trailing_whitespace);
            } else if (valid_pure_ident()) {
                usize count = 0;

                bool leading_whitespace = at(index - 1)
                    .transform([] (char c) { return c == ' ' or c == '\n'; })
                    .value_or(true);

                do {
                    count += 1;
                    consume();
                } while (not finished() and valid_pure_ident());

                bool trailing_whitespace = at(index)
                    .transform([] (char c) { return c == ' ' or c == '\n' or c == ',' or c == '>' or c == ')'; })
                    .value_or(true);

                rewind(count);
                auto content = take(count);

                return yield<Token::Identifier>(content, leading_whitespace, trailing_whitespace);
            } else {
                throw Diagnostic::error(yield<Token::Error>(), std::format("unexpected character: {}", at()));
            }
        }

        /// Peek for a future token non destructively.
        auto peek(u32 offset = 1) -> std::optional<Token> {
            auto index = this->index;
            auto line = this->line;
            auto column = this->column;
            auto consumed_count = this->consumed_count;
            auto last_token = this->last_token;

            ScopeExit _ = [&] {
                this->index = index;
                this->line = line;
                this->column = column;
                this->consumed_count = consumed_count;
                this->last_token = last_token;
            };

            std::optional<Token> result;
            for (u32 i = 0; i < offset; i += 1) result = next();

            return result;
        }

        /// Drop a count of tokens.
        void drop(u32 count = 1) {
            for (u32 i = 0; i < count; i += 1) last_token = next();
        }

        /// Drop tokens while they satisfy a predicate.
        void drop_while(auto&& predicate) {
            std::optional token = peek();
            while (token and std::invoke(predicate, std::as_const(*token))) {
                last_token = next();
                token = peek();
            }
        }

        /// Match a token of specified type.
        template <typename T> auto match() -> std::optional<Token> {
            std::optional token = peek();

            if (token and std::holds_alternative<T>(token->data)) {
                last_token = next();
                return last_token;
            } else {
                return std::nullopt;
            }
        }

        /// Match an identifier token of specified type with an exact content pattern.
        template <typename T> auto match(std::string_view content) -> std::optional<Token> {
            std::optional token = peek();

            if (token and std::holds_alternative<T>(token->data) and token->identifier_content() == content) {
                last_token = next();
                return last_token;
            } else {
                return std::nullopt;
            }
        }

        /// Match a token with an additional predicate.
        template <typename T, typename F> auto match(F&& predicate) -> std::optional<Token>
        requires
            std::invocable<F, T const&>
        {
            std::optional token = peek();

            if (
                token and
                std::holds_alternative<T>(token->data) and
                std::invoke(predicate, std::as_const(std::get<T>(token->data)))
            ) {
                last_token = next();
                return last_token;
            } else {
                return std::nullopt;
            }
        }

        /// Match a token of specified type.
        /// Unwraps the result to the specified token type.
        template <typename T> auto match_as() -> std::optional<T> {
            return match<T>().transform(&Token::get<T>);
        }

        /// Match an identifier token of specified type with an exact content pattern.
        /// Unwraps the result to the specified token type.
        template <typename T> auto match_as(std::string_view content) -> std::optional<T> {
            return match<T>(content).transform(&Token::get<T>);
        }

        /// Match a token with an additional predicate.
        /// Unwraps the result to the specified token type.
        template <typename T, typename F> auto match_as(F&& predicate) -> std::optional<T> {
            return match<T>(std::forward<F>(predicate)).transform(&Token::get<T>);
        }

        template <typename T> auto allow() -> std::optional<Token> {
            std::optional token = peek();

            if (token and std::holds_alternative<T>(token->data)) {
                return next();
            } else {
                return std::nullopt;
            }
        }

        /// Like matching but does not contribute to the tail of a provenance span.
        /// Used mainly to prevent trailing newlines from being tracked by diagnostics.
        template <typename T> auto allow(std::string_view content) -> std::optional<Token> {
            std::optional token = peek();

            if (token and std::holds_alternative<T>(token->data) and token->identifier_content() == content) {
                return next();
            } else {
                return std::nullopt;
            }
        }

        /// Like matching but does not contribute to the tail of a provenance span.
        /// Used mainly to prevent trailing newlines from being tracked by diagnostics.
        template <typename T, typename F> auto allow(F&& predicate) -> std::optional<Token>
        requires
            std::invocable<F, T const&>
        {
            std::optional token = peek();

            if (
                token and
                std::holds_alternative<T>(token->data) and
                std::invoke(predicate, std::as_const(std::get<T>(token->data)))
            ) {
                return next();
            } else {
                return std::nullopt;
            }
        }

        /// Like matching but does not contribute to the tail of a provenance span.
        /// Used mainly to prevent trailing newlines from being tracked by diagnostics.
        template <typename T> auto allow_as(std::string_view content) -> std::optional<T> {
            return allow<T>(content).transform(&Token::get<T>);
        }

        /// Like matching but does not contribute to the tail of a provenance span.
        /// Used mainly to prevent trailing newlines from being tracked by diagnostics.
        template <typename T> auto allow_as() -> std::optional<T> {
            return allow<T>().transform(&Token::get<T>);
        }

        /// Like matching but does not contribute to the tail of a provenance span.
        /// Used mainly to prevent trailing newlines from being tracked by diagnostics.
        template <typename T, typename F> auto allow_as(F&& predicate) -> std::optional<T> {
            return allow<T>(std::forward<F>(predicate)).transform(&Token::get<T>);
        }

        /// Like matching but unwraps the token and throws a diagnostic otherwise.
        template <typename T> auto expect() -> Token {
            std::optional token = match<T>();

            if (token) {
                return *token;
            } else {
                std::optional<Token> failure = peek();
                if (failure) {
                    throw Diagnostic::error(*failure,
                        std::format("expected: {}, found: {}", std::meta::identifier_of(^^T), *failure)
                    );
                } else {
                    throw Diagnostic::error(failure_provenance(), "unexpected end of stream");
                }
            }
        }

        /// Like matching but unwraps the token and throws a diagnostic otherwise.
        template <typename T> auto expect(std::string_view content) -> Token {
            std::optional token = match<T>(content);

            if (token) {
                return *token;
            } else {
                std::optional<Token> failure = peek();
                if (failure) {
                    if (std::holds_alternative<T>(failure->data)) {
                        throw Diagnostic::error(*failure,
                            std::format(
                                "expected content: {}, found: {}",
                                content, failure->identifier_content().value_or("std::nullopt")
                            )
                        );
                    } else {
                        throw Diagnostic::error(*failure,
                            std::format("expected: {}, found: {}", std::meta::identifier_of(^^T), *failure)
                        );
                    }
                } else {
                    throw Diagnostic::error(failure_provenance(), "unexpected end of stream");
                }
            }
        }

        /// Like matching but unwraps the token and throws a diagnostic otherwise.
        template <typename T, typename F> auto expect(F&& predicate) -> Token {
            std::optional token = match<T>(std::forward<F>(predicate));

            if (token) {
                return *token;
            } else {
                std::optional<Token> failure = peek();
                if (failure) {
                    if (std::holds_alternative<T>(failure->data)) {
                        throw Diagnostic::error(*failure, "expected predicate failed");
                    } else {
                        throw Diagnostic::error(*failure,
                            std::format("expected: {}, found: {}", std::meta::identifier_of(^^T), *failure)
                        );
                    }
                } else {
                    throw Diagnostic::error(failure_provenance(), "unexpected end of stream");
                }
            }
        }

        /// Like matching but unwraps the token and throws a diagnostic otherwise.
        template <typename T> auto expect_as() -> T {
            return expect<T>().template get<T>();
        }

        /// Like matching but unwraps the token and throws a diagnostic otherwise.
        template <typename T> auto expect_as(std::string_view content) -> T {
            return expect<T>(content).template get<T>();
        }

        /// Like matching but unwraps the token and throws a diagnostic otherwise.
        template <typename T, typename F> auto expect_as(F&& predicate) -> T {
            return expect<T>(std::forward<F>(predicate)).template get<T>();
        }

        /// Like matching but does not consume the token.
        template <typename T> auto peek_match() -> std::optional<Token> {
            std::optional token = peek();

            if (token and std::holds_alternative<T>(token->data)) {
                return token;
            } else {
                return std::nullopt;
            }
        }

        /// Like matching but does not consume the token.
        template <typename T> auto peek_match(std::string_view content) -> std::optional<Token> {
            std::optional token = peek();

            if (token and std::holds_alternative<T>(token->data) and token->identifier_content() == content) {
                return token;
            } else {
                return std::nullopt;
            }
        }

        /// Like matching but does not consume the token.
        template <typename T, typename F> auto peek_match(F&& predicate) -> std::optional<Token>
        requires
            std::invocable<F, T const&>
        {
            std::optional token = peek();

            if (
                token and
                std::holds_alternative<T>(token->data) and
                std::invoke(predicate, std::as_const(std::get<T>(token->data)))
            ) {
                return token;
            } else {
                return std::nullopt;
            }
        }

        /// Like matching but does not consume the token.
        template <typename T> auto peek_match_as() -> std::optional<T> {
            return peek_match<T>().transform(&Token::get<T>);
        }

        /// Like matching but does not consume the token.
        template <typename T> auto peek_match_as(std::string_view content) -> std::optional<T> {
            return peek_match<T>(content).transform(&Token::get<T>);
        }

        /// Like matching but does not consume the token.
        template <typename T, typename F> auto peek_match_as(F&& predicate) -> std::optional<T> {
            return peek_match<T>(std::forward<F>(predicate)).transform(&Token::get<T>);
        }

        /// Like expecting but does not consume the token.
        template <typename T> auto peek_expect() -> Token {
            std::optional token = peek_match<T>();

            if (token) {
                return *token;
            } else {
                std::optional<Token> failure = peek();
                if (failure) {
                    throw Diagnostic::error(*failure,
                        std::format("expected: {}, found: {}", std::meta::identifier_of(^^T), *failure)
                    );
                } else {
                    throw Diagnostic::error(failure_provenance(), "unexpected end of stream");
                }
            }
        }

        /// Like expecting but does not consume the token.
        template <typename T> auto peek_expect(std::string_view content) -> Token {
            std::optional token = peek_match<T>(content);

            if (token) {
                return *token;
            } else {
                std::optional<Token> failure = peek();
                if (failure) {
                    if (std::holds_alternative<T>(failure->data)) {
                        throw Diagnostic::error(*failure,
                            std::format(
                                "expected content: {}, found: {}",
                                content, failure->identifier_content().value_or("std::nullopt")
                            )
                        );
                    } else {
                        throw Diagnostic::error(*failure,
                            std::format("expected: {}, found: {}", std::meta::identifier_of(^^T), *failure)
                        );
                    }
                } else {
                    throw Diagnostic::error(failure_provenance(), "unexpected end of stream");
                }
            }
        }

        /// Like expecting but does not consume the token.
        template <typename T, typename F> auto peek_expect(F&& predicate) -> Token {
            std::optional token = peek_match<T>(std::forward<F>(predicate));

            if (token) {
                return *token;
            } else {
                std::optional<Token> failure = peek();
                if (failure) {
                    if (std::holds_alternative<T>(failure->data)) {
                        throw Diagnostic::error(*failure, "expected predicate failed");
                    } else {
                        throw Diagnostic::error(*failure,
                            std::format("expected: {}, found: {}", std::meta::identifier_of(^^T), *failure)
                        );
                    }
                } else {
                    throw Diagnostic::error(failure_provenance(), "unexpected end of stream");
                }
            }
        }

        /// Like expecting but does not consume the token.
        template <typename T> auto peek_expect_as() -> T {
            return peek_expect<T>().template get<T>();
        }

        /// Like expecting but does not consume the token.
        template <typename T> auto peek_expect_as(std::string_view content) -> T {
            return peek_expect<T>(content).template get<T>();
        }

        /// Like expecting but does not consume the token.
        template <typename T, typename F> auto peek_expect_as(F&& predicate) -> T {
            return peek_expect<T>(std::forward<F>(predicate)).template get<T>();
        }
    };

    constexpr auto tokenize(std::string_view text, std::string_view source) -> TokenStream {
        return TokenStream(text, source);
    }
}

namespace str {
    constexpr usize GENERIC_PRECEDENCE = 7;
    constexpr usize PREFIX_PRECEDENCE = 1000;

    constexpr auto precedence(std::string_view pattern) -> usize {
        if (pattern == "=")   return 0; // Assignment. Not needed here? It is a right associative postfix expression.

        if (pattern == "or")  return 1; // Logical or.
        if (pattern == "and") return 2; // Logical and.

        if (pattern == "==")  return 3; // Equal to.
        if (pattern == "!=")  return 3; // Not equal to.

        if (pattern == "<")   return 4; // Less than.
        if (pattern == "<=")  return 4; // Less than or equal to.
        if (pattern == ">")   return 4; // Greater than.
        if (pattern == ">=")  return 4; // Greater than or equal to.

        if (pattern == "+")   return 5; // Addition operator.
        if (pattern == "-")   return 5; // Subtraction operator.

        if (pattern == "*")   return 6; // Multiplication operator.
        if (pattern == "/")   return 6; // True division operator.
        if (pattern == "\\")  return 6; // Pseudo division operator.
        if (pattern == "%")   return 6; // Remainder operator.

        return 7;
    }

    /// Determines the role of an operator in an expression.
    enum class OperatorRole {
        Infix,
        Prefix,
        Postfix
    };

    /// Answers the role of an operator based on associated whitespace.
    inline auto operator_role(Token token) -> OperatorRole {
        return std::visit(overloaded {
            // Symbolic operators are determined to be infix, prefix or postfix based on their whitespace.
            // If they are adjacent to the token on the right they are a prefix operator,
            // if they are adjacent to the token on the left they are a postfix operator,
            // and if they have whitespace on both sides they are an infix operator.
            //
            // The language intentionally specifies away the ability to have no whitespace on most operators,
            // except for operators that contain dots, such as range or concatenation operators which are
            // ideally written without whitespace on either side.
            [token] (Token::Symbolic symbolic) -> OperatorRole {
                bool lead = symbolic.leading_whitespace;
                bool trail = symbolic.trailing_whitespace;

                if (lead and trail)                 return OperatorRole::Infix;
                if (lead and not trail)             return OperatorRole::Prefix;
                if (trail and not lead)             return OperatorRole::Postfix;
                if (symbolic.content.contains('.')) return OperatorRole::Infix;

                throw Diagnostic::error(token, "infix operators without whitespace are only allowed for dot operators");
            },
            // We don't actually try resolve non infix pure identifier operators here,
            // the other roles are a fallback for prefix and postfix parsing instead.
            [token] (Token::Identifier identifier) -> OperatorRole {
                bool lead = identifier.leading_whitespace;
                bool trail = identifier.trailing_whitespace;

                if (lead and trail) return OperatorRole::Infix;

                throw Diagnostic::error(token, "infix operators without whitespace are only allowed for dot operators");
            },
            // Arbitrary tokens can't be used as operators, this should never happen.
            [] (auto other) -> OperatorRole {
                throw std::logic_error("attempt to query operator role of non identifier token");
            }
        }, token.data);
    }

    /// A checked qualified path.
    class Path final {
        std::string data;

        void validate() {
            if (data.empty()) throw std::logic_error("empty path");
            if (data.front() == '.') throw std::logic_error("leading dot in path");
            if (data.back() == '.') throw std::logic_error("trailing dot in path");

            for (usize i = 0; i < data.size(); i += 1) {
                if (data[i] == '.' and data[i - 1] == '.') throw std::logic_error("empty subpath in path");
            }
        }

      public:
        Path(std::string value) : data(value) { validate(); }
        Path(std::string_view value) : data(value) { validate(); }

        operator std::string() const { return data; }

        auto operator == (Path const& other) const -> bool = default;
        auto operator != (Path const& other) const -> bool = default;

        auto operator + (Path const& other) const -> Path {
            return data + "." + other.data;
        }

        void operator += (Path const& other) {
            data += ".";
            data += other.data;
        }

        auto split() { return data | std::views::split('.'); }

        static auto join(auto&& range) -> Path {
            std::string data;

            usize i = 0; // Still no enumerate...
            for (auto& component : range) {
                if (not (i == 0 or i == range.size() - 1)) data += ".";
                data += std::move(component);
                i += 1;
            }

            return data;
        }

        auto prefix() const -> std::optional<Path> {
            auto result = data.find_last_of('.');

            if (result == std::string::npos) {
                return std::nullopt;
            } else {
                return data.substr(0, result);
            }
        }

        auto nested_in(Path const& other) const -> bool {
            using Ref = std::reference_wrapper<const Path>;
            using PathRef = std::optional<Ref>;

            static constexpr auto nested_in_recurse = [] (this auto recurse, PathRef self, PathRef other) -> bool {
                if (not self) return false;
                if (self == other) return true;
                return recurse(
                    self->get()
                        .prefix()
                        .transform([] (auto p) { return Ref(p); }),
                    other
                );
            };

            return nested_in_recurse(*this, other);
        }
    };

    /// A syntax tree structure used to compose expressions within declarations.
    /// Everything outside the rigid declaration grammar is an expression in this language.
    class Expr final {
      public:
        using ExprBox = std::unique_ptr<Expr>;

        static ExprBox box(Expr& expr) {
            return std::make_unique<Expr>(std::move(expr));
        }

        static ExprBox box(Expr&& expr) {
            return std::make_unique<Expr>(std::move(expr));
        }

        static ExprBox box(ExprBox& expr) {
            return std::move(expr);
        }

        static ExprBox box(ExprBox&& expr) {
            return std::move(expr);
        }

        struct Infix final {
            std::string_view name;
            ExprBox lhs;
            ExprBox rhs;

            Infix(
                std::string_view name,
                auto lhs,
                auto rhs
            ) : name(name)
              , lhs(box(lhs))
              , rhs(box(rhs))
            {}
        };

        struct Prefix final {
            std::string_view name;
            ExprBox rhs;

            Prefix(
                std::string_view name,
                auto rhs
            ) : name(name)
              , rhs(box(rhs))
            {}
        };

        struct Postfix final {
            std::string_view name;
            ExprBox lhs;

            Postfix(
                std::string_view name,
                auto lhs
            ) : name(name)
              , lhs(box(lhs))
            {}
        };

        struct [[deprecated]] Projection final {
            enum class Kind { Mutating, Borrowing } kind;
            ExprBox expr;

            Projection(Kind kind, auto expr) : kind(kind), expr(box(expr)) {}
        };

        struct Wildcard final {};

        struct Number final {
            std::string_view literal;
            explicit Number(std::string_view literal) : literal(literal) {}
        };

        struct String final {
            std::string content;
            explicit String(std::string content) : content(content) {}
        };

        struct Boolean final {
            bool value;
            explicit Boolean(bool value) : value(value) {}
        };

        /// A mutable projection argument forward `foo(&bar)`.
        struct MutableForward final {
            ExprBox expr;
        };

        /// A universal projection argument forward `foo(&&bar)`.
        struct UniversalForward final {
            ExprBox expr;
        };

        struct Intrinsic final {
            std::optional<std::string_view> backend;
            std::string_view name;
            std::vector<ExprBox> expressions;

            Intrinsic(std::optional<std::string_view> backend, std::string_view name, std::vector<ExprBox> expressions)
                : backend(backend), name(name), expressions(std::move(expressions)) {}
        };

        struct Tuple final {
            struct Element final {
                std::optional<std::string_view> label;
                ExprBox expr;
            };

            std::vector<Element> elements;

            Tuple() {}

            explicit Tuple(std::vector<Element> elements) : elements(std::move(elements)) {}
        };

        struct Callable final {
            ExprBox arguments;
            std::optional<std::vector<ExprBox>> captures;
            bool async;
            std::optional<std::vector<ExprBox>> throws;
            ExprBox return_type;

            Callable(
                auto arguments,
                std::optional<std::vector<ExprBox>> captures,
                bool async,
                std::optional<std::vector<ExprBox>> throws,
                auto return_type
            ) : arguments(box(arguments))
              , captures(std::move(captures))
              , async(async)
              , throws(std::move(throws))
              , return_type(box(return_type))
            {}
        };

        /// Generic subtype boolean expression.
        ///
        /// It can be used on anything.
        /// ```
        /// Never: Int // true
        /// Int: Numeric // true
        /// Float: Numeric // false
        /// Int: Any // true
        /// variable: Int // this works too
        /// ```
        ///
        /// For classes it accounts for their hierarchy and works at runtime.
        ///
        /// In general this expression is used in type constraints, because it shapes the rigid type variable.
        /// For example, this is how it could be used for generic constraints:
        /// ```
        /// // A where clause is a boolean condition filtering out the declaration based on a compile time boolean.
        /// // Because this means types that do not subtype the Numeric category are rejected, the rigid type
        /// // variable is now known to be Numeric and we are allowed to use its requirements, like the + operator.
        /// fun foo<Bar>(bar: Bar) -> Bar where Bar: Numeric { bar + bar }
        ///
        /// // We can conditionally use it in the body but that will not filter the declaration itself,
        /// // which is usually not desirable because it can't be overloaded.
        /// fun foo<Bar>(bar: Bar) -> Bar {
        ///     if Bar: Numeric then bar + bar else bar
        /// }
        /// ```
        struct Subtype final {
            ExprBox lhs;
            ExprBox rhs;

            Subtype(auto lhs, auto rhs) : lhs(box(lhs)), rhs(box(rhs)) {}
        };

        /// A set of annotations attached to an expression rather than declaration.
        /// Mainly useful for annotating structural types like tuples or callables.
        struct Annotations final {
            struct Annotation final {
                ExprBox annotation;
                bool unsafe;

                Annotation(auto annotation, bool unsafe) : annotation(box(annotation)), unsafe(unsafe) {}
            };

            std::vector<Annotation> annotations;
            ExprBox expr;

            Annotations(std::vector<Annotation> annotations, auto expr)
                : annotations(std::move(annotations)), expr(box(expr)) {}
        };

        /// A variadic pack fold expression.
        ///
        /// They are useful for abstracting over variadic packs without immediately having to drop
        /// down to a compile time loop.
        ///
        /// They have two forms depending on if they use an infix operator, comma or semicolon:
        /// - Statement map expression: `(expr.toggle(); ...)`
        /// - Tuple map expression: `(expr.toggle(), ...)`
        /// - Reduction expression: `(expr + ...)`
        struct Fold final {
            struct CommaConjunction final {};
            struct SemiColonConjunction final {};
            struct InfixConjunction final { std::string_view name; };

            using Conjunction = std::variant<CommaConjunction, SemiColonConjunction, InfixConjunction>;

            ExprBox expr;
            Conjunction conjunction;

            Fold(auto expr, Conjunction conjunction) : expr(box(expr)), conjunction(conjunction) {}
        };

        /// Tuple swizzle expression.
        ///
        /// Some languages, especially those dedicated to GPU programming, tend to have a set of hardcoded
        /// swizzles usable on vectors. That is indeed rather convenient for dense vector math, but
        /// hardcoding things isn't the Strawberry way. Thankfully there is an unambiguous syntax to
        /// do exactly that.
        ///
        /// If we didn't have swizzles:
        /// `let mut point = (x: 0, y: 0); point = (x: point.y, y: point.x)`
        ///
        /// They use labels if present:
        /// `(x: 0, y: 0).(y, x)`
        ///
        /// Or indices:
        /// `(x: 0, y: 0).(1, 0)`
        ///
        /// Or mix both:
        /// `(x: 0, y: 0).(1, x)`
        ///
        /// They can relabel while swizzling:
        /// `(x: 0, y: 0).(w: y, h: x)`
        ///
        /// They can relabel without swizzling (shorthand, useful to explicitly pass a tuple despite mismatched labels):
        /// `(x: 0, y: 0).(w:h:)`
        ///
        /// They can
        ///
        /// Tuples enter a decay tendency after a swizzle. If they are bound
        /// they implicitly preserve labels, but they will decay to all other tuples regardless of labels.
        /// Swizzles that relabel are an exception, they do not want to decay
        /// and will still raise a compile error if labels don't match.
        ///
        /// Not having a decay tendency would have made swizzles very annoying to use:
        /// ```
        /// let mut point = (x: 0, y: 0)
        /// point = point.(y, x) // If labels didn't decay this would be a compile error like usual!
        /// ```
        struct Swizzle final {

        };

        /// The standard call operator used as postfix to to an expression.
        struct Call final {
            struct Argument final {
                std::optional<std::string_view> label;
                ExprBox expr;
            };

            ExprBox callee;
            std::vector<Argument> arguments;

            Call(auto callee, std::vector<Argument> arguments)
                : callee(box(callee)), arguments(std::move(arguments)) {}
        };

        /// The subscript operator `[]`, used to match unqualified accessors `get` `set` and `mut` with extra arguments.
        ///
        /// The no-extra-argument variants are instead matched by the `*` and `->` direct projection operators.
        struct Subscript final {
            struct Argument final {
                std::optional<std::string_view> label;
                ExprBox expr;
            };

            ExprBox callee;
            std::vector<Argument> arguments;

            Subscript(auto callee, std::vector<Argument> arguments)
                : callee(box(callee)), arguments(std::move(arguments)) {}
        };

        /// A block expression `{}`.
        ///
        /// Contains any number of expressions separated by newlines or semicolons.
        struct Block final {
            std::vector<ExprBox> expressions;
            explicit Block(std::vector<ExprBox> expressions) : expressions(std::move(expressions)) {}
        };

        /// An explicit specialization invocation `<>`.
        struct Generics final {
            struct Parameter final {
                std::optional<std::string_view> label;
                ExprBox expr;
            };

            ExprBox callee;
            std::vector<Parameter> parameters;

            Generics(auto callee, std::vector<Parameter> parameters)
                : callee(box(callee)), parameters(std::move(parameters)) {}
        };

        /// A literal list expression `[1, 2, 3]`.
        struct List final {
            std::vector<ExprBox> expressions;
            explicit List(std::vector<ExprBox> expressions) : expressions(std::move(expressions)) {}
        };

        /// A pure identifier expression.
        struct Identifier final {
            std::string_view name;
            explicit Identifier(std::string_view name) : name(name) {}
        };

        /// An unsafe expression.
        struct Unsafe final {
            ExprBox expr;
            explicit Unsafe(auto expr) : expr(box(expr)) {}
        };

        /// A non-tail recursion expression.
        struct Recurse final {
            ExprBox expr;
            explicit Recurse(auto expr) : expr(box(expr)) {}
        };

        /// A non-tail return expression.
        struct Return final {
            std::optional<ExprBox> expr;

            Return() {}
            explicit Return(auto expr) : expr(box(expr)) {}
        };

        /// A coroutine-closure (generator) yield expression.
        struct Yield final {
            ExprBox expr;
            explicit Yield(auto expr) : expr(box(expr)) {}
        };

        /// A throw expression.
        struct Throw final {
            ExprBox expr;
            explicit Throw(auto expr) : expr(box(expr)) {}
        };

        /// An await expression.
        /// `await <expr>`
        struct Await final {
            ExprBox expr;
            explicit Await(auto expr) : expr(box(expr)) {}
        };

        /// A member projection expression `.`.
        /// `<expr>.name`
        struct Member final {
            std::string_view name;
            ExprBox expr;

            Member(std::string_view name, auto expr) : name(name), expr(box(expr)) {}
        };

        /// A reflective meta member projection expression `::`.
        struct MetaMember final {
            std::string_view name;
            ExprBox expr;

            MetaMember(std::string_view name, auto expr) : name(name), expr(box(expr)) {}
        };

        /// A ternary conditional expression.
        ///
        /// ```
        /// if <pattern> then <expr> // implicit else of ().
        /// if <pattern> {<blockexpr>} // implicit else of ().
        ///
        /// if <pattern> then <expr> else <expr>
        /// if <pattern> then {<blockexpr>} else <expr>
        /// ```
        ///
        /// Note that there is no distinct "else if", because it is actually a natural consequence of the grammar.
        /// Effectively, another `if` as the expression in the `else`, which already has the semantics we want.
        ///
        /// Similarly there is no dedicated block version of else, one can just use a free block expression
        /// as the else expression to the same effect.
        ///
        /// This unifies conditional control flow really nicely.
        struct If final {
            ExprBox pattern;
            ExprBox body;
            std::optional<ExprBox> else_body;

            If(
                auto pattern,
                auto body,
                auto else_body
            ) : pattern(box(pattern))
              , body(box(body))
              , else_body(box(else_body))
            {}

            If(
                auto pattern,
                auto body
            ) : pattern(box(pattern))
              , body(box(body))
            {}
        };

        /// A divergent pattern binding inversion expression.
        struct Guard final {
            ExprBox pattern;
            ExprBox else_body;

            Guard(auto pattern, auto else_body) : pattern(box(pattern)), else_body(box(else_body)) {}
        };

        struct When final {
            ExprBox pattern;
            explicit When(auto pattern) : pattern(box(pattern)) {}
        };

        struct While final {
            ExprBox pattern;
            ExprBox body;

            While(auto pattern, auto body) : pattern(box(pattern)), body(box(body)) {}
        };

        struct MemberInfer final {
            std::string_view name;
            explicit MemberInfer(std::string_view name) : name(name) {}
        };

        struct Label final {
            std::string_view name;
            ExprBox expr;

            Label(std::string_view name, auto expr) : name(name), expr(box(expr)) {}
        };

        struct Break final {
            std::optional<std::string_view> label;
            std::optional<ExprBox> expr;

            explicit Break(std::optional<std::string_view> label) : label(label) {}
            Break(std::optional<std::string_view> label, auto expr) : label(label), expr(box(expr)) {}
        };

        struct Continue final {
            std::optional<std::string_view> label;

            Continue() {}
            explicit Continue(std::string_view label) : label(label) {}
        };

        struct Loop final {
            ExprBox body;
            explicit Loop(auto body) : body(box(body)) {}
        };

        struct Match final {
            struct Arm final {
                ExprBox pattern;
                ExprBox body;
            };

            ExprBox lhs;
            std::vector<Arm> arms;

            Match(auto lhs, std::vector<Arm> arms) : lhs(box(lhs)), arms(std::move(arms)) {}
        };

        struct Catch final {
            struct Arm final {
                std::string_view name;
                ExprBox pattern;
                std::optional<ExprBox> where_clause;
                ExprBox body;
            };

            ExprBox lhs;
            std::vector<Arm> arms;

            Catch(auto lhs, std::vector<Arm> arms) : lhs(box(lhs)), arms(std::move(arms)) {}
        };

        struct Closure final {
            struct Argument final {
                bool mut;
                bool ref;
                std::string_view name;
                std::optional<ExprBox> type_expr;

                Argument(bool mut, bool ref, std::string_view name, std::optional<ExprBox> type_expr)
                    : mut(mut), ref(ref), name(name), type_expr(std::move(type_expr)) {}
                Argument(bool mut, bool ref, std::string_view name, auto type_expr)
                    : mut(mut), ref(ref), name(name), type_expr(box(type_expr)) {}
                Argument(bool mut, bool ref, std::string_view name)
                    : mut(mut), ref(ref), name(name) {}
            };

            struct Capture final {
                bool mut;
                bool ref;
                std::string_view name;
                std::optional<ExprBox> init_expr;

                Capture(bool mut, bool ref, std::string_view name, std::optional<ExprBox> init_expr)
                    : mut(mut), ref(ref), name(name), init_expr(std::move(init_expr)) {}
                Capture(bool mut, bool ref, std::string_view name, auto init_expr)
                    : mut(mut), ref(ref), name(name), init_expr(box(init_expr)) {}
                Capture(bool mut, bool ref, std::string_view name)
                    : mut(mut), ref(ref), name(name) {}
            };

            std::vector<Argument> arguments;
            std::optional<std::vector<Capture>> captures;
            std::optional<std::vector<ExprBox>> throws;
            std::optional<ExprBox> return_type;
            bool async = false;
            ExprBox body;

            Closure(
                std::vector<Argument> arguments,
                std::optional<std::vector<Capture>> captures,
                std::optional<std::vector<ExprBox>> throws,
                std::optional<ExprBox> return_type,
                bool async,
                auto body
            ) : arguments(std::move(arguments))
              , captures(std::move(captures))
              , throws(std::move(throws))
              , return_type(std::move(return_type))
              , async(async)
              , body(box(body))
            {}

            explicit Closure(auto body) : body(box(body)) {}
        };

        struct TrailingClosure final {
            ExprBox lhs;
            ExprBox closure;

            TrailingClosure(auto lhs, auto closure) : lhs(box(lhs)), closure(box(closure)) {}
        };

        struct Destructuring final {
            struct Binding final {
                bool mut;
                bool ref;
                std::string_view name;
                std::optional<ExprBox> type_expr;
            };

            struct Tuple final {
                std::vector<Destructuring> elements;
            };

            using Data = std::variant<Binding, Tuple>;

            std::unique_ptr<Data> data;

            explicit Destructuring(Binding binding) : data(std::make_unique<Data>(std::move(binding))) {}
            explicit Destructuring(Tuple tuple) : data(std::make_unique<Data>(std::move(tuple))) {}

            template <typename T> auto get() -> T& {
                return std::get<T>(data);
            }

            template <typename T> auto get_as() -> T* {
                if (std::holds_alternative<T>(data)) {
                    return &std::get<T>(data);
                } else {
                    return nullptr;
                }
            }

            template <typename T> auto get() const -> T const& {
                return std::get<T>(data);
            }

            template <typename T> auto get_as() const -> T const* {
                if (std::holds_alternative<T>(data)) {
                    return &std::get<T>(data);
                } else {
                    return nullptr;
                }
            }
        };

        struct For final {
            Destructuring binding;
            ExprBox iterator;
            std::optional<ExprBox> where_clause;
            ExprBox body;
            std::optional<Destructuring> else_binding;
            std::optional<ExprBox> else_body;

            For(
                Destructuring binding,
                auto iterator,
                std::optional<ExprBox> where_clause,
                auto body,
                std::optional<Destructuring> else_binding,
                std::optional<ExprBox> else_body
            ) : binding(std::move(binding))
              , iterator(box(iterator))
              , where_clause(std::move(where_clause))
              , body(box(body))
              , else_binding(std::move(else_binding))
              , else_body(std::move(else_body))
            {}
        };

        struct Binding final {
            bool mut;
            bool ref;
            std::string_view name;
            std::optional<ExprBox> type_expr;
            std::optional<ExprBox> rhs;

            Binding(
                bool mut,
                bool ref,
                std::string_view name,
                std::optional<ExprBox> type_expr,
                std::optional<ExprBox> rhs
            ) : mut(mut)
              , ref(ref)
              , name(name)
              , type_expr(std::move(type_expr))
              , rhs(std::move(rhs))
            {}
        };

        struct DestructuringBinding final {
            Destructuring pattern;
            std::optional<ExprBox> rhs;

            DestructuringBinding(Destructuring pattern, std::optional<ExprBox> rhs)
                : pattern(std::move(pattern)), rhs(std::move(rhs)) {}
        };

        struct PatternBinding final {
            bool mut;
            bool ref;
            std::string_view name;
            std::optional<ExprBox> type_expr;

            PatternBinding(bool mut, bool ref, std::string_view name, std::optional<ExprBox> type_expr)
                : mut(mut), ref(ref), name(name), type_expr(std::move(type_expr)) {}
            PatternBinding(bool mut, bool ref, std::string_view name, auto type_expr)
                : mut(mut), ref(ref), name(name), type_expr(box(type_expr)) {}
            PatternBinding(bool mut, bool ref, std::string_view name)
                : mut(mut), ref(ref), name(name) {}
        };

        /// An expression used as a subexpression in other expressions when matching patterns.
        struct Pattern final {
            struct Enum final {
                std::string_view name;
                std::vector<ExprBox> elements;
            };

            struct Tuple final {
                std::vector<ExprBox> elements;
            };

            struct Value final {
                ExprBox expr;
            };

            using Data = std::variant<Enum, Tuple, Value>;

            Data data;

            std::optional<ExprBox> rhs;
            std::optional<ExprBox> where_clause;

            Pattern(Data data, std::optional<ExprBox> rhs, std::optional<ExprBox> where_clause)
                : data(std::move(data)), rhs(std::move(rhs)), where_clause(std::move(where_clause)) {}
        };

        /// A list of lifetime bindings.
        /// `<expr> 'binding 'other`
        ///
        /// This is used to bind return types, and it is an expression rather than feature of the
        /// function declaration because it needs to be precise `-> (Int, NonCopyable 'self)`
        struct Lifetimes final {
            std::vector<std::string_view> bindings;
            explicit Lifetimes(std::vector<std::string_view> bindings) : bindings(std::move(bindings)) {}
        };

        /// A class initialization sugar expression.
        ///
        /// For a base class and its backing storage type the expression is a composition of
        /// invoking an initializer of the backing storage type with a concrete class storage.
        ///
        /// ```
        /// base class Foo: Id<Self> {
        ///     pub init(x: Int, y: Int) {}
        /// }
        ///
        /// class Bar: Foo {}
        ///
        /// let x: Foo = new Bar(x: 0, y: 0)
        /// let y: Foo = new(in: allocator) Bar(x: 0, y: 0)
        ///
        /// // Desugaring
        /// let x: Id<Foo> = .init(Bar(x: 0, y: 0))
        /// let y: Id<Foo> = .init(Bar(x: 0, y: 0), in: allocator)
        /// ```
        struct New final {

        };

        using Data = std::variant<
            Infix,
            Prefix,
            Postfix,
            Wildcard,
            Number,
            String,
            Boolean,
            MutableForward,
            UniversalForward,
            Intrinsic,
            Tuple,
            Callable,
            Subtype,
            Annotations,
            Call,
            Subscript,
            Block,
            Generics,
            List,
            Identifier,
            Unsafe,
            Recurse,
            Return,
            Yield,
            Throw,
            Await,
            Member,
            MetaMember,
            If,
            Guard,
            When,
            While,
            MemberInfer,
            Label,
            Break,
            Continue,
            Loop,
            Match,
            Catch,
            Closure,
            TrailingClosure,
            For,
            Binding,
            DestructuringBinding,
            PatternBinding,
            Pattern,
            Lifetimes,
            New
        >;

        Data data;
        Provenance provenance;

        Expr(Provenance provenance, Data data) : data(std::move(data)), provenance(provenance) {}

        template <typename T> auto get() -> T& {
            return std::get<T>(data);
        }

        template <typename T> auto get_as() -> T* {
            if (std::holds_alternative<T>(data)) {
                return &std::get<T>(data);
            } else {
                return nullptr;
            }
        }

        template <typename T> auto get() const -> T const& {
            return std::get<T>(data);
        }

        template <typename T> auto get_as() const -> T const* {
            if (std::holds_alternative<T>(data)) {
                return &std::get<T>(data);
            } else {
                return nullptr;
            }
        }
    };

    class Decl final {
      public:
        struct GenericParameter final {
            enum class Kind { Value, Type } kind;
            std::optional<std::string_view> label;
            std::string_view name;
            std::optional<Expr> type_expr;
            std::optional<Expr> default_expr;
        };

        struct Argument final {
            /// The convention by which the argument is being passed.
            enum class Convention {
                /// The default argument convention `fun foo(arg: T)`.
                /// Represents a value we own.
                Consume,
                /// The borrowed argument convention `fun foo(arg: &T)`.
                /// Represents a value we do not own.
                BorrowedProjection,
                /// The mutable argument convention `fun foo(arg: mut &T)`.
                /// Represents a value we temporarily own but must return back.
                MutableProjection,
                /// The universal argument convention `fun foo(arg: &&T)`.
                /// Represents a value of unknown (any of the other three) ownership status.
                UniversalProjection
            };

            /// The argument label.
            std::optional<std::string_view> label;
            /// The binding name, and the label if one was not provided.
            std::string_view name;
            /// When passing a closure with no arguments an implicit convention means that
            /// passing a value of the closure's return type will automatically wrap it in one.
            /// This is used to implement lazy semantics, primarily short circuit semantics for boolean operators.
            bool implicit;
            /// The argument passing convention.
            Convention convention;
            /// The type expression.
            Expr type_expr;
            /// The default expression.
            std::optional<Expr> default_expr;
        };

        /// There are
        struct Fun final {
            struct Operator final {
                enum class Kind { Prefix, Infix, Postfix } kind;
                std::string_view name;
            };

            enum class Accessor { Get, Set, Mut } accessor;

            std::optional<Operator> operator_spec;
            std::string_view name;
            std::vector<GenericParameter> generics;
            std::vector<Argument> args;
            /// A list of thrown exceptions.
            std::vector<Expr> throws;
            /// In highly generic code `rethrows` can be used instead of a `throws` list, which makes the function
            /// completely transparent to exceptions, forwarding all of them. This doesn't conflict with an explicit
            /// exception list, so in cases where distinct exceptions are still thrown they should be expressed
            /// as such in the normal list for completeness and readability.
            bool rethrows;
            std::optional<Expr> return_type;
            std::optional<Expr> where;
            std::optional<Expr> body;
        };

        /// Type initializer.
        ///
        /// There are two kinds of initializers, instance and const. A const init has no parenthesized argument list,
        /// must be const, and defines compile time initialization logic (through side effects, such as
        /// emitting type metadata for a class hierarchy into the constant section of a binary). An instance init
        /// can be const, has a parenthesized argument list, and defines in-place initialization logic for instances.
        struct Init final {

        };

        /// Type deinitializer.
        ///
        /// Deinitializers are only available to noncopyable types.
        ///
        /// There are two kinds of deinitializers, automatic and functional. An automatic deinit has no
        /// parenthesized argument list and defines deinitialization logic which will happen automatically at
        /// the end of the instance's lifetime. A functional deinit is a normal function declaration following
        /// deinit as a modifier. It must consume `self` and it takes destructive ownership of the type, that is,
        /// the automatic deinit is not run and instead the function takes individual ownership of its members.
        struct Deinit final {

        };

        /// Structure declaration.
        ///
        /// Structs are nominal value types.
        struct Struct final {
            std::string_view name;
            std::vector<GenericParameter> generics;
            std::vector<Expr> superlist;
            std::optional<Expr> where;
            std::vector<Decl> decls;
        };

        /// Enumeration declaration.
        struct Enum final {

        };

        /// Category declaration.
        struct Category final {

        };

        /// Extension declaration.
        ///
        /// There are four kinds of extensions.
        /// A static extension targets the namespace of a type or category declaration, but does not inherit its
        /// generic context. A nominal type extension targets a nominal type and extends individual instances if it
        /// has a generic context, inheriting it. A blanket extension targets the top type, `Any`. A structural
        /// extension is a variadic extension which allows extending tuples.
        ///
        /// Note that the static extension can't extend with a category conformance since it is just a namespace
        /// mixin feature. If it wasn't there making a type generic would lose the ability to namespace
        /// related types within non-generically, which would be very unfortunate and necessitate naming conventions
        /// such as `PlaneSlice` instead of just nesting `Plane.Slice`.
        struct Extend final {

        };

        /// Type alias declaration.
        struct Type final {

        };

        struct TypeOperator final {

        };

        /// Class declaration.
        ///
        /// Classes are a special kind of declaration which isn't useful on its own. Unlike most languages
        /// Strawberry does not come with a base class, instead requiring the hierarchy root to be implemented
        /// manually using compile time reflection for erasure.
        ///
        /// ```str
        /// base class Foo {
        ///     pub init
        /// }
        /// ```
        ///
        /// Classes are initialized with a new expression which is lightweight sugar for the backing container.
        ///
        /// ```str
        /// let x: Base = new(in: allocator) Instance(x: 0, y: 0) // Class syntax.
        /// let x: BackingErasureType = BackingErasureType(Instance(x: 0, y: 0), in: allocator) // Effective desugaring.
        /// ```
        struct Class final {
            std::string_view name;
        };

        struct Member final {

            bool conditionally_mutable;
        };

        struct Object final {

        };

        struct Annotation final {

        };

        struct Import final {
            Path path;
            explicit Import(Path path) : path(std::move(path)) {}
        };

        using Data = std::variant<
            Fun,
            Init,
            Deinit,
            Struct,
            Enum,
            Category,
            Extend,
            Type,
            Class,
            Member,
            Object,
            Annotation,
            Import
        >;

        /// The visiblity modifier is `pub` but it also allows variants `pub(get)` and `pub(set)`
        /// which only make read-only or write-only access public respectively.
        enum class Visibility {
            /// Read-write access, the modifier `pub`.
            Pub,
            /// Read-only access, the modifier `pub(get)`.
            PubGet,
            /// Write-only access, the modifier `pub(set)`.
            PubSet
        };

        /// A declaration attached annotation.
        struct AnnotationAttachment final {
            /// The annotation expression itself.
            Expr::ExprBox annotation;
            /// If true the annotation was attached with `@!` syntax instead of `@`.
            /// The exclamation syntax is used to invoke unsafe annotation initializers.
            bool unsafe;

            AnnotationAttachment(auto annotation, bool unsafe) : annotation(Expr::box(annotation)), unsafe(unsafe) {}
        };

        Data data;
        Provenance provenance;

        Decl(Provenance provenance, Data data) : data(std::move(data)), provenance(provenance) {}

        std::optional<std::string> documentation;
        std::vector<AnnotationAttachment> annotations;

        Visibility mod_visibility;
        bool mod_unsafe = false;
        bool mod_open = false;
        bool mod_override = false;
        bool mod_inherent = false;
        bool mod_const = false;
        bool mod_static = false;
        bool mod_inline = false;
        bool mod_implicit = false;
        bool mod_final = false;
        bool mod_base = false;

        template <typename T> auto get() -> T& {
            return std::get<T>(data);
        }

        template <typename T> auto get_as() -> T* {
            if (std::holds_alternative<T>(data)) {
                return &std::get<T>(data);
            } else {
                return nullptr;
            }
        }

        template <typename T> auto get() const -> T const& {
            return std::get<T>(data);
        }

        template <typename T> auto get_as() const -> T const* {
            if (std::holds_alternative<T>(data)) {
                return &std::get<T>(data);
            } else {
                return nullptr;
            }
        }
    };

    class Ast final {
      public:
        std::vector<Decl> decls;
        Path module;
        std::string_view source;

        Ast(std::vector<Decl> decls, Path module, std::string_view source)
            : decls(std::move(decls)), module(std::move(module)), source(source) {}
    };
}

template <> struct std::formatter<str::Expr, char> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() and *it != '}') throw std::format_error("invalid format args for str::Expr");
        return it;
    }

    constexpr auto format(str::Expr const& expr, std::format_context& ctx) const {
        throw std::runtime_error("todo");
    }
};

template <> struct std::formatter<str::Decl, char> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() and *it != '}') throw std::format_error("invalid format args for str::Decl");
        return it;
    }

    constexpr auto format(str::Decl const& decl, std::format_context& ctx) const {
        throw std::runtime_error("todo");
    }
};

namespace str {
    class Parser final {
      public:
        // The parsing procedures are in a class because C++ can't call global procedures out of declaration order.
        // That is fine with forward declarations until default arguments are involved, at which point they
        // are only written in one place, forcing jumps between files to know the default values for arguments.
        // That is exceptionally stupid but everything just works inside a class so it's okay.

        /// Parses the initial module header at the very top of the file.
        auto parse_module_header(TokenStream& tokens) -> Path {
            tokens.expect<Token::Identifier>("module");

            // Base path.
            Path path = tokens.expect_as<Token::Identifier>().content;

            // Submodule path.
            while (tokens.match<Token::Dot>()) {
                path += tokens.expect_as<Token::Identifier>().content;
            }

            // Required newline.
            tokens.expect<Token::NewLine>();

            return path;
        }

        auto parse_block_expr(TokenStream& tokens) -> Expr {
            auto span = tokens.span();

            tokens.expect<Token::BraceLeft>();

            std::vector<Expr::ExprBox> expressions;

            while (true) {
                tokens.drop_while(&Token::is<Token::NewLine>);

                if (tokens.match<Token::BraceRight>()) break;
                if (tokens.finished()) throw Diagnostic::error(tokens.failure_provenance(),
                    "unexpected end of stream"
                );

              immediate_chain:
                expressions.emplace_back(Expr::box(parse_expr(tokens)));

                // Semicolons require another expression on the same line, they are not
                // allowed without reason at the end of lines to catch accidental semicolons
                // instinctively left by multilingual developers after using a semicolon terminated language.
                if (tokens.match<Token::SemiColon>()) goto immediate_chain;
            }

            return Expr(span.take(), Expr::Block(std::move(expressions)));
        }

        auto parse_pattern_expr(TokenStream& tokens, bool top_level = true) -> Expr {
            auto span = tokens.span();

            std::optional<std::string_view> case_name;
            std::vector<Expr::ExprBox> elements;
            bool tuple_or_enum = false;

            if (tokens.match<Token::Dot>()) {
                case_name = tokens.expect_as<Token::Identifier>().content;
                tuple_or_enum = true;
            }

            if (tokens.match<Token::ParenLeft>()) {
                tuple_or_enum = true;

                if (not tokens.peek_match<Token::ParenRight>()) {
                    do {

                        if (auto wildcard = tokens.match<Token::Identifier>("_")) {
                            elements.push_back(Expr::box(Expr(*wildcard, Expr::Wildcard())));
                        } else if (auto let = tokens.match<Token::Identifier>("let")) {
                            auto post_let_span = tokens.span();
                            bool mut = (bool) tokens.match<Token::Identifier>("mut");
                            bool ref = (bool) tokens.match<Token::Symbolic>("&");

                            std::string_view name = tokens.expect_as<Token::Identifier>().content;
                            std::optional<Expr::ExprBox> type_expr;

                            if (tokens.match<Token::Colon>()) {
                                type_expr = Expr::box(parse_expr(tokens, "="));
                            }

                            elements.emplace_back(Expr::box(Expr(
                                { Provenance(*let), post_let_span.take() },
                                Expr::PatternBinding(mut, ref, name, std::move(type_expr))
                            )));
                        } else {
                            elements.emplace_back(Expr::box(parse_pattern_expr(tokens, false)));
                        }

                    } while (tokens.match<Token::Comma>());
                }

                tokens.expect<Token::ParenRight>();
            }

            Expr::Pattern::Data data;
            if (case_name) {
                data = Expr::Pattern::Enum(*case_name, std::move(elements));
            } else if (tuple_or_enum) {
                data = Expr::Pattern::Tuple(std::move(elements));
            } else {
                data = Expr::Pattern::Value(Expr::box(parse_expr(tokens)));
            }

            std::optional<Expr::ExprBox> rhs;
            std::optional<Expr::ExprBox> where_clause;

            if (top_level) {
                if (auto assignment = tokens.match_as<Token::Symbolic>("=")) {
                    rhs = Expr::box(parse_expr(tokens));
                }

                if (tokens.match<Token::Identifier>("where")) {
                    where_clause = Expr::box(parse_expr(tokens));
                }
            }

            return Expr(
                span.take(),
                Expr::Pattern(
                    std::move(data),
                    std::move(rhs),
                    std::move(where_clause)
                )
            );
        }

        auto parse_destructuring(TokenStream& tokens) -> Expr::Destructuring {
            if (tokens.match<Token::ParenLeft>()) {
                std::vector<Expr::Destructuring> elements;

                if (not tokens.peek_match<Token::ParenRight>()) {
                    do {
                        elements.emplace_back(parse_destructuring(tokens));
                    } while (tokens.match<Token::Comma>());
                }

                tokens.expect<Token::ParenRight>();

                return Expr::Destructuring(Expr::Destructuring::Tuple(std::move(elements)));
            } else {
                bool mut = (bool) tokens.match<Token::Identifier>("mut");
                bool ref = (bool) tokens.match<Token::Symbolic>("&");
                std::string_view name = tokens.expect_as<Token::Identifier>().content;
                std::optional<Expr::ExprBox> type_expr;

                if (tokens.match<Token::Colon>()) type_expr = Expr::box(parse_expr(tokens, "="));

                return Expr::Destructuring(Expr::Destructuring::Binding(mut, ref, name, std::move(type_expr)));
            }
        }

        auto parse_prefix_expr(TokenStream& tokens) -> Expr {
            auto span = tokens.span();

            if (tokens.match<Token::Pound>()) {
                std::optional<std::string_view> backend;
                std::string_view name = tokens.expect_as<Token::Identifier>().content;

                if (tokens.match<Token::Dot>()) {
                    backend = name;
                    name = tokens.expect_as<Token::Identifier>().content;
                }

                tokens.expect<Token::ParenLeft>();

                std::vector<Expr::ExprBox> arguments;

                if (not tokens.peek_match<Token::ParenRight>()) {
                    do {
                        tokens.allow<Token::NewLine>();
                        if (tokens.peek_match<Token::ParenRight>()) break;

                        arguments.emplace_back(Expr::box(parse_expr(tokens)));
                    } while (tokens.match<Token::Comma>());
                }

                tokens.allow<Token::NewLine>();
                tokens.expect<Token::ParenRight>();

                return Expr(span.take(), Expr::Intrinsic(backend, name, std::move(arguments)));
            }

            if (tokens.match<Token::Identifier>("_")) {
                return Expr(span.take(), Expr::Wildcard());
            }

            if (tokens.match<Token::Identifier>("true")) {
                return Expr(span.take(), Expr::Boolean(true));
            }

            if (tokens.match<Token::Identifier>("false")) {
                return Expr(span.take(), Expr::Boolean(false));
            }

            if (tokens.match<Token::Identifier>("let")) {
                if (not tokens.peek_match<Token::ParenLeft>()) {
                    // Normal named binding.
                    bool mut = (bool) tokens.match<Token::Identifier>("mut");
                    bool ref = (bool) tokens.match<Token::Symbolic>("&");
                    std::string_view name = tokens.expect_as<Token::Identifier>().content;

                    std::optional<Expr::ExprBox> type_expr;
                    if (tokens.match<Token::Colon>()) {
                        type_expr = Expr::box(parse_expr(tokens, "="));
                    }

                    std::optional<Expr::ExprBox> rhs;
                    if (tokens.match<Token::Symbolic>("=")) {
                        rhs = Expr::box(parse_expr(tokens));
                    }

                    return Expr(span.take(), Expr::Binding(mut, ref, name, std::move(type_expr), std::move(rhs)));
                } else {
                    Expr::Destructuring pattern = parse_destructuring(tokens);

                    std::optional<Expr::ExprBox> rhs;
                    if (tokens.match<Token::Symbolic>("=")) rhs = Expr::box(parse_expr(tokens));

                    return Expr(span.take(), Expr::DestructuringBinding(std::move(pattern), std::move(rhs)));
                }
            }

            if (tokens.match<Token::Identifier>("loop")) {
                Expr body = parse_expr(tokens);
                return Expr(span.take(), Expr::Loop(std::move(body)));
            }

            if (tokens.match<Token::Identifier>("while")) {
                Expr pattern = parse_pattern_expr(tokens);

                Expr body = tokens.match<Token::Identifier>("do")
                    ? parse_expr(tokens)
                    : parse_block_expr(tokens);

                return Expr(span.take(), Expr::While(std::move(pattern), std::move(body)));
            }

            if (tokens.match<Token::Identifier>("for")) {
                Expr::Destructuring pattern = parse_destructuring(tokens);

                tokens.expect<Token::Identifier>("in");

                Expr iterator = parse_expr(tokens);

                std::optional<Expr::ExprBox> where_clause;
                if (tokens.match<Token::Identifier>("where")) where_clause = Expr::box(parse_expr(tokens));

                Expr body = tokens.match<Token::Identifier>("do")
                    ? parse_expr(tokens)
                    : parse_block_expr(tokens);

                std::optional<Expr::Destructuring> else_pattern;
                std::optional<Expr::ExprBox> else_body;

                if (tokens.match<Token::Identifier>("else")) {
                    if (tokens.peek_match<Token::BraceLeft>()) {
                        else_body = Expr::box(parse_block_expr(tokens));
                    } else if (tokens.match<Token::Identifier>("do")) {
                        else_body = Expr::box(parse_expr(tokens));
                    } else {
                        else_pattern = parse_destructuring(tokens);

                        else_body = Expr::box(
                            tokens.match<Token::Identifier>("do")
                                ? parse_expr(tokens)
                                : parse_block_expr(tokens)
                        );
                    }
                }

                return Expr(
                    span.take(),
                    Expr::For(
                        std::move(pattern),
                        std::move(iterator),
                        std::move(where_clause),
                        std::move(body),
                        std::move(else_pattern),
                        std::move(else_body)
                    )
                );
            }

            if (tokens.match<Token::Identifier>("unsafe")) {
                Expr expr = parse_expr(tokens);
                return Expr(span.take(), Expr::Unsafe(std::move(expr)));
            }

            if (tokens.match<Token::Identifier>("recurse")) {
                Expr expr = parse_expr(tokens);
                return Expr(span.take(), Expr::Recurse(std::move(expr)));
            }

            if (tokens.match<Token::Identifier>("return")) {
                if (
                    tokens.peek_match<Token::NewLine>() or
                    tokens.peek_match<Token::BraceRight>()
                ) {
                    return Expr(span.take(), Expr::Return());
                } else {
                    Expr expr = parse_expr(tokens);
                    return Expr(span.take(), Expr::Return(std::move(expr)));
                }
            }

            if (tokens.match<Token::Identifier>("yield")) {
                Expr expr = parse_expr(tokens);
                return Expr(span.take(), Expr::Yield(std::move(expr)));
            }

            if (tokens.match<Token::Colon>()) {
                std::string_view name = tokens.expect_as<Token::Identifier>().content;
                Expr expr = parse_expr(tokens);
                return Expr(span.take(), Expr::Label(name, std::move(expr)));
            }

            if (tokens.match<Token::Identifier>("break")) {
                std::optional<std::string_view> label;
                if (tokens.match<Token::Colon>()) label = tokens.expect_as<Token::Identifier>().content;

                if (
                    tokens.peek_match<Token::NewLine>() or
                    tokens.peek_match<Token::SemiColon>() or
                    tokens.peek_match<Token::BraceRight>()
                ) {
                    return Expr(span.take(), Expr::Break(label));
                } else {
                    Expr expr = parse_expr(tokens);
                    return Expr(span.take(), Expr::Break(label, std::move(expr)));
                }
            }

            if (tokens.match<Token::Identifier>("continue")) {
                if (tokens.match<Token::Colon>()) {
                    std::string_view label = tokens.expect_as<Token::Identifier>().content;
                    return Expr(span.take(), Expr::Continue(label));
                } else {
                    return Expr(span.take(), Expr::Continue());
                }
            }

            if (tokens.match<Token::Identifier>("throw")) {
                Expr expr = parse_expr(tokens);
                return Expr(span.take(), Expr::Throw(std::move(expr)));
            }

            if (tokens.match<Token::Identifier>("await")) {
                Expr expr = parse_expr(tokens);
                return Expr(span.take(), Expr::Await(std::move(expr)));
            }

            if (tokens.match<Token::Dot>()) {
                std::string_view name = tokens.expect_as<Token::Identifier>().content;
                return Expr(span.take(), Expr::MemberInfer(name));
            }

            if (tokens.match<Token::Identifier>("if")) {
                // Technically legal syntax but it doesn't add any value.
                if (auto illegal = tokens.peek_match<Token::Identifier>("when")) {
                    throw Diagnostic::error(*illegal, "redundant when nested inside if");
                }

                Expr pattern = parse_pattern_expr(tokens);

                Expr body = tokens.match<Token::Identifier>("then")
                    ? parse_expr(tokens)
                    : parse_block_expr(tokens);

                if (tokens.match<Token::Identifier>("else")) {
                    Expr else_body = parse_expr(tokens);
                    return Expr(span.take(), Expr::If(std::move(pattern), std::move(body), std::move(else_body)));
                } else {
                    return Expr(span.take(), Expr::If(std::move(pattern), std::move(body)));
                }
            }

            if (tokens.match<Token::Identifier>("guard")) {
                Expr pattern = parse_pattern_expr(tokens);
                tokens.expect<Token::Identifier>("else");
                Expr else_body = parse_expr(tokens);
                return Expr(span.take(), Expr::Guard(std::move(pattern), std::move(else_body)));
            }

            if (tokens.match<Token::Identifier>("when")) {
                Expr pattern = parse_pattern_expr(tokens);
                return Expr(span.take(), Expr::When(std::move(pattern)));
            }

            if (tokens.peek_match<Token::BraceLeft>()) {
                (void) span.take();
                return parse_block_expr(tokens);
            }

            if (auto number = tokens.match_as<Token::Number>()) {
                return Expr(span.take(), Expr::Number(number->content));
            }

            if (auto string = tokens.match_as<Token::String>()) {
                return Expr(span.take(), Expr::String(std::string(string->content)));
            }

            if (auto string_base = tokens.match_as<Token::MultilineString>()) {
                std::string string = std::string(string_base->content);

                bool past_newline = false;
                while (true) {
                    if (auto string_continuation = tokens.match_as<Token::MultilineString>()) {
                        past_newline = false;
                        string += "\n";
                        string += string_continuation->content;
                    } else if (tokens.allow<Token::NewLine>() and not past_newline) {
                        past_newline = true;
                    } else {
                        break;
                    }
                }

                return Expr(span.take(), Expr::String(std::move(string)));
            }

            if (tokens.match<Token::ParenLeft>()) {
                // The special case of an empty tuple.
                if (tokens.match<Token::ParenRight>()) {
                    return Expr(span.take(), Expr::Tuple());
                }

                std::vector<Expr::Tuple::Element> elements;
                bool is_tuple = false;

                tokens.allow<Token::NewLine>();

                while (true) {
                    std::optional<std::string_view> label;

                    auto next_1 = tokens.peek(1);
                    auto next_2 = tokens.peek(2);

                    if (
                        next_1 and next_1->is<Token::Identifier>() and
                        next_2 and next_2->is<Token::Colon>()
                    ) {
                        label = tokens.expect_as<Token::Identifier>().content;
                        tokens.expect<Token::Colon>();

                        is_tuple = true;
                    }

                    elements.emplace_back(label, Expr::box(parse_expr(tokens)));

                    if (tokens.match<Token::Comma>()) {
                        is_tuple = true;
                    } else {
                        break;
                    }
                }

                tokens.allow<Token::NewLine>();
                tokens.expect<Token::ParenRight>();

                // If not a tuple it's just grouping. Precedence is handled inherently by how the parser works,
                // no Ast transformations ever occur after parsing, so we just discard this information.
                if (is_tuple) {
                    return Expr(span.take(), Expr::Tuple(std::move(elements)));
                } else {
                    (void) span.take(); // We do not need it.
                    return std::move(*elements.front().expr);
                }
            }

            if (tokens.match<Token::BracketLeft>()) {
                // List literal.
                throw tokens.todo();
            }

            if (tokens.match<Token::Pipe>()) {
                std::vector<Expr::Closure::Argument> arguments;

                if (not tokens.peek_match<Token::Pipe>()) {
                    do {
                        bool mut = (bool) tokens.match<Token::Identifier>("mut");
                        bool ref = (bool) tokens.match<Token::Symbolic>("&");
                        std::string_view name = tokens.expect_as<Token::Identifier>().content;

                        std::optional<Expr::ExprBox> type_expr;
                        if (tokens.match<Token::Colon>()) type_expr = Expr::box(parse_expr(tokens, "="));

                        arguments.emplace_back(mut, ref, name, std::move(type_expr));
                    } while (tokens.match<Token::Comma>());
                }

                tokens.expect<Token::Pipe>();

                std::optional<std::vector<Expr::Closure::Capture>> captures;
                if (tokens.match<Token::Colon>()) {
                    tokens.expect<Token::BracketLeft>();
                    captures.emplace();

                    if (not tokens.peek_match<Token::BracketRight>()) {
                        do {
                            bool mut = (bool) tokens.match<Token::Identifier>("mut");
                            bool ref = (bool) tokens.match<Token::Symbolic>("&");
                            std::string_view name = tokens.expect_as<Token::Identifier>().content;
                            std::optional<Expr::ExprBox> init_expr;

                            if (tokens.peek_match<Token::Dot>()) init_expr = Expr::box(parse_expr(tokens));

                            captures->emplace_back(mut, ref, name, std::move(init_expr));
                        } while (tokens.match<Token::Comma>());
                    }

                    tokens.expect<Token::BracketRight>();
                }

                bool async = (bool) tokens.match<Token::Identifier>("async");

                std::optional<std::vector<Expr::ExprBox>> throws;
                if (tokens.match<Token::Identifier>("throws")) {
                    throws.emplace();

                    do {
                        throws->emplace_back(Expr::box(parse_expr(tokens, ",")));
                    } while (tokens.match<Token::Comma>());
                }

                std::optional<Expr::ExprBox> return_type;
                if (tokens.match<Token::Arrow>()) {
                    return_type = Expr::box(parse_expr(tokens));
                }

                Expr body = parse_expr(tokens);

                return Expr(
                    span.take(),
                    Expr::Closure(
                        std::move(arguments),
                        std::move(captures),
                        std::move(throws),
                        std::move(return_type),
                        async,
                        std::move(body)
                    )
                );
            }

            // Annotates an expression with one or more annotations.
            // type Foo = @Convention("c") @!Bar ():[] -> ()
            if (tokens.match<Token::At>()) {
                std::vector<Expr::Annotations::Annotation> annotations;

                do {
                    bool unsafe = false;
                    if (tokens.match<Token::Symbolic>("!")) unsafe = true;

                    // In order to disambiguate elided parentheses we can use whitespace.
                    if (
                        auto identifier = tokens.peek_match_as<Token::Identifier>();
                        identifier and identifier->trailing_whitespace
                    ) {
                        auto token = tokens.expect<Token::Identifier>();
                        Expr annotation = Expr(token, Expr::Identifier(identifier->content));
                        annotations.emplace_back(std::move(annotation), unsafe);
                    } else {
                        Expr annotation = parse_expr(tokens);
                        annotations.emplace_back(std::move(annotation), unsafe);
                    }
                } while (tokens.match<Token::At>());

                Expr expr = parse_expr(tokens);

                return Expr(span.take(), Expr::Annotations(std::move(annotations), std::move(expr)));
            }

            if (auto symbolic = tokens.match_as<Token::Symbolic>([] (auto symbolic) {
                return not symbolic.trailing_whitespace;
            })) {
                auto rhs = parse_expr(tokens);
                return Expr(span.take(), Expr::Prefix(symbolic->content, std::move(rhs)));
            }

            // The not identifier is the only allowed non symbolic prefix operator in order to
            // provide familiar DNF rules for boolean logic.
            if (tokens.match<Token::Identifier>("not")) {
                auto rhs = parse_expr(tokens);
                return Expr(span.take(), Expr::Prefix("not", std::move(rhs)));
            }

            if (auto identifier = tokens.match_as<Token::Identifier>()) {
                return Expr(span.take(), Expr::Identifier(identifier->content));
            }

            throw Diagnostic::error(tokens.fallthrough_provenance(), "expected expression");
        }

        /// Once a prefix expression ends we still try to keep going to match postfix expressions.
        auto parse_postfix_expr(Expr lhs, TokenStream& tokens) -> Expr {
            while (true) {
                if (tokens.finished()) break;

                if (tokens.peek_match<Token::Tick>()) {
                    std::vector<std::string_view> lifetimes;

                    do {
                        lifetimes.emplace_back(tokens.expect_as<Token::Identifier>().content);
                    } while (tokens.match<Token::Tick>());

                    auto provenance = Provenance(lhs.provenance, tokens.last_provenance());

                    lhs = Expr(provenance, Expr::Lifetimes(std::move(lifetimes)));
                } else if (tokens.match<Token::ParenLeft>()) {
                    std::vector<Expr::Call::Argument> arguments;

                    if (not tokens.peek_match<Token::ParenRight>()) {
                        do {
                            tokens.allow<Token::NewLine>();
                            if (tokens.peek_match<Token::ParenRight>()) break;

                            std::optional<std::string_view> label;
                            auto next_1 = tokens.peek(1);
                            auto next_2 = tokens.peek(2);

                            if (next_1 and next_1->is<Token::Identifier>() and next_2 and next_2->is<Token::Colon>()) {
                                label = tokens.expect_as<Token::Identifier>().content;
                                tokens.expect<Token::Colon>();
                            }

                            arguments.emplace_back(label, Expr::box(parse_expr(tokens)));
                        } while (tokens.match<Token::Comma>());
                    }

                    tokens.allow<Token::NewLine>();

                    auto closing = tokens.expect<Token::ParenRight>();
                    auto provenance = Provenance(lhs.provenance, Provenance(closing));

                    lhs = Expr(provenance, Expr::Call(std::move(lhs), std::move(arguments)));
                } else if (tokens.match<Token::Dot>()) {
                    auto name_token = tokens.expect<Token::Identifier>();
                    std::string_view name = name_token.get<Token::Identifier>().content;

                    auto provenance = Provenance(lhs.provenance, Provenance(name_token));

                    lhs = Expr(provenance, Expr::Member(name, std::move(lhs)));
                } else if (tokens.match<Token::Colon>()) {
                    tokens.expect<Token::Colon>();

                    auto name_token = tokens.expect<Token::Identifier>();
                    std::string_view name = name_token.get<Token::Identifier>().content;

                    auto provenance = Provenance(lhs.provenance, Provenance(name_token));

                    lhs = Expr(provenance, Expr::MetaMember(name, std::move(lhs)));
                } else if (tokens.match<Token::BracketLeft>()) {
                    std::vector<Expr::Subscript::Argument> arguments;

                    if (not tokens.peek_match<Token::BracketRight>()) {
                        do {
                            tokens.allow<Token::NewLine>();
                            if (tokens.peek_match<Token::BracketRight>()) break;

                            std::optional<std::string_view> label;
                            auto next_1 = tokens.peek(1);
                            auto next_2 = tokens.peek(2);

                            if (next_1 and next_1->is<Token::Identifier>() and next_2 and next_2->is<Token::Colon>()) {
                                label = tokens.expect_as<Token::Identifier>().content;
                                tokens.expect<Token::Colon>();
                            }

                            arguments.emplace_back(label, Expr::box(parse_expr(tokens)));
                        } while (tokens.match<Token::Comma>());
                    }

                    tokens.allow<Token::NewLine>();

                    auto closing = tokens.expect<Token::BracketRight>();
                    auto provenance = Provenance(lhs.provenance, Provenance(closing));

                    lhs = Expr(provenance, Expr::Subscript(std::move(lhs), std::move(arguments)));
                } else if (tokens.match<Token::Identifier>("match")) {
                    tokens.expect<Token::BraceLeft>();

                    std::vector<Expr::Match::Arm> arms;

                    while (not tokens.match<Token::BraceRight>()) {
                        tokens.drop_while(&Token::is<Token::NewLine>);

                        if (tokens.peek_match<Token::BraceRight>()) break;

                        Expr pattern = parse_pattern_expr(tokens);
                        tokens.expect<Token::Arrow>();
                        Expr body = parse_expr(tokens);

                        arms.emplace_back(Expr::box(std::move(pattern)), Expr::box(std::move(body)));

                        if (not tokens.allow<Token::NewLine>()) tokens.allow<Token::SemiColon>();
                    }

                    auto provenance = Provenance(lhs.provenance, tokens.last_provenance());

                    lhs = Expr(provenance, Expr::Match(std::move(lhs), std::move(arms)));
                } else if (tokens.match<Token::Identifier>("catch")) {
                    tokens.expect<Token::BraceLeft>();

                    std::vector<Expr::Catch::Arm> arms;

                    while (not tokens.match<Token::BraceRight>()) {
                        tokens.drop_while(&Token::is<Token::NewLine>);

                        if (tokens.peek_match<Token::BraceRight>()) break;

                        auto name = tokens.expect_as<Token::Identifier>().content;
                        tokens.expect<Token::Colon>();
                        Expr type_expr = parse_expr(tokens);

                        std::optional<Expr::ExprBox> where_clause;
                        if (tokens.match<Token::Identifier>("where")) where_clause = Expr::box(parse_expr(tokens));

                        tokens.expect<Token::Arrow>();
                        Expr body = parse_expr(tokens);

                        arms.emplace_back(name, Expr::box(type_expr), std::move(where_clause), Expr::box(body));

                        if (not tokens.allow<Token::NewLine>()) tokens.allow<Token::SemiColon>();
                    }

                    auto provenance = Provenance(lhs.provenance, tokens.last_provenance());

                    lhs = Expr(provenance, Expr::Catch(std::move(lhs), std::move(arms)));
                 } else if (tokens.match<Token::Symbolic>("<")) {
                    std::vector<Expr::Generics::Parameter> parameters;

                    if (not tokens.peek_match<Token::Symbolic>(">")) {
                        do {
                            tokens.allow<Token::NewLine>();
                            if (tokens.peek_match<Token::Symbolic>(">")) break;

                            std::optional<std::string_view> label;
                            auto next_1 = tokens.peek(1);
                            auto next_2 = tokens.peek(2);

                            if (next_1 and next_1->is<Token::Identifier>() and next_2 and next_2->is<Token::Colon>()) {
                                label = tokens.expect_as<Token::Identifier>().content;
                                tokens.expect<Token::Colon>();
                            }

                            parameters.emplace_back(label, Expr::box(parse_expr(tokens)));
                        } while (tokens.match<Token::Comma>());
                    }

                    tokens.allow<Token::NewLine>();

                    auto closing = tokens.expect<Token::Symbolic>(">");
                    auto provenance = Provenance(lhs.provenance, Provenance(closing));

                    lhs = Expr(provenance, Expr::Generics(std::move(lhs), std::move(parameters)));
                } else if (tokens.peek_match<Token::Pipe>()) {
                    Expr closure = parse_prefix_expr(tokens);

                    auto provenance = Provenance(lhs.provenance, closure.provenance);

                    lhs = Expr(
                        provenance,
                        Expr::TrailingClosure(std::move(lhs), std::move(closure))
                    );
                } else if (tokens.peek_match<Token::BraceLeft>()) {
                    Expr block = parse_prefix_expr(tokens);
                    auto block_provenance = block.provenance;
                    Expr closure = Expr(block_provenance, Expr::Closure(std::move(block)));

                    auto provenance = Provenance(lhs.provenance, closure.provenance);

                    lhs = Expr(
                        provenance,
                        Expr::TrailingClosure(std::move(lhs), std::move(closure))
                    );
                } else if (auto symbolic = tokens.match<Token::Symbolic>([] (auto symbolic) {
                    return not symbolic.leading_whitespace and symbolic.content != ">";
                })) {
                    auto provenance = Provenance(lhs.provenance, Provenance(*symbolic));

                    lhs = Expr(
                        provenance,
                        Expr::Postfix(symbolic->get<Token::Symbolic>().content, std::move(lhs))
                    );
                } else {
                    break;
                }
            }

            return lhs;
        }

        template <std::convertible_to<std::string_view>... Arg> auto parse_expr(
            TokenStream& tokens,
            Arg... excluded_ops
        ) -> Expr requires (sizeof...(Arg) > 0) {
            return parse_expr(tokens, 0, excluded_ops...);
        }

        template <std::convertible_to<std::string_view>... Arg> auto parse_expr(
            TokenStream& tokens,
            usize minimum_precedence = 0,
            Arg... excluded_ops
        ) -> Expr {
            Expr lhs = parse_postfix_expr(parse_prefix_expr(tokens), tokens);

            while (true) {
                auto op = tokens.peek();
                if (not op) break;
                if (not op->is<Token::Identifier>() and not op->is<Token::Symbolic>()) break;
                if (op->is<Token::Identifier>()) {
                    auto content = op->identifier_content().value();
                    if (content != "and" and content != "or") break;
                }
                if (operator_role(*op) != OperatorRole::Infix) break;
                if (((op->identifier_content() == excluded_ops) or ...)) break;

                usize prec = precedence(op->identifier_content().value());
                if (prec < minimum_precedence) break;

                tokens.drop();

                Expr rhs = parse_expr(tokens, prec + 1);

                Provenance provenance = tokens.generic_provenance();
                if (
                    std::holds_alternative<Provenance::Span>(lhs.provenance.data) and
                    std::holds_alternative<Provenance::Span>(rhs.provenance.data)
                ) {
                    auto lhs_span = std::get<Provenance::Span>(lhs.provenance.data);
                    auto rhs_span = std::get<Provenance::Span>(rhs.provenance.data);
                    provenance = Provenance(lhs_span.start, rhs_span.end);
                }

                lhs = Expr(
                    provenance,
                    Expr::Infix(op->identifier_content().value(), std::move(lhs), std::move(rhs))
                );
            }

            return lhs;
        }

        auto parse_fun(TokenStream& tokens) -> Decl {
            throw tokens.todo();
        }

        auto parse_init(TokenStream& tokens) -> Decl {
            throw tokens.todo();
        }

        auto parse_deinit(TokenStream& tokens) -> Decl {
            throw tokens.todo();
        }

        auto parse_struct(TokenStream& tokens) -> Decl {
            throw tokens.todo();
        }

        auto parse_enum(TokenStream& tokens) -> Decl {
            throw tokens.todo();
        }

        auto parse_category(TokenStream& tokens) -> Decl {
            throw tokens.todo();
        }

        auto parse_extend(TokenStream& tokens) -> Decl {
            throw tokens.todo();
        }

        auto parse_type(TokenStream& tokens) -> Decl {
            throw tokens.todo();
        }

        auto parse_class(TokenStream& tokens) -> Decl {
            throw tokens.todo();
        }

        auto parse_member(TokenStream& tokens) -> Decl {
            throw tokens.todo();
        }

        auto parse_object(TokenStream& tokens) -> Decl {
            throw tokens.todo();
        }

        auto parse_annotation(TokenStream& tokens) -> Decl {
            throw tokens.todo();
        }

        auto parse_import(TokenStream& tokens) -> Decl {
            auto span = tokens.span();
            tokens.expect<Token::Identifier>("import");

            // Base path.
            Path path = tokens.expect_as<Token::Identifier>().content;

            // Submodule path.
            while (tokens.match<Token::Dot>()) {
                path += tokens.expect_as<Token::Identifier>().content;
            }

            // Required newline.
            tokens.expect<Token::NewLine>();

            return Decl(
                span.take(),
                Decl::Import(std::move(path))
            );
        }

        auto parse_decl(TokenStream& tokens) -> Decl {
            std::vector<std::string_view> docs;

            while (auto doc = tokens.match_as<Token::Comment>([] (auto comment) { return comment.selector == "/"; })) {
                docs.push_back(doc->content);
                tokens.expect<Token::NewLine>();
            }

            std::optional<std::string> documentation;
            if (not docs.empty()) documentation = docs
                | std::views::join_with('\n')
                | std::ranges::to<std::string>();

            std::vector<Decl::AnnotationAttachment> annotations;

            while (tokens.match<Token::At>()) {
                bool unsafe = false;
                if (tokens.match<Token::Symbolic>("!")) unsafe = true;

                Expr expr = parse_expr(tokens);
                annotations.emplace_back(std::move(expr), unsafe);
                tokens.expect<Token::NewLine>();
            }

            Decl::Visibility mod_visibility;
            if (tokens.match<Token::Identifier>("pub")) {
                if (tokens.match<Token::ParenLeft>()) {
                    auto token = tokens.expect<Token::Identifier>();
                    std::string_view mode = token.get<Token::Identifier>().content;

                    if      (mode == "get") mod_visibility = Decl::Visibility::PubGet;
                    else if (mode == "set") mod_visibility = Decl::Visibility::PubSet;
                    else throw Diagnostic::error(token,
                        "visibility can only be refined to `get` or `set`"
                    );

                    tokens.expect<Token::ParenRight>();
                } else {
                    mod_visibility = Decl::Visibility::Pub;
                }
            }

            bool mod_unsafe   = (bool) tokens.match<Token::Identifier>("unsafe");
            bool mod_open     = (bool) tokens.match<Token::Identifier>("open");
            bool mod_override = (bool) tokens.match<Token::Identifier>("override");
            bool mod_inherent = (bool) tokens.match<Token::Identifier>("inherent");
            bool mod_const    = (bool) tokens.match<Token::Identifier>("const");
            bool mod_static   = (bool) tokens.match<Token::Identifier>("static");
            bool mod_inline   = (bool) tokens.match<Token::Identifier>("inline");
            bool mod_implicit = (bool) tokens.match<Token::Identifier>("implicit");
            bool mod_final    = (bool) tokens.match<Token::Identifier>("final");
            bool mod_base     = (bool) tokens.match<Token::Identifier>("base");

            std::string_view next = tokens.peek_expect_as<Token::Identifier>().content;

            std::optional<Decl> decl;
            if (
                next == "fun" or
                next == "infix" or
                next == "prefix" or
                next == "postfix"
            ) {
                decl = parse_fun(tokens);
            } else if (next == "init") {
                decl = parse_init(tokens);
            } else if (next == "deinit") {
                decl = parse_deinit(tokens);
            } else if (next == "struct") {
                decl = parse_struct(tokens);
            } else if (next == "enum") {
                decl = parse_enum(tokens);
            } else if (next == "category") {
                decl = parse_category(tokens);
            } else if (next == "extend") {
                decl = parse_extend(tokens);
            } else if (next == "type") {
                decl = parse_type(tokens);
            } else if (next == "class") {
                decl = parse_class(tokens);
            } else if (next == "let") {
                decl = parse_member(tokens);
            } else if (next == "object") {
                decl = parse_object(tokens);
            } else if (next == "annotation") {
                decl = parse_annotation(tokens);
            } else if (next == "import") {
                decl = parse_import(tokens);
            } else {
                throw Diagnostic::error(tokens.fallthrough_provenance(), "invalid declaration");
            }

            decl->documentation = std::move(documentation);
            decl->annotations = std::move(annotations);
            decl->mod_visibility = mod_visibility;
            decl->mod_unsafe = mod_unsafe;
            decl->mod_open = mod_open;
            decl->mod_override = mod_override;
            decl->mod_inherent = mod_inherent;
            decl->mod_const = mod_const;
            decl->mod_static = mod_static;
            decl->mod_inline = mod_inline;
            decl->mod_implicit = mod_implicit;
            decl->mod_final = mod_final;
            decl->mod_base = mod_base;

            return std::move(decl.value());
        }
    };

    inline auto parse(TokenStream& tokens) -> Ast {
        Parser parser;

        std::vector<Decl> decls;
        Path module = parser.parse_module_header(tokens);

        while (not tokens.finished()) {
            tokens.drop_while(&Token::is<Token::NewLine>);
            decls.emplace_back(parser.parse_decl(tokens));
        }

        return Ast(std::move(decls), module, tokens.get_source());
    }
}

namespace str {
    /// Hardcoded search paths for the bootstrap compiler.
    static constexpr auto MODULE_PATHS = std::to_array<std::string_view>({
        "modules", "src"
    });

    /// Hardcoded automatic imports for the bootstrap compiler.
    static constexpr auto AUTO_IMPORTS = std::to_array<std::string_view>({
        "core"
    });
}

namespace str::raw {
    /// Implementation of the `Integer` intrinsic type.
    /// The bootstrap compiler should be fine maxing out at 64 bits.
    struct Integer final {
        i64 number;
    };

    /// Implementation of the `Int` intrinsic type.
    /// The bootstrap compiler should be fine maxing out at 64 bits.
    struct Int final {
        u64 number;
        u64 size;
    };

    /// Implementation of the `Boolean` intrinsic type.
    struct Boolean final {
        bool value;
    };

    /// Implementation of the `Pointer` intrinsic type.
    struct Pointer final {

    };
}

namespace str {
    struct SourceUnit final {
        std::string text;
        std::string source;
    };

    using Modules = std::unordered_map<std::string, std::vector<Ast>>;

    /// The actual data structure we form from the Ast and operate on as we evaluate.
    /// It was named the Strawberry Intermediate Representation.
    ///
    /// The bootstrap implementation is hardcoded and does not use the plugin architecture
    /// planned for the self-hosted backends.
    class Sir final {
        Modules modules;
        std::vector<SourceUnit> source_units;
        std::vector<std::string> auto_imports;
        std::vector<Diagnostic> diagnostics;

        Sir(
            Modules modules,
            std::vector<SourceUnit> source_units,
            std::vector<std::string> auto_imports
        ) : modules(std::move(modules))
          , source_units(std::move(source_units))
          , auto_imports(std::move(auto_imports))
        {}

        friend auto evaluate(Modules modules, std::vector<SourceUnit> source_units) -> Sir;

      public:
        auto get_diagnostics() const -> std::span<const Diagnostic> {
            return diagnostics;
        }

        auto get_source_units() const -> std::span<const SourceUnit> {
            return source_units;
        }

        /// Answers true if the evaluation is erroneous, that is, if any diagnostics of error severity were raised.
        auto erroneous() const -> bool {
            for (auto const& diagnostic : diagnostics)
                if (diagnostic.severity == Diagnostic::Severity::Error) return true;
            return false;
        }

        /// An exception used to propagate multiple diagnostics at once.
        struct DiagnosticBundle final : std::exception {
            std::vector<Diagnostic> diagnostics;
            auto what() const noexcept -> char const* override { return "N/A"; }

            template <std::same_as<Diagnostic>... Diagnostic> DiagnosticBundle(Diagnostic... diagnostics)
                : diagnostics({ diagnostics... }) {}
        };

        /// The evaluator domain type of either a Type, Value or Residual.
        struct Term final {
            struct Type final {

            };

            struct Value final {

            };

            struct Residual final {

            };

            using Data = std::variant<Type, Value, Residual>;

            Data data;

            template <typename T> auto get() -> T& {
                return std::get<T>(data);
            }

            template <typename T> auto get_as() -> T* {
                if (std::holds_alternative<T>(data)) {
                    return &std::get<T>(data);
                } else {
                    return nullptr;
                }
            }

            template <typename T> auto get() const -> T const& {
                return std::get<T>(data);
            }

            template <typename T> auto get_as() const -> T const* {
                if (std::holds_alternative<T>(data)) {
                    return &std::get<T>(data);
                } else {
                    return nullptr;
                }
            }
        };

        /// As the evaluator descends it may be operating in a lexical scope, like a block. Blocks allow a few
        /// additional statement expressions which declare bindings for the duration of the lexical scope, and
        /// this type is used to represent that associated state.
        struct LexicalScope final {
            struct Binding final {
                /// A binding can be a matrix of mutability and projection class.
                /// Mutable projections are subject to mandatory definite reinitialization.
                /// Projections can of course themselves be projected.
                enum class Kind {
                    /// An owned binding `x`.
                    Let,
                    /// An owned mutable binding `mut x`.
                    LetMut,
                    /// A projected binding `&x`.
                    LetRef,
                    /// A projected mutable binding `mut &x`
                    LetRefMut,
                };

                /// The kind of the binding.
                Kind kind;
                /// The name of the binding.
                std::string_view name;
                /// The provenance of the binding.
                Provenance provenance;
                /// The meaning is as follows:
                /// - `-2` is an uninitialized binding.
                /// - `-1` is a mutably projected binding.
                /// - `0` is a balanced binding.
                /// - `1...` is a reference count of borrowing projections.
                i32 refcount;

                void consume(Provenance usage_provenance) {
                    switch (refcount) {
                        case -2: throw DiagnosticBundle(
                            Diagnostic::error(usage_provenance, std::format("binding `{}` moved while uninitialized", name)),
                            Diagnostic::info(provenance, std::format("binding `{}` located here", name))
                        );
                        case -1: throw DiagnosticBundle(
                            Diagnostic::error(usage_provenance, std::format("binding `{}` moved while mutably projected", name)),
                            Diagnostic::info(provenance, std::format("binding `{}` located here", name))
                        );
                        case 0: refcount = -2; break;
                        default: throw DiagnosticBundle(
                            Diagnostic::error(usage_provenance, std::format("binding `{}` moved while borrowed", name)),
                            Diagnostic::info(provenance, std::format("binding `{}` located here", name))
                        );
                    }
                }

                void initialize(Provenance usage_provenance) {
                    switch (refcount) {
                        case -2: refcount = 0; break;
                        case -1: throw DiagnosticBundle(
                            Diagnostic::error(usage_provenance, std::format("binding `{}` reinitialized while mutably projected", name)),
                            Diagnostic::info(provenance, std::format("binding `{}` located here", name))
                        );
                        case 0: refcount = 0; break;
                        default: throw DiagnosticBundle(
                            Diagnostic::error(usage_provenance, std::format("binding `{}` reinitialized while borrowed", name)),
                            Diagnostic::info(provenance, std::format("binding `{}` located here", name))
                        );
                    }
                }

                void borrow(Provenance usage_provenance) {
                    switch (refcount) {
                        case -2: throw DiagnosticBundle(
                            Diagnostic::error(usage_provenance, std::format("binding `{}` borrowed while uninitialized", name)),
                            Diagnostic::info(provenance, std::format("binding `{}` located here", name))
                        );
                        case -1: throw DiagnosticBundle(
                            Diagnostic::error(usage_provenance, std::format("binding `{}` borrowed while mutably projected", name)),
                            Diagnostic::info(provenance, std::format("binding `{}` located here", name))
                        );
                        default: refcount += 1; break;
                    }
                }

                void yield_borrow() {
                    if (refcount <= 0) throw std::logic_error("yielded borrow of an unborrowed binding");
                    refcount -= 1;
                }

                void mutate(Provenance usage_provenance) {
                    switch (refcount) {
                        case -2: throw DiagnosticBundle(
                            Diagnostic::error(usage_provenance, std::format("binding `{}` mutably projected while uninitialized", name)),
                            Diagnostic::info(provenance, std::format("binding `{}` located here", name))
                        );
                        case -1: throw DiagnosticBundle(
                            Diagnostic::error(usage_provenance, std::format("binding `{}` is mutably projected while already mutably projected", name)),
                            Diagnostic::info(provenance, std::format("binding `{}` located here", name))
                        );
                        case 0: refcount = -1; break;
                        default: throw DiagnosticBundle(
                            Diagnostic::error(usage_provenance, std::format("binding `{}` mutably projected while borrowed", name)),
                            Diagnostic::info(provenance, std::format("binding `{}` located here", name))
                        );
                    }
                }

                void yield_mutate() {
                    if (refcount != -1) throw std::logic_error("yielded mutable projection of an unprojected binding");
                    refcount = 0;
                }
            };

            /// All the lexical bindings in order of instantiation.
            /// Lexically bound objects are deinitialized in reverse order unless manually consumed early.
            std::vector<Binding> bindings;
            /// Determines if the scope is a boundary for consumption. For example, a loop body is a boundary
            /// because consuming across would make a binding uninitialized past the first iteration.
            /// Note that this is just a definite initialization problem, so if said loop reinitializes the binding
            /// before it ends that is perfectly valid, much like temporarily moving out of a mutable
            /// projection argument e.g. to implement a swap.
            bool consume_boundary;
        };

        /// Whenever something is being evaluated it will register itself here.
        /// If evaluation then circularly depends on itself it can fail with a diagnostic.
        /// Evaluations that fail should never remove themselves from the registry because we must know
        /// to abort other evaluations that depend on it as well.
        /// This is because individual evaluations will catch diagnostics and we can continue evaluating something else.
        /// In the end, if any diagnostics of error severity were present, the sir is erroneous and can't be lowered.
        std::unordered_set<std::string> active_evaluations;

        /// A unique evaluation context.
        struct EvaluationContext final {

        };

        template <std::convertible_to<std::string_view>... Arg> auto residualize_all_annotated_as(
            Arg... annotation_qualified_paths
        ) -> std::span<int> { // TODO: Return type or general API change.
            throw std::runtime_error("todo");

            template for (constexpr std::string_view path : annotation_qualified_paths) {

            }
        }

        void collect_decl(Decl& decl, bool top_level = false) {

        }

      private:
        /// This method must only be called once, so it is private and only called in the controlled
        /// environment of the `evaluate` free function.
        void evaluate() {

        }
    };

    /// Produces an evaluated Sir instance.
    inline auto evaluate(Modules modules, std::vector<SourceUnit> source_units) -> Sir {
        Sir sir(
            std::move(modules),
            std::move(source_units),
            AUTO_IMPORTS
                | std::views::transform([] (auto s) { return std::string(s); })
                | std::ranges::to<std::vector>()
        );

        sir.evaluate();

        return sir;
    }
}

namespace str {
    /// Unlike the actual compiler, the bootstrap compiler naively executes the residual tree directly
    /// instead of lowering, since it would be a waste to write non self-hosted backends.
    inline i32 execute(Sir& sir) {
        // auto residuals = sir.residualize_all_annotated_as("core.Entry");

        // for (auto residual : residuals) {

        // }

        return 0;
    }

    /// A special temporary backend for the 6502 for prototyping the language in an extremely constrained environment.
    inline i32 backend_lower_ca65(Sir& sir) {
        // auto residuals = sir.residualize_all_annotated_as("core.Entry", "core.Export");

        // for (auto residual : residuals) {

        // }

        return 0;
    }
}

namespace str::run {
    inline auto collect_all_source_units() -> std::vector<SourceUnit> {
        std::vector<SourceUnit> source_units;

        for (auto path : MODULE_PATHS) {
            std::error_code error_code;
            if (not std::filesystem::exists(path, error_code)) continue;

            auto options = std::filesystem::directory_options::skip_permission_denied;
            for (auto const& entry : std::filesystem::recursive_directory_iterator(path, options, error_code)) {
                if (entry.is_regular_file() and entry.path().extension() == ".str") {
                    std::FILE* file = std::fopen(entry.path().c_str(), "rb");

                    if (file) {
                        ScopeExit scope_exit = [file] { std::fclose(file); };

                        std::fseek(file, 0, SEEK_END);
                        u32 size = std::ftell(file);
                        std::rewind(file);

                        std::string content;
                        content.resize(size);
                        std::fread(content.data(), 1, size, file);

                        source_units.push_back({
                            .text = std::move(content),
                            .source = entry.path().string()
                        });
                    }
                }
            }
        }

        return source_units;
    }

    inline void print_compile_diagnostic(std::ostream& os, Diagnostic const& diagnostic, std::span<const SourceUnit> source_units) {
        static constexpr auto reset = "\033[0m";
        static constexpr auto dim   = "\033[90m";

        std::string_view color;
        std::string_view level;

        switch (diagnostic.severity) {
            case Diagnostic::Severity::Error:   color = "\033[31m"; level = "error";   break; // Red
            case Diagnostic::Severity::Warning: color = "\033[33m"; level = "warning"; break; // Yellow
            case Diagnostic::Severity::Info:    color = "\033[34m"; level = "info";    break; // Blue
            case Diagnostic::Severity::Runtime: color = "\033[35m"; level = "runtime"; break; // Purple
        }

        std::println(os, "{}{}:{}{} {}", color, level, reset, "\033[97m", diagnostic.what());

        std::visit(overloaded {
            [&] (Provenance::Span const& span) {
                auto f = span.start;
                auto l = span.end;

                std::println(os, "{}{}:{}:{}-{}:{}{}", dim, f.source, f.line, f.column - f.count + 1, l.line, l.column, reset);

                u32 max_lines = 3;
                u32 line_count = l.line - f.line + 1;

                std::string_view src_text;
                for (auto const& source_unit : source_units) {
                    if (source_unit.source == f.source) {
                        src_text = source_unit.text;
                        break;
                    }
                }

                if (src_text.empty()) {
                    std::println(os, "");
                    return;
                }

                std::vector<std::string_view> lines;
                usize line_n = 1;
                usize pos = 0;

                while (pos < src_text.size()) {
                    usize next_nl = src_text.find('\n', pos);
                    std::string_view line_str =
                        next_nl == std::string_view::npos
                            ? src_text.substr(pos)
                            : src_text.substr(pos, next_nl - pos);

                    if (line_n >= f.line) {
                        lines.push_back(line_str);
                    }

                    if (line_n >= l.line or lines.size() >= max_lines) {
                        break;
                    }

                    pos = next_nl == std::string_view::npos ? src_text.size() : next_nl + 1;
                    line_n += 1;
                }

                for (usize i = 0; i < lines.size(); i += 1) {
                    u32 curr_line = f.line + i;
                    std::string ln_str = std::to_string(curr_line);
                    std::string pad(ln_str.size() < 4 ? 4 - ln_str.size() : 0, ' ');

                    std::println(os, "{}{} | {}{}", dim, pad + ln_str, reset, lines[i]);

                    std::print(os, "{}     | {}", dim, color);

                    if (f.line == l.line) {
                        u32 start_col = (f.column >= f.count) ? f.column - f.count : 0;
                        u32 width = (l.column > start_col) ? l.column - start_col : 1;
                        std::println(os, "{}{}", std::string(start_col, ' '), std::string(std::max(1u, width), '^'));
                    } else if (curr_line == f.line) {
                        u32 start_col = (f.column >= f.count) ? f.column - f.count : 0;
                        u32 width = lines[i].size() > start_col ? lines[i].size() - start_col : 1;
                        std::println(os, "{}{}", std::string(start_col, ' '), std::string(std::max(1u, width), '^'));
                    } else if (curr_line == l.line) {
                        std::println(os, "{}", std::string(l.column, '^'));
                    } else {
                        std::println(os, "{}", std::string(lines[i].size(), '^'));
                    }
                    std::print(os, "{}", reset);
                }

                if (line_count > max_lines) std::println(os, "{} ... | {}", dim, reset);

                std::println(os, "");
            },
            [&] (Provenance::Source const& src) {
                std::println(os, "{}{}{}\n", dim, src.source, reset);
            }
        }, diagnostic.provenance.data);
    }

    inline auto parse_modules(
        std::span<const SourceUnit> source_units
    ) -> std::expected<Modules, std::vector<Diagnostic>> {
        Modules modules;
        std::vector<Diagnostic> diagnostics;

        for (auto const& source_unit : source_units) {
            try {
                auto tokens = tokenize(source_unit.text, source_unit.source);
                auto ast = parse(tokens);
                modules[ast.module].emplace_back(std::move(ast));
            } catch (Diagnostic& diagnostic) {
                diagnostics.emplace_back(std::move(diagnostic));
            }
        }

        if (diagnostics.empty()) {
            return std::move(modules);
        } else {
            return std::unexpected(std::move(diagnostics));
        }
    }

    /// The entry point for the run subcommand.
    inline i32 main() {
        auto source_units = collect_all_source_units();

        auto modules = parse_modules(source_units);
        if (not modules) {
            for (auto const& diagnostic : modules.error()) {
                print_compile_diagnostic(std::cerr, diagnostic, source_units);
            }

            return -1;
        }

        auto sir = evaluate(std::move(modules.value()), std::move(source_units));

        for (auto const& diagnostic : sir.get_diagnostics()) {
            print_compile_diagnostic(std::cerr, diagnostic, sir.get_source_units());
        }

        if (not sir.erroneous()) {
            return execute(sir);
        } else {
            return -1;
        }
    }
}

namespace str::test {
    /// Exception used to communicate test failure.
    struct Unexpected : std::exception {
        std::string results;

        explicit Unexpected(std::string results) : results(std::move(results)) {}

        auto what() const noexcept -> char const* override {
            return results.c_str();
        }
    };

    /// Dynamic compiler test interface.
    struct Test {
        std::string name;

        explicit Test(std::string name) : name(std::move(name)) {}

        virtual ~Test() = default;

        /// Runs the test.
        virtual void run() = 0;

        /// If the test throws diagnostics it should answer the source text
        /// in order for the test runner to be able to print the diagnostic properly.
        virtual auto source() -> std::string = 0;
    };

    /// A test used to verify correctness of the expression parser.
    struct ExprTest final : Test {
        /// The expression to parse.
        std::string expr;
        /// The expected AST output.
        std::string expect;

        constexpr ExprTest(std::string name, std::string expr, std::string expect)
            : Test(std::move(name)), expr(std::move(expr)), expect(std::move(expect)) {}

        void run() override {
            auto tokens = tokenize(expr, name);
            auto ast = Parser().parse_expr(tokens);
            auto fmt = std::format("{}", ast);

            if (fmt != expect) throw Unexpected(std::format(
                "exp: {}\n"
                "got: {}\n",
                expect, fmt
            ));
        }

        auto source() -> std::string override {
            return expr;
        }
    };

    inline auto tests = std::to_array<std::unique_ptr<Test>>({
        std::make_unique<ExprTest>(
            "identifier",
            "value",
            "value"
        ),
        std::make_unique<ExprTest>(
            "intrinsic",
            "#Foo()",
            "#Foo()"
        )
    });

    /// The entry point for the test subcommand.
    inline i32 main() {
        usize failures = 0;
        usize i = 0;

        for (auto& test : tests) {
            try {
                test->run();
                std::println(std::cout, R"(Test {}, "{}" succeeded\n)", i, test->name);
            } catch (Unexpected& unexpected) {
                std::println(std::cout, R"(Test {}, "{}" failed\n)", i, test->name);
                std::println(std::cout, "{}\n", unexpected.results);
                failures += 1;
            } catch (Diagnostic& diagnostic) {
                std::array<SourceUnit, 1> pseudo_units = {
                    SourceUnit {
                        .text = test->source(),
                        .source = test->name
                    }
                };

                std::println(std::cout, R"(Test {}, "{}" failed\n)", i, test->name);
                run::print_compile_diagnostic(std::cout, diagnostic, pseudo_units);
                failures += 1;
            } catch (std::exception& exception) {
                std::println(std::cout, R"(Test {}, "{}" failed\n)", i, test->name);
                std::println(std::cout, "Unknown exception: {}\n", exception.what());
                failures += 1;
            }

            i += 1;
        }

        if (failures) {
            std::println(std::cout, "Failed {} of {} tests", failures, i);
        } {
            std::println(std::cout, "Succeeded all {} tests", i);
        }

        return 0;
    }
}
