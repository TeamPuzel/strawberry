# Reflection for the Strawberry Programming Language🍓

The intention of this document is to specify meta reflection capabilities.

## Meta types

The following types are primitive to the language:
- `Self`, the concrete type of the current scope, placeholder for the concrete implementation when used in categories.
- `Type`, the type of types.
- `Type.Member`, the type of data members.
- `<modifiers> (<argument list>) <exception specification> -> <return type>`, the type of functions.

## Meta syntax

The syntax of meta operations uses `::` instead of `.` providing a clear visual distinction.

### Accessing types

To reference a type itself the syntax is `T::Type`.
