# Reflection for the Strawberry Programming Language 🍓

The intent of this document is to specify meta reflection capabilities.

## Motivation

There is great value in metaprogramming to avoid repetitive and potentially error prone implementation.
There are many approaches to this, from text based preprocessing and code generation to token based macros
and finally full compile time introspection of the types themselves.

I believe that the type safety widely agreed upon to be important should be applied to metaprogramming as well,
and that compiler plugin based procedural macros as seen in Rust and Swift are a mistake and offer no value
over type level introspection.

## Annotations

Reflectable constructs can be annotated freely with the following syntax:

```
annotation Serialize {
    pub init(rename: String) {}
}

struct Point<T> {
    @Serialize(rename: "X")
    x: T
    y: T
}
```


## Meta types

The following types are primitive to the language:
- `Self`, the concrete type of the current scope, placeholder for the concrete implementation when used in categories.
- `Type`, the type of types.
- `Type.Member`, the type of data members.
- `<modifiers> (<argument list>) <exception specification> -> <return type>`, the type of functions.

## Meta syntax

The syntax of meta operations uses `::` instead of `.` providing clear visual distinction.

### Accessing types

To access a type itself the syntax is `T::Type`.
