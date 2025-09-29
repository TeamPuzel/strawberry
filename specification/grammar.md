# Grammar for the Strawberry Programming Language 🍓

This is currently not the complete or canonical grammar, but a scratch space to sketch the grammar out while working
on the reference parser implementation.

The language is whitespace sensitive and should enforce that whitespace is always exactly 1 space wide unless
the pattern explicitly allows more, such as for indentation.

## Syntax

- `ident` is an identifier in the language.
- `newline` is of course a line terminator.
- `(<pattern>)+(<pattern>)` a sequence of at least one repetition, where second pattern represents
  the separators. For instance, `ident+,_` means comma and whitespace separated list of identifiers.
- `(<pattern>)?` represents an optional pattern.
- `...<pattern>` skip arbitrary sequence until a specific pattern.

## Specification

```
SOURCE = (DECL newline+)+

EXPR =

DECL = MODULE
     | USE
     | STRUCT
     | ENUM
     | CATEGORY
     | EXTENSION
     | CLASS
     | CONSTANT
     | STATIC
     | FUNCTION
     | TYPE_ALIAS
     | CATEGORY_ALIAS

// A declaration of a module at the start of a source unit.
MODULE = "module" _ (ident)+.

// A declaration bringing in another module.
USE = "use" _ (ident)+. (_ "as" ident)?

// A sequence of statements which composes a block of code.
STATEMENTS = (STATEMENT)+separator
where
    separator = newline+
              | ";" _

STATEMENT = binding
          | branching
          | matching
          | EXPR
where
    binding = ("const"|"let"|"let" _ "mut") _ ident (":" _ EXPR)? (_ "=" EXPR)?

// A generic list of constant and type parameters.
GENERICS = "<" (parameter (_ "=" _ EXPR)?)+,_ ">"
where
    parameter = "const" _ (ident _)? ident ":" _ EXPR
              | (ident ":" _)? ident

ARGUMENTS = "(" (argument (_ "=" _ EXPR)?)+,_ ")"
          | "(" self_argument ("," _ (argument (_ "=" _ EXPR)?)+,_)? ")"
where
    argument = ("mut" _)? (ident _)? ident ":" _ EXPR
    self_argument = ("mut" _)? "&"? "self"

WHERE_BOUNDS = "where" _ EXPR

// A comment, generally discarded as it's of no semantic value.
COMMENT = "//" _ ...newline
// A documentation comment, combined across multiple lines and attached to declarations.
DOC_COMMENT = ("///" _ ...newline)+

ANNOTATIONS = annotation+newline
where
    annotation = "@" TODO

INHERITANCE = (":" _ EXPR+,)

STRUCT = prelude GENERICS? INHERITANCE? ((newline|_) WHERE_BOUNDS)? (_ body)?
where
    prelude = ("pub" _)? "struct" _ ident
    body = "{" newline (member newline+)+ (DECL newline+)+ "}"
    member = ident ":" _ "type

ENUM = prelude GENERICS? INHERITANCE?
where
    prelude = ("pub" _)? ("open" _)? "enum" _ ident

CATEGORY = prelude GENERICS? INHERITANCE? ((newline|_) WHERE_BOUNDS)? (_ body)?
where
    prelude = ("pub" _)? ("unsafe" _)? "category" _ ident
    body = "{" newline (DECL newline+)+ "}"

EXTENSION = prelude INHERITANCE? ((newline|_) WHERE_BOUNDS)? (_ body)?
where
    prelude = ("unsafe" _)? "extend" _ ident
    body = "{" newline (DECL newline+)+ "}"

CLASS =

CONSTANT = ("pub" _)? "const" _ ident (":" _ EXPR)? _ "=" _ EXPR

STATIC = ("pub" _)? "static" (_ "mut")? _ ident (":" _ EXPR)? _ "=" _ EXPR

TYPE_ALIAS = ("pub" _)? "type" _ ident GENERICS? _ "=" _ EXPR

CATEGORY_ALIAS = ("pub" _)? "static" (_ "mut")? _ ident GENERICS? (":" _ EXPR)? _ "=" _ EXPR

FUNCTION = ("pub" _)? ("async" _)? (operator _)? "fun" _ ident GENERICS? ARGUMENTS EXCEPTIONS? ("->" EXPR)? body
where
    operator = ("infix"|"prefix"|"postfix") _ "operator" _ ident
    body = "{" newline (STATEMENT newline+)+ "}"
```
