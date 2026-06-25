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
    template <typename... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template <typename... Ts> overloaded(Ts...) -> overloaded<Ts...>;

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


    class Token final {
      public:
        struct Error final {};
        struct NewLine final {};
        struct ParenLeft final {};
        struct ParenRight final {};
        struct BraceLeft final {};
        struct BraceRight final {};
        struct BracketLeft final {};
        struct BracketRight final {};
        struct Comma final {};
        struct Colon final {};
        struct SemiColon final {};
        struct At final {};
        struct Pound final {};
        struct Tick final {};
        struct Dot final {};
        struct Arrow final {};

        struct Comment final {
            std::string_view selector;
            std::string_view content;
        };

        struct Number final {
            std::string_view content;
        };

        struct String final {
            std::string_view content;
        };

        struct MultilineString final {
            std::string_view content;
        };

        struct Identifier final {
            std::string_view content;
        };

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

    class Provenance final {
      public:
        struct Span final {
            Token start;
            Token end;
        };

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
    };

    class Diagnostic final : std::exception {
      public:
        enum class Severity { Error, Warning, Info, Runtime } severity;
        Provenance provenance;
        std::string reason;
        std::optional<std::string> help;

        constexpr Diagnostic(Severity severity, Provenance provenance, std::string reason) noexcept
            : severity(severity), provenance(provenance), reason(std::move(reason)) {}

        constexpr Diagnostic(Severity severity, Provenance provenance, std::string reason, std::string help) noexcept
            : severity(severity), provenance(provenance), reason(std::move(reason)), help(std::move(help)) {}

        auto what() const noexcept -> char const* override {
            return reason.c_str();
        }

        static constexpr Diagnostic error(Provenance provenance, std::string reason) noexcept {
            return Diagnostic(Severity::Error, provenance, std::move(reason));
        }

        static constexpr Diagnostic warning(Provenance provenance, std::string reason) noexcept {
            return Diagnostic(Severity::Warning, provenance, std::move(reason));
        }

        static constexpr Diagnostic info(Provenance provenance, std::string reason) noexcept {
            return Diagnostic(Severity::Info, provenance, std::move(reason));
        }

        static constexpr Diagnostic runtime(Provenance provenance, std::string reason) noexcept {
            return Diagnostic(Severity::Runtime, provenance, std::move(reason));
        }

        static constexpr Diagnostic error(Provenance provenance, std::string reason, std::string help) noexcept {
            return Diagnostic(Severity::Error, provenance, std::move(reason), std::move(help));
        }

        static constexpr Diagnostic warning(Provenance provenance, std::string reason, std::string help) noexcept {
            return Diagnostic(Severity::Warning, provenance, std::move(reason), std::move(help));
        }

        static constexpr Diagnostic info(Provenance provenance, std::string reason, std::string help) noexcept {
            return Diagnostic(Severity::Info, provenance, std::move(reason), std::move(help));
        }

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
            [&ctx] (Token::Error _)        { return std::format_to(ctx.out(), "Error"); },
            [&ctx] (Token::NewLine _)      { return std::format_to(ctx.out(), "NewLine"); },
            [&ctx] (Token::ParenLeft _)    { return std::format_to(ctx.out(), "ParenLeft"); },
            [&ctx] (Token::ParenRight _)   { return std::format_to(ctx.out(), "ParenRight"); },
            [&ctx] (Token::BraceLeft _)    { return std::format_to(ctx.out(), "BraceLeft"); },
            [&ctx] (Token::BraceRight _)   { return std::format_to(ctx.out(), "BraceRight"); },
            [&ctx] (Token::BracketLeft _)  { return std::format_to(ctx.out(), "BracketLeft"); },
            [&ctx] (Token::BracketRight _) { return std::format_to(ctx.out(), "BracketRight"); },
            [&ctx] (Token::Comma _)        { return std::format_to(ctx.out(), "Comma"); },
            [&ctx] (Token::Colon _)        { return std::format_to(ctx.out(), "Colon"); },
            [&ctx] (Token::SemiColon _)    { return std::format_to(ctx.out(), "SemiColon"); },
            [&ctx] (Token::At _)           { return std::format_to(ctx.out(), "At"); },
            [&ctx] (Token::Pound _)        { return std::format_to(ctx.out(), "Pound"); },
            [&ctx] (Token::Tick _)         { return std::format_to(ctx.out(), "Tick"); },
            [&ctx] (Token::Dot _)          { return std::format_to(ctx.out(), "Dot"); },
            [&ctx] (Token::Arrow _)        { return std::format_to(ctx.out(), "Arrow"); },

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
        auto get_text() const -> std::string_view {
            return text;
        }

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

            auto take() -> Provenance {
                auto result = tokens->unchecked_span_pop();
                tokens = nullptr;
                return result;
            }
        };

        [[nodiscard]] auto span() -> ProvenanceScope {
            unchecked_span_push();
            return ProvenanceScope(this);
        }

      private:
        void consume(u32 count = 1) {
            index += count;
            column += count;
            consumed_count += count;
        }

        void rewind(u32 count = 1) {
            index -= count;
            column -= count;
            consumed_count -= count;
        }

        auto at() const -> char {
            return text.at(index);
        }

        auto at(usize index) const -> std::optional<char> {
            if (index < text.size()) {
                return text[index];
            } else {
                return std::nullopt;
            }
        }

        auto take(usize count) -> std::string_view {
            auto result = text.substr(index, count);
            consume(count);
            return result;
        }

        auto take_until(char sentinel) -> std::string_view {
            usize count = 0;
            for (usize i = index; i < text.size() and text[i] != sentinel; i += 1) count += 1;
            return take(count);
        }

        auto take_until(auto&& predicate) -> std::string_view {
            usize count = 0;
            for (usize i = index; i < text.size() and not std::invoke(predicate, text[i]); i += 1) count += 1;
            return take(count);
        }

        auto is(char pattern) const -> bool {
            return text.at(index) == pattern;
        }

        auto are(std::string_view pattern) const -> bool {
            for (u32 i = 0; i < pattern.size(); i += 1)
                if (i + index >= text.size() or text.at(i + index) != pattern[i])
                    return false;
            return true;
        }

        auto in_range(char from, char to) const -> bool {
            auto value = text.at(index);
            return value >= from and value <= to;
        }

        auto valid_ident() const -> bool {
            return
                not in_range(0, 31) and not is(127) and
                not is(' ') and not is('"') and not is('#') and not is('\'') and
                not is('(') and not is(')') and not is(',') and not is(':') and
                not is(';') and not is('@') and not is('[') and not is('\\') and
                not is(']') and not is('{') and not is('}') and not is('`');
        }

        auto valid_digit() const -> bool {
            return in_range('0', '9') or in_range('A', 'Z') or in_range('a', 'z');
        }

        auto valid_symbolic_ident() const -> bool {
            return
                valid_ident() and
                not in_range('0', '9') and not in_range('A', 'Z') and not in_range('a', 'z') and
                not is('_');
        }

        auto valid_pure_ident() const -> bool {
            return in_range('0', '9') or in_range('A', 'Z') or in_range('a', 'z') or is('_');
        }

        void no_yield() {
            consumed_count = 0;
        }

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

        /// Generic provenance for the file itself, with unspecified location.
        auto generic_provenance() const -> Provenance {
            return Provenance(source);
        }

        auto todo() -> Diagnostic {
            return Diagnostic::error(fallthrough_provenance(), "todo");
        }

        auto finished() const -> bool {
            return index >= text.size();
        }

        auto next() -> std::optional<Token> { start:
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

                do {
                    count += 1;
                    consume();
                } while (not finished() and valid_pure_ident());

                rewind(count);
                auto content = take(count);

                return yield<Token::Identifier>(content);
            } else {
                throw Diagnostic::error(yield<Token::Error>(), std::format("unexpected character: {}", at()));
            }
        }

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

        void drop(u32 count = 1) {
            for (u32 i = 0; i < count; i += 1) last_token = next();
        }

        void drop_while(auto&& predicate) {
            std::optional token = peek();
            while (token and std::invoke(predicate, std::as_const(*token))) {
                last_token = next();
                token = peek();
            }
        }

        template <typename T> auto match() -> std::optional<Token> {
            std::optional token = peek();

            if (token and std::holds_alternative<T>(token->data)) {
                last_token = next();
                return last_token;
            } else {
                return std::nullopt;
            }
        }

        template <typename T> auto match(std::string_view content) -> std::optional<Token> {
            std::optional token = peek();

            if (token and std::holds_alternative<T>(token->data) and token->identifier_content() == content) {
                last_token = next();
                return last_token;
            } else {
                return std::nullopt;
            }
        }

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

        template <typename T> auto match_as() -> std::optional<T> {
            return match<T>().transform(&Token::get<T>);
        }

        template <typename T> auto match_as(std::string_view content) -> std::optional<T> {
            return match<T>(content).transform(&Token::get<T>);
        }

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

        template <typename T> auto allow(std::string_view content) -> std::optional<Token> {
            std::optional token = peek();

            if (token and std::holds_alternative<T>(token->data) and token->identifier_content() == content) {
                return next();
            } else {
                return std::nullopt;
            }
        }

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

        template <typename T> auto allow_as(std::string_view content) -> std::optional<T> {
            return allow<T>(content).transform(&Token::get<T>);
        }

        template <typename T> auto allow_as() -> std::optional<T> {
            return allow<T>().transform(&Token::get<T>);
        }

        template <typename T, typename F> auto allow_as(F&& predicate) -> std::optional<T> {
            return allow<T>(std::forward<F>(predicate)).transform(&Token::get<T>);
        }

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

        template <typename T> auto expect_as() -> T {
            return expect<T>().template get<T>();
        }

        template <typename T> auto expect_as(std::string_view content) -> T {
            return expect<T>(content).template get<T>();
        }

        template <typename T, typename F> auto expect_as(F&& predicate) -> T {
            return expect<T>(std::forward<F>(predicate)).template get<T>();
        }

        template <typename T> auto peek_match() -> std::optional<Token> {
            std::optional token = peek();

            if (token and std::holds_alternative<T>(token->data)) {
                return token;
            } else {
                return std::nullopt;
            }
        }

        template <typename T> auto peek_match(std::string_view content) -> std::optional<Token> {
            std::optional token = peek();

            if (token and std::holds_alternative<T>(token->data) and token->identifier_content() == content) {
                return token;
            } else {
                return std::nullopt;
            }
        }

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

        template <typename T> auto peek_match_as() -> std::optional<T> {
            return peek_match<T>().transform(&Token::get<T>);
        }

        template <typename T> auto peek_match_as(std::string_view content) -> std::optional<T> {
            return peek_match<T>(content).transform(&Token::get<T>);
        }

        template <typename T, typename F> auto peek_match_as(F&& predicate) -> std::optional<T> {
            return peek_match<T>(std::forward<F>(predicate)).transform(&Token::get<T>);
        }

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

        template <typename T> auto peek_expect_as() -> T {
            return peek_expect<T>().template get<T>();
        }

        template <typename T> auto peek_expect_as(std::string_view content) -> T {
            return peek_expect<T>(content).template get<T>();
        }

        template <typename T, typename F> auto peek_expect_as(F&& predicate) -> T {
            return peek_expect<T>(std::forward<F>(predicate)).template get<T>();
        }
    };

    constexpr auto tokenize(std::string_view text, std::string_view source) -> TokenStream {
        return TokenStream(text, source);
    }
}

