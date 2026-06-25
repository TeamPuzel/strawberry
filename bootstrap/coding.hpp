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
#include <optional>
#include <charconv>
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

    template <typename T> struct OptionalTraits final {
        static constexpr bool value = false;
    };

    template <typename T> struct OptionalTraits<std::optional<T>> final {
        static constexpr bool value = true;
        using Inner = T;
    };

    template <typename T> constexpr bool is_optional_v = OptionalTraits<T>::value;
    template <typename T> using optional_value_t = typename OptionalTraits<T>::Inner;
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

            if constexpr (detail::is_optional_v<Decay>) {
                return value ? encode(*value) : "null";
            } else if constexpr (std::same_as<Decay, bool>) {
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
                static_assert(sizeof(T) == 0, "unsupported json type");
            }
        }

        template <typename T> constexpr auto decode(std::string_view format) -> T {
            return decode_recurse<T>(format);
        }

        class Error final : std::exception {
            std::string reason;

          public:
            constexpr explicit Error(std::string reason) : reason(std::move(reason)) {}

            auto what() const noexcept -> char const* override {
                return reason.c_str();
            }
        };

      private:
        constexpr void skip_whitespace(std::string_view& text) {
            while (
                not text.empty() and
                (text[0] == ' ' or text[0] == '\n' or text[0] == '\r' or text[0] == '\t')
            ) {
                text.remove_prefix(1);
            }
        }

        constexpr auto empty(std::string_view text) -> bool {
            if (text.empty()) {
                throw Error("unexpected end");
            } else {
                return false;
            }
        }

        template <typename T> constexpr auto decode_recurse(std::string_view& text) -> T {
            using Decay = std::decay_t<T>;

            skip_whitespace(text);

            if constexpr (detail::is_optional_v<Decay>) {
                if (text.starts_with("null")) {
                    text.remove_prefix(4);
                    return std::nullopt;
                } else {
                    return decode_recurse<detail::optional_value_t<Decay>>(text);
                }
            } else if constexpr (std::same_as<Decay, bool>) {
                if (text.starts_with("true")) {
                    text.remove_prefix(4);
                    return true;
                } else if (text.starts_with("false")) {
                    text.remove_prefix(5);
                    return false;
                } else {
                    throw Error("expected boolean");
                }
            } else if constexpr (std::integral<Decay> or std::floating_point<Decay>) {
                usize i = 0;

                while (
                    i < text.size() and (
                        std::isdigit(text[i]) or
                        text[i] == '.' or
                        text[i] == '-' or
                        text[i] == '+' or
                        text[i] == 'e' or
                        text[i] == 'E'
                    )
                ) {
                    i += 1;
                }

                Decay value {};
                auto [_, ec] = std::from_chars(text.data(), text.data() + i, value);
                text.remove_prefix(i);

                if (ec != std::errc()) throw Error("expected number");

                return value;
            } else if constexpr (std::is_convertible_v<Decay, std::string_view>) {
                if (not empty(text) and text.front() == '"') text.remove_prefix(1);

                usize i = 0;

                while (i < text.size() and text[i] != '"') {
                    if (text[i] == '\\') i += 1;
                    i += 1;
                }

                std::string_view str = text.substr(0, i);
                if (i < text.size()) text.remove_prefix(i + 1);

                return T(str);
            } else if constexpr (std::ranges::range<Decay>) {
                Decay range;

                if (text.empty() or text.front() != '[') throw Error("expected array");

                text.remove_prefix(1);

                while (not empty(text)) {
                    skip_whitespace(text);

                    if (text.front() == ']') {
                        text.remove_prefix(1);
                        break;
                    }

                    range.push_back(decode_recurse<std::ranges::range_value_t<Decay>>(text));

                    skip_whitespace(text);

                    if (text.front() == ',') text.remove_prefix(1);
                }

                return range;
            } else if constexpr (std::is_class_v<Decay>) {
                Decay object;

                if (text.empty() or text.front() != '{') throw Error("expected object");

                text.remove_prefix(1);

                while (not text.empty()) {
                    skip_whitespace(text);

                    if (text.front() == '}') {
                        text.remove_prefix(1);
                        break;
                    }

                    auto key = decode_recurse<std::string>(text);

                    skip_whitespace(text);
                    if (text.front() == ':') text.remove_prefix(1); else throw Error("expected key separator");
                    skip_whitespace(text);

                    bool found = false;
                    constexpr auto members = detail::reflected_members<Decay>();

                    template for (constexpr auto member : members) {
                        constexpr auto ignore =
                            std::meta::annotation_of_type<annotations::detail::ignore_t>(member);
                        if constexpr (not ignore) {
                            std::string_view expected_key = identifier_of(member);
                            constexpr auto rename_annotation =
                                std::meta::annotation_of_type<annotations::detail::rename_t>(member);
                            if constexpr (rename_annotation) {
                                expected_key = rename_annotation->name;
                            }

                            if (key == expected_key) {
                                object.[:member:] =
                                    decode_recurse<std::remove_cvref_t<decltype(object.[:member:])>>(text);
                                found = true;
                            }
                        }
                    }

                    if (not found) {
                        usize bracket_depth = 0;
                        bool in_string = false;
                        bool escape = false;

                        while (not text.empty()) {
                            char c = text.front();
                            if (in_string) {
                                if (escape) {
                                    escape = false;
                                } else if (c == '\\') {
                                    escape = true;
                                } else if (c == '"') {
                                    in_string = false;
                                }
                            } else {
                                if (c == '"') {
                                    in_string = true;
                                } else if (c == '{' or c == '[') {
                                    bracket_depth += 1;
                                } else if (c == '}' or c == ']') {
                                    if (bracket_depth == 0) break;
                                    bracket_depth -= 1;
                                } else if (bracket_depth == 0 and (c == ',' or c == '}')) {
                                    break;
                                }
                            }
                            text.remove_prefix(1);
                        }
                    }

                    skip_whitespace(text);

                    if (text.front() == ',') text.remove_prefix(1);
                }

                return object;
            } else {
                static_assert(sizeof(T) == 0, "unsupported json type");
            }
        }
    };

    template <> struct SupportsEncodingFormat<Json, std::string> : std::true_type {};
    template <> struct SupportsDecodingFormat<Json, std::string> : std::true_type {};
    template <> struct SupportsDecodingFormat<Json, std::string_view> : std::true_type {};

    static_assert(Coder<Json, std::string>);
    static_assert(Decoder<Json, std::string_view>);
}
