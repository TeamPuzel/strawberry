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

#include <meta>
#include <type_traits>
#include <concepts>
#include <string_view>
#include <string>
#include <ranges>
#include "strc.hpp"
#include "primitive.hpp"

namespace str::coding {
    // Query if the type supports a given encoding format.
    // By default this is obtained from a dependent type but this trait can be
    // specialized manually to provide multiple formats.
    template <typename Encoder, typename Format> struct SupportsEncodingFormat final {
        static constexpr bool value = std::same_as<Format, typename Encoder::EncodingFormat>;
    };

    // Query if the type supports a given decoding format.
    // By default this is obtained from a dependent type but this trait can be
    // specialized manually to provide multiple formats.
    template <typename Decoder, typename Format> struct SupportsDecodingFormat final {
        static constexpr bool value = std::same_as<Format, typename Decoder::DecodingFormat>;
    };
}

namespace str::coding::detail {
    struct Dummy final {};

    // Workaround for constant vector iteration being broken in clang at the moment.
    template <typename T> consteval auto reflected_members() {
        constexpr auto size = std::meta::nonstatic_data_members_of(^^T).size();
        std::array<std::meta::info, size> arr;
        auto vec = std::meta::nonstatic_data_members_of(^^T);
        for (usize i = 0; i < size; i += 1) arr[i] = vec[i];
        return arr;
    }
}

namespace str::coding {
    template <typename Self, typename Format> concept Encoder = SupportsEncodingFormat<Self, Format>::value and
    requires (Self encoder, detail::Dummy value, detail::Dummy const& ref) {
        { encoder.encode(std::move(value)) } -> std::same_as<Format>;
        { encoder.encode(ref) } -> std::same_as<Format>;
    };

    template <typename Self, typename Format> concept Decoder = SupportsDecodingFormat<Self, Format>::value and
    requires (Self decoder) {
        { decoder.template decode<detail::Dummy>(std::declval<Format>()) } -> std::same_as<detail::Dummy>;
    };

    template <typename Self, typename Format> concept Coder = Encoder<Self, Format> and Decoder<Self, Format>;
}

namespace str::coding::annotations {
    namespace detail {
        struct ignore_t final {
            constexpr bool operator <=> (ignore_t const&) const = default;
        };

        struct rename_t final {
            char const* name;
            constexpr bool operator <=> (rename_t const&) const = default;
        };
    }

    constexpr detail::ignore_t ignore;

    consteval detail::rename_t rename(std::string_view name) {
        return { .name = std::define_static_string(name) };
    }
}

namespace str::coding {
    using annotations::ignore;
    using annotations::rename;
}

namespace str::coding {
    struct Json final {
        template <typename T> constexpr auto encode(T&& value) -> std::string {
            using Decay = std::decay_t<T>;

            if constexpr (std::same_as<Decay, bool>) {
                return value ? "true" : "false";
            } else if constexpr (std::integral<Decay> or std::floating_point<Decay>) {
                return std::to_string(value);
            } else if constexpr (std::is_convertible_v<Decay, std::string_view>) {
                return "\"" + std::string(value) + "\"";
            } else if constexpr (std::ranges::range<Decay>) {
                std::string out;
                out += "[";

                bool first = true;
                for (auto&& element : value) {
                    if (not first) out += ", "; first = false;
                    out += encode(element);
                }

                out += "]";
                return out;
            } else if constexpr (std::is_class_v<Decay>) {
                std::string out;
                out += "{";

                bool first = true;
                constexpr auto members = detail::reflected_members<Decay>();
                template for (constexpr auto member : members) {
                    constexpr auto ignore =
                        std::meta::annotation_of_type<annotations::detail::ignore_t>(member);

                    if constexpr (not ignore) {
                        if (not first) out += ", "; first = false;

                        out += "\"";
                        constexpr auto rename_annotation =
                            std::meta::annotation_of_type<annotations::detail::rename_t>(member);
                        if constexpr (rename_annotation) {
                            out += rename_annotation->name;
                        } else {
                            out += identifier_of(member);
                        }
                        out += "\": ";

                        out += encode(value.[:member:]);
                    }
                }

                out += "}";
                return out;
            } else {
                static_assert(false, "unsupported json type");
            }
        }

