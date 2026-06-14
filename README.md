# The Strawberry Programming Language 🍓

Strawberry is a unique in its ideals generic programming language with no unspecified behavior designed for
guaranteed portability, expressive power and performance.

It looks and feels familiar to anyone used to modern languages like Swift or Rust, but it differs
in how the foundation of the language is specified and how its implemented.

## The greatest enemy of portability, unspecified behavior

Strawberry explicitly acknowledges unspecified behavior as a far greater evil in relation to undefined behavior,
as it is unacceptable for a modern platform independent language to legally change program meaning between targets.

## Motivation

Programming should be as independent of platform as realistically possible,
helping developers build portable software for everyone, without difficult to find surprise logic bugs
that often make the cost of doing so highly prohibitive.

## Highlight features

- Safety: The language contains all undefined behavior potential in unsafe code.
- Portability and determinism: Unlike practically every existing language the specification is absolute, a program
will never change meaning between targets even if that means a compilation error to alert the programmer.
- Monadic loops: Fully generalized for-in-else loops for all monadic alternative applicative functors
rather than being hardcoded to the most basic of them all, the optional type, as most languages tend to do.
- Destructive move semantics and clean lifetime syntax.
- Strict floats: The language implements floating point numbers without sweeping a single property under the rug,
forcing code to handle them in painful detail. Fixed point and other representations are generally preferred.
This is in line with the philosphy of deterministic code.
- Guaranteed eager constant evaluation.
- Constant evaluation independent of target platform: Code for constrained targets can make use of
all functionality, even if unavailable, as long as it is evaluated at compile time.
- Variadic generic packs, compile time metaprogramming and reflection with annotation values.
- Contained support for target properties: Unportable semantics like pointer arithmetic or linear memory
are opt-in library features allowing most code to be shared between esoteric platforms, and the language
can scale from 8 bit CPUs through unusual heterogeneous architectures historically difficult to program.
- Projections with checked lifetimes: Reference semantics with support for computed properties and strict provenance,
independent of the concept of pointers to support more abtract targets.
- State of the art, highly generic core library.
- Strict grammar: The grammar specifies away most stylistic choices so that there is no room for pointless debate
and no need for external formatting.
- Extremely clean, easy to read syntax with overloadable argument labels for both arguments and generic parameters.
- Everything is an expression.
- Minimal compiler magic: Near all sugar is library implementable, not hardcoded to the core module types.
