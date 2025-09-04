# Specification draft for the Strawberry Programming Language

The Strawberry Programming Language is an abstract meta language focused on portability and simplicity
through expressive compile time metaprogramming and modular implementation.

The language must scale to anything from old 8 bit computers to modern hardware or completely exotic environments
like a GPU, webassembly or the JVM. It is intended as an ultimate language design which can easily adapt
into arbitrary runtime mechanisms. The source code can introspect itself and transform itself arbitrarily at the
type level which should serve extreme use cases like reflecting a method at compile time and invoking itself
to compile it for the GPU and execute a pure functional chain as a fragment shader.

The language must be simple and naive in implementation, focusing on leveraging existing environments but also
being self sufficient. It must be fairly transparent to implement a simple backend to lower for a console like the NES
or LLVM IR for efficiency competitive with other such languages on conventional platforms. Critically, the idea is
to modularize such that adding backends and cross compilation is easy.

## Builtins

There is a special namespace, `builtin`, which contains intrinsic types and functions used to declare the
actual primitives in the core library itself.

## Standard library

The portable, generic library which defines the language's primitives and collections is called `core`.
There is no IO of any kind covered by the core library.

There are however other libraries included which provide such abstractions for various platforms. They are not
considered standard since they inherently depend on the target platform and it is only reasonable to build
higher level abstractions on top. It is a mistake for a standard library to do otherwise because there is no
good way to abstract IO without tradeoffs on various platforms. I think that modularity through architecture is better,
without any universal IO abstraction.

### Core primitives

There are several fundamental primitives defined by the core library, their availability can depend on the platform.
- `Integer` which is the arbitrary precision two's complement integer type.
- `Decimal` which is the arbitrary precision binary coded decimal type.
- `InlineArray<T, COUNT>` which is a straightforward inline array
- `Int<SIZE>` and `UInt<SIZE>` which describe arbitrary size itegers in a generic way.

### Collections

There are some commonly used collections provided by the core library.
- `Array<T>` which is a contiguous resizable array.
- `RingArray<T>` which is a contiguous resizable ring buffer array.
- `HashMap<K, V>` which is a standard hash map.
- `HashSet<T>` which is a standard hash set.

Still, these are just implementations. The language expresses these concepts more broadly, so that code need not
be tied to them. This is done through useful traits:
- `Sequence`
- `Collection`
-

There is also a related family of iterators for operating on them.

### Graphics library

Provided is a completely portable generic abstraction for raster graphics. I generally dislike overly high
level abstractions but this actually isn't one. It's a functional iterator like lazy composition of bitmap mapping,
effectively just 2d collections of pixels to match the conventional 1d library, with specialized semantics for
graphics composition.

## Declarations

There are more kinds of declarations in this language compared to most other languages.

## Traits

Strawberry has the concept of traits, but while the name is inspired by Rust they are actually a mix
of my favorite semantics of Rust's traits and Swift's protocols. They have methods, associated types but also generics
like Rust but I dislike how default method implementations and overriding (as well as dyn compatibility) is handled
so I drew from Swift instead with its more intuitive and expressive extensions.

Here's a simple example of abstract programming with traits:
```
pub trait Foo<T> {
    type Bar
    // Much like Rust and unlike Swift, overrideable defaults can be provided inline.
    fun bar(T t) -> Bar { ... }
}

extend<T> Foo<T> {
    // Like Swift however there can be blanket non overrideable implementations in terms of the trait.
    // This is similar to the hack of extension traits in Rust but more streamlined like in Swift.
    pub fun bar2() -> (Bar, Bar) = (Self.bar(), Self.bar())
}

// Trait union syntax looks like this
type FooBar = Foo and Bar
```

## Arithmetic

Strawberry does not have any built in operators. Instead, one can declare prefix, infix and suffix operators.
Operators can be symbols or identifiers and they are determined by whitespace, it is illegal to write code like `1+2`
because it's unintuitive.

Strawberry also does not have precedence or associativity of any kind. There is no good solution for this as precedence
is a globally scoped problem, this inelegance leads me to preferring parentheses even for conventional arithmetic.

## Conditional logic

```
// Strawberry uses named operators instead of archaic && and || operators.
// They short-circuit as usual but they are left associative, without the unintuitive precedence.
// Use parentheses for grouping instead.
if a and b or c

// While conditionals are always expressions, they can be expressed with ternary syntax without the braces.
if a and b then 42 else 0
```