        template <typename T> constexpr auto decode(std::string_view format) -> T {
            auto tokens = tokenize(format, "<json-rpc>", {
                .allow_tabs = true,
                .skip_newlines = true
            });
            return decode_recurse<T>(tokens);
        }

      private:
        constexpr void skip_json_value(TokenStream& tokens) {
            if (tokens.match<Token::BraceLeft>()) {
                if (tokens.match<Token::BraceRight>()) return;

                while (true) {
                    tokens.expect<Token::String>();
                    tokens.expect<Token::Colon>();
                    skip_json_value(tokens);
                    if (not tokens.match<Token::Comma>()) break;
                }

                tokens.expect<Token::BraceRight>();
            } else if (tokens.match<Token::BracketLeft>()) {
                if (tokens.match<Token::BracketRight>()) return;

                while (true) {
                    skip_json_value(tokens);
                    if (not tokens.match<Token::Comma>()) break;
                }

                tokens.expect<Token::BracketRight>();
            } else if (tokens.match<Token::Symbolic>("-")) {
                tokens.expect<Token::Number>();
            } else {
                tokens.next();
            }
        }

        template <typename T> constexpr auto decode_recurse(TokenStream& tokens) -> T {
            using Decay = std::decay_t<T>;

            if constexpr (std::same_as<Decay, bool>) {
                if (tokens.match<Token::Identifier>("true")) return true;
                if (tokens.match<Token::Identifier>("false")) return false;

                throw Diagnostic::error(tokens.failure_provenance(), "expected boolean value");
            } else if constexpr (std::integral<Decay> or std::floating_point<Decay>) {
                bool negative = (bool) tokens.match<Token::Symbolic>("-");
                auto content = tokens.expect_as<Token::Number>().content;

                std::string num_str(content);

                if (negative) num_str = "-" + num_str;

                if constexpr (std::floating_point<Decay>) {
                    return static_cast<Decay>(std::stod(num_str));
                } else {
                    return static_cast<Decay>(std::stoll(num_str));
                }
            } else if constexpr (std::is_convertible_v<Decay, std::string_view>) {
                return Decay(tokens.expect_as<Token::String>().content);
            } else if constexpr (std::ranges::range<Decay>) {
                Decay result;

                tokens.expect<Token::BracketLeft>();

                if (tokens.match<Token::BracketRight>()) return result;

                while (true) {
                    result.push_back(decode_recurse<std::ranges::range_value_t<Decay>>(tokens));
                    if (not tokens.match<Token::Comma>()) break;
                }

                tokens.expect<Token::BracketRight>();

                return result;
            } else if constexpr (std::is_class_v<Decay>) {
                Decay result;

                tokens.expect<Token::BraceLeft>();

                if (tokens.match<Token::BraceRight>()) return result;

                while (true) {
                    auto key = tokens.expect_as<Token::String>().content;
                    tokens.expect<Token::Colon>();

                    bool found = false;
                    constexpr auto members = detail::reflected_members<Decay>();

                    template for (constexpr auto member : members) {
                        constexpr auto ignore =
                            std::meta::annotation_of_type<annotations::detail::ignore_t>(member);
                        if constexpr (not ignore) {
                            std::string_view member_name = identifier_of(member);
                            constexpr auto rename_annotation =
                                std::meta::annotation_of_type<annotations::detail::rename_t>(member);

                            if constexpr (rename_annotation) {
                                member_name = rename_annotation->name;
                            }

                            if (key == member_name) {
                                result.[:member:] =
                                    decode_recurse<std::remove_cvref_t<decltype(result.[:member:])>>(tokens);
                                found = true;
                            }
                        }
                    }

                    if (not found) {
                        skip_json_value(tokens);
                    }

                    if (not tokens.match<Token::Comma>()) break;
                }

                tokens.expect<Token::BraceRight>();

                return result;
            } else {
                static_assert(false, "unsupported json type");
            }
        }
    };

    template <> struct SupportsEncodingFormat<Json, std::string> : std::true_type {};
    template <> struct SupportsDecodingFormat<Json, std::string> : std::true_type {};
    template <> struct SupportsDecodingFormat<Json, std::string_view> : std::true_type {};

    static_assert(Coder<Json, std::string>);
    static_assert(Decoder<Json, std::string_view>);
}
