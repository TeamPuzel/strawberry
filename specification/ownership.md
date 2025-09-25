# Ownership for the Strawberry Programming Language 🍓

## Motivation

There has to be a way of reasoning about ownserhip of types in order to efficiently implement many primitives.

## Copyable types

When types are Copyable it means that it's trivial to copy them in a bitwise fashion. Effectively consuming still
leaves a valid copy in place when moving the type's instance.

A type can only be Copyable if all of its structural members are also Copyable.

```
struct Foo: Copyable {}

fun foo(bar: Foo) {}

pub fun main() {
    let a = Foo()
    let b = a // New copy, a remains valid

    // we can freely implicitly copy it around as much as we want without worrying about ownership.
    foo(a)
    foo(a)
    foo(a)
    foo(a)
}
```

## Ownership

For most cases however generic code will need to specify the convention by which it receives an argument,
unless it's constrained to only work on Copyable types.

```
struct Bar {}

// A consumed value is much like passing by value, and it is generally implemented the same way, however
// the original is no longer valid and a compile error to access.
fun foo(bar: Bar)

// A borrowed value is a less permissive view of a type. It can't be moved from or affected in any conventional way.
// There can be many aliasing borrows of a type and for as long as they are in use
// consuming or mutating the original value is not allowed.
fun foo(bar: &Bar)

// A mutably referenced value is similarly permissive to ownership but it's forwarding to the original.
// There can only be one mutable reference and aliasing is not possible. The main limitation is
// that the original value can't be destroyed this way, so it can't be left in an invalid state.
// It can be moved from as long as it is initialized again on all paths.
fun foo(mut bar: &Bar)
```

Notably passing a mutable reference requires explicit syntax opt-in:

```
foo(bar: &b)
```

The same syntax applies to all other value bindings in the language, for example variables.
When the type is inferred the modifier is applied to the name itself instead of the type.

```
mut a: T = T()
let b: &T = a
mut c: & = a

mut d: T = T()
let &e = d
mut &f = d
```

## Lifetimes

This is fairly useful so far but it does not account for the ability to escape references, for example
storing them inside a struct. To achieve that there has to be a way to tie lifetimes together in a signature
and across containers.

```
fun foo(bar: &'a Bar) -> &'a Bar { bar }

struct BarContainer<'a> {
    bar: &'a Bar
}

fun baz(bar: &'a Bar) -> BarContainer<'a>
```

## Lifecycle

Since all ownership is tracked there can only be one owner at a time. Once that owner is destroyed
that is the end of the value's lifecycle and its deinitializer runs. This means that deinitialization
is very optimizable and only happens once.