namespace str {
    constexpr auto precedence(std::string_view pattern) -> usize {
        if (pattern == "=")   return 0;

        if (pattern == "or")  return 1;
        if (pattern == "and") return 2;

        if (pattern == "==")  return 3;
        if (pattern == "!=")  return 3;

        if (pattern == "<")   return 4;
        if (pattern == "<=")  return 4;
        if (pattern == ">")   return 4;
        if (pattern == ">=")  return 4;

        if (pattern == "+")   return 5;
        if (pattern == "-")   return 5;

        if (pattern == "*")   return 6;
        if (pattern == "/")   return 6;
        if (pattern == "%")   return 6;

        return 7;
    }

    enum class OperatorRole {
        Infix,
        Prefix,
        Postfix
    };

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
            [] (Token::Identifier identifier) -> OperatorRole { return OperatorRole::Infix; },
            // Arbitrary tokens can't be used as operators, this should never happen.
            [] (auto other) -> OperatorRole {
                throw std::logic_error("attempt to query operator role of non identifier token");
            }
        }, token.data);
    }

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

    class Expr final {
      public:
        using ExprBox = std::unique_ptr<Expr>;

        static ExprBox box(Expr& expr) {
            return std::make_unique<Expr>(std::move(expr));
        }

        static ExprBox box(Expr&& expr) {
            return std::make_unique<Expr>(std::move(expr));
        }

        struct Infix final {
            std::string_view name;
            ExprBox lhs;
            ExprBox rhs;

            Infix(
                std::string_view name,
                Expr lhs,
                Expr rhs
            ) : name(name)
              , lhs(box(lhs))
              , rhs(box(rhs))
            {}

            Infix(
                std::string_view name,
                ExprBox lhs,
                ExprBox rhs
            ) : name(name)
              , lhs(std::move(lhs))
              , rhs(std::move(rhs))
            {}
        };

        struct Prefix final {
            std::string_view name;
            ExprBox rhs;

            Prefix(
                std::string_view name,
                Expr rhs
            ) : name(name)
              , rhs(box(rhs))
            {}

            Prefix(
                std::string_view name,
                ExprBox rhs
            ) : name(name)
              , rhs(std::move(rhs))
            {}
        };

        struct Postfix final {
            std::string_view name;
            ExprBox lhs;

            Postfix(
                std::string_view name,
                Expr lhs
            ) : name(name)
              , lhs(box(lhs))
            {}

            Postfix(
                std::string_view name,
                ExprBox lhs
            ) : name(name)
              , lhs(std::move(lhs))
            {}
        };

        struct Projection final {
            enum class Kind { Mutating, Borrowing } kind;
            ExprBox expr;

            Projection(Kind kind, Expr expr) : kind(kind), expr(box(expr)) {}
            Projection(Kind kind, ExprBox expr) : kind(kind), expr(std::move(expr)) {}
        };

        struct Wildcard final {};

        struct Number final {
            std::string_view literal;
            explicit Number(std::string_view literal) : literal(literal) {}
        };

        struct String final {
            std::string_view literal;
            explicit String(std::string_view literal) : literal(literal) {}
        };

        struct Boolean final {
            bool value;
            explicit Boolean(bool value) : value(value) {}
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

        struct Call final {
            struct Argument final {
                std::optional<std::string_view> label;
                ExprBox expr;
            };

            std::string_view name;
            std::vector<Argument> arguments;

            Call(std::string_view name, std::vector<Argument> arguments)
                : name(name), arguments(std::move(arguments)) {}
        };

        struct Subscript final {
            struct Argument final {
                std::optional<std::string_view> label;
                ExprBox expr;
            };

            std::string_view name;
            std::vector<Argument> arguments;

            Subscript(std::string_view name, std::vector<Argument> arguments)
                : name(name), arguments(std::move(arguments)) {}
        };

        struct Block final {
            std::vector<ExprBox> expressions;
            explicit Block(std::vector<ExprBox> expressions) : expressions(std::move(expressions)) {}
        };

        struct Generics final {
            struct Parameter final {
                std::optional<std::string_view> label;
                ExprBox expr;
            };

            std::string_view name;
            std::vector<Parameter> parameters;

            Generics(std::string_view name, std::vector<Parameter> parameters)
                : name(name), parameters(std::move(parameters)) {}
        };

        struct List final {
            std::vector<ExprBox> expressions;
            explicit List(std::vector<ExprBox> expressions) : expressions(std::move(expressions)) {}
        };

        struct TrailingPack final {
            std::vector<ExprBox> expressions;
            explicit TrailingPack(std::vector<ExprBox> expressions) : expressions(std::move(expressions)) {}
        };

        struct Identifier final {
            std::string_view name;
            explicit Identifier(std::string_view name) : name(name) {}
        };

        struct Unsafe final {
            ExprBox expr;

            explicit Unsafe(Expr expr) : expr(box(expr)) {}
            explicit Unsafe(ExprBox expr) : expr(std::move(expr)) {}
        };

        struct Return final {
            ExprBox expr;

            explicit Return(Expr expr) : expr(box(expr)) {}
            explicit Return(ExprBox expr) : expr(std::move(expr)) {}
        };

        struct Throw final {
            ExprBox expr;

            explicit Throw(Expr expr) : expr(box(expr)) {}
            explicit Throw(ExprBox expr) : expr(std::move(expr)) {}
        };

        struct Await final {
            ExprBox expr;

            explicit Await(Expr expr) : expr(box(expr)) {}
            explicit Await(ExprBox expr) : expr(std::move(expr)) {}
        };

        struct Member final {
            std::string_view name;
            ExprBox expr;

            Member(std::string_view name, Expr expr) : name(name), expr(box(expr)) {}
            Member(std::string_view name, ExprBox expr) : name(name), expr(std::move(expr)) {}
        };

        struct Pattern final {

        };

        struct If final {

        };

        struct Guard final {

        };

        struct When final {

        };

        struct While final {

        };

        struct MemberInfer final {

        };

        struct Break final {

        };

        struct Loop final {

        };

        struct Match final {

        };

        struct Catch final {

        };

        struct Closure final {

        };

        struct Binding final {

        };

        struct PatternBinding final {

        };

        using Data = std::variant<
            Infix,
            Prefix,
            Postfix,
            Projection,
            Wildcard,
            Number,
            String,
            Boolean,
            Intrinsic,
            Tuple,
            Call,
            Subscript,
            Block,
            Generics,
            List,
            TrailingPack,
            Identifier,
            Unsafe,
            Return,
            Throw,
            Await,
            Member,
            Pattern,
            If,
            Guard,
            When,
            While,
            MemberInfer,
            Break,
            Loop,
            Match,
            Catch,
            Closure,
            Binding,
            PatternBinding
        >;

        Data data;
        Provenance provenance;

        Expr(Provenance provenance, Data data) : data(std::move(data)), provenance(provenance) {}
    };

    class Decl final {
      public:
        struct Fun final {

        };

        struct Init final {

        };

        struct Struct final {

        };

        struct Enum final {

        };

        struct Category final {

        };

        struct Extend final {

        };

        struct Type final {

        };

        struct Class final {

        };

        struct Member final {

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

        Data data;
        Provenance provenance;

        Decl(Provenance provenance, Data data) : data(std::move(data)), provenance(provenance) {}

        std::optional<std::string> documentation;
        std::vector<Expr> annotations;

        bool mod_pub = false;
        bool mod_unsafe = false;
        bool mod_open = false;
        bool mod_override = false;
        bool mod_inherent = false;
        bool mod_const = false;
        bool mod_static = false;
        bool mod_inline = false;
        bool mod_implicit = false;
        bool mod_base = false;
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

    constexpr auto format(str::Expr const& token, std::format_context& ctx) const {
        throw std::runtime_error("todo");
    }
};

template <> struct std::formatter<str::Decl, char> {
    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() and *it != '}') throw std::format_error("invalid format args for str::Decl");
        return it;
    }

    constexpr auto format(str::Decl const& token, std::format_context& ctx) const {
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

        auto parse_paren_expr(TokenStream& tokens) -> Expr {
            auto span = tokens.span();
            tokens.expect<Token::ParenLeft>();

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

        auto parse_prefix_expr(TokenStream& tokens) -> Expr {
            auto span = tokens.span();

            if (tokens.match<Token::Identifier>("mut")) {
                tokens.expect<Token::Symbolic>("&");

                Expr expr = parse_expr(tokens);

                return Expr(
                    span.take(),
                    Expr::Projection(Expr::Projection::Kind::Mutating, std::move(expr))
                );
            }

            if (tokens.match<Token::Symbolic>("&")) {
                Expr expr = parse_expr(tokens);

                return Expr(
                    span.take(),
                    Expr::Projection(Expr::Projection::Kind::Borrowing, std::move(expr))
                );
            }

            if (tokens.match<Token::Pound>()) {
                // auto name = tokens.expect_as<Token::Identifier>().content;

                // return Expr(
                //     span.take(),
                //     Expr::Intrinsic(name, std::move)
                // )
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
                } else {
                    // Destructuring tuple binding.
                }
            }

            if (tokens.match<Token::Identifier>("loop")) {
                // Manual loop.
            }

            if (tokens.match<Token::Identifier>("while")) {
                // Pattern loop.
            }

            if (tokens.match<Token::Identifier>("for")) {
                // Monadic loop.
            }

            if (tokens.match<Token::Identifier>("unsafe")) {

            }

            if (tokens.match<Token::Identifier>("return")) {

            }

            if (tokens.match<Token::Identifier>("break")) {

            }

            if (tokens.match<Token::Identifier>("throw")) {

            }

            if (tokens.match<Token::Identifier>("await")) {

            }

            if (tokens.match<Token::Dot>()) {
                // Type member inference.
            }

            if (tokens.match<Token::Identifier>("if")) {
                // Pattern conditional.
            }

            if (tokens.match<Token::Identifier>("guard")) {
                // Inverse pattern conditional.
            }

            if (tokens.match<Token::Identifier>("when")) {
                // Pattern boolean expression.
            }

            if (tokens.match<Token::BraceLeft>()) {
                // Block expression.
            }

            if (tokens.match<Token::Number>()) {

            }

            if (tokens.match<Token::String>()) {

            }

            if (tokens.match<Token::MultilineString>()) {

            }

            if (tokens.match<Token::ParenLeft>()) {
                // Parenthesized expression.
            }

            if (tokens.match<Token::BracketLeft>()) {
                // List literal.
            }

            if (tokens.peek_match<Token::Symbolic>("|") or tokens.peek_match<Token::Symbolic>("||")) {
                // Closure.
            }

            if (tokens.peek_match<Token::Identifier>()) {
                // Try match pure identifier prefix operator.
            }

            throw Diagnostic::error(tokens.fallthrough_provenance(), "expected expression");
        }

        auto parse_postfix_expr(Expr lhs, TokenStream& tokens) -> Expr {
            while (true) {

            }
        }

        auto parse_expr(
            TokenStream& tokens,
            usize minimum_precedence = 0,
            std::optional<std::string_view> excluded_op = std::nullopt
        ) -> Expr {
            Expr lhs = parse_postfix_expr(parse_prefix_expr(tokens), tokens);

            while (true) {
                auto op = tokens.peek();
                if (not op) break;
                if (not op->is<Token::Identifier>() or not op->is<Token::Symbolic>())
                    throw Diagnostic::error(*op, "invalid token for infix expression");
                if (operator_role(*op) != OperatorRole::Infix) break;
                if (op->identifier_content() == excluded_op) break;

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

            std::vector<Expr> annotations;

            while (tokens.match<Token::At>()) {
                annotations.emplace_back(parse_expr(tokens));
                tokens.expect<Token::NewLine>();
            }

            bool mod_pub      = (bool) tokens.match<Token::Identifier>("pub");
            bool mod_unsafe   = (bool) tokens.match<Token::Identifier>("unsafe");
            bool mod_open     = (bool) tokens.match<Token::Identifier>("open");
            bool mod_override = (bool) tokens.match<Token::Identifier>("override");
            bool mod_inherent = (bool) tokens.match<Token::Identifier>("inherent");
            bool mod_const    = (bool) tokens.match<Token::Identifier>("const");
            bool mod_static   = (bool) tokens.match<Token::Identifier>("static");
            bool mod_inline   = (bool) tokens.match<Token::Identifier>("inline");
            bool mod_implicit = (bool) tokens.match<Token::Identifier>("implicit");
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
            decl->mod_pub = mod_pub;
            decl->mod_unsafe = mod_unsafe;
            decl->mod_open = mod_open;
            decl->mod_override = mod_override;
            decl->mod_inherent = mod_inherent;
            decl->mod_const = mod_const;
            decl->mod_static = mod_static;
            decl->mod_inline = mod_inline;
            decl->mod_implicit = mod_implicit;
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

namespace str {
    struct SourceUnit final {
        std::string text;
        std::string source;
    };

    using Modules = std::unordered_map<std::string, std::vector<Ast>>;

    class Sir final {
        Modules modules;
        std::vector<SourceUnit> source_units;
        std::vector<std::string> auto_imports;
        std::vector<Diagnostic> diagnostics;
        bool erroneous = false;

        Sir(
            Modules modules,
            std::vector<SourceUnit> source_units,
            std::vector<std::string> auto_imports
        ) : modules(std::move(modules))
          , source_units(std::move(source_units))
          , auto_imports(std::move(auto_imports))
        {}

        friend auto evaluate(Modules modules, std::vector<SourceUnit> source_units) -> Sir;

        void evaluate() {

        }

      public:
        auto get_diagnostics() const -> std::span<const Diagnostic> {
            return diagnostics;
        }

        auto get_source_units() const -> std::span<const SourceUnit> {
            return source_units;
        }

        auto failed() const -> bool {
            return erroneous;
        }
    };

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
                print_compile_diagnostic(std::cout, diagnostic, source_units);
            }

            return -1;
        }

        auto sir = evaluate(std::move(modules.value()), std::move(source_units));

        for (auto const& diagnostic : sir.get_diagnostics()) {
            print_compile_diagnostic(std::cout, diagnostic, sir.get_source_units());
        }

        if (not sir.failed()) {
            return execute(sir);
        } else {
            return -1;
        }
    }
}

namespace str::test {
    /// The entry point for the test subcommand.
    inline i32 main() {
        std::println("TESTING: Ast");

        return 0;
    }
}
