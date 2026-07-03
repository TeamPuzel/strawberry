
const KEYWORDS = [
    "guard", "get", "set", "Nil", "nil", "object", "is", "annotation",
    "Any", "inherent", "unsafe", "base", "as", "open", "async", "decay",
    "await", "inline", "use", "import", "true", "false", "else", "module",
    "then", "throw", "throws", "catch", "try", "do", "and", "or", "not", "if",
    "let", "mut", "subscript", "extend", "Self", "Type", "self", "type",
    "implicit", "init", "deinit", "where", "fun", "pub", "enum", "category", "class",
    "struct", "operator", "infix", "prefix", "postfix", "match", "break", "loop",
    "while", "for", "in", "const", "static", "return", "when", "new", "final", "super",
    "rethrows", "override"
]

module.exports = grammar({
    name: 'strawberry',

    rules: {
        source_file: $ => repeat($._token),

        _token: $ => choice(
            $.comment,
            $.string,
            $.number,
            $.intrinsic,
            $.keyword,
            $.type_identifier,
            $.identifier,
            $.operator,
            $.punctuation
        ),

        comment: $ => /\/\/[^\n]*/,

        string: $ => choice(
            /"[^"]*"/,
            /\\\\\s.*/
        ),

        number: $ => /[0-9][0-9_]*(\.[0-9_]+)?/,

        intrinsic: $ => /#[a-zA-Z0-9_]+/,

        keyword: $ => choice(...KEYWORDS),

        type_identifier: $ => /[A-Z][a-zA-Z0-9_]*/,

        identifier: $ => /[a-z_][a-zA-Z0-9_]*/,

        operator: $ => /[+\-*/%<>=!&|^~?]+/,

        punctuation: $ => choice('(', ')', '{', '}', '[', ']', ',', ':', ';', '.', '@')
    }
})
