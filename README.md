# The Strawberry Programming Language 🍓

Strawberry is a unique in its ideals generic programming language with no unspecified behavior designed for
guaranteed portability, expressive power and performance.

It looks and feels familiar to anyone used to modern languages like Swift or Rust, but it differs
in how the foundation of the language is specified and how its implemented.

Effectively, Strawberry is a pure functional language, at first glance imperceptibly disguised as a traditional language.
Compilation is a conversation between the one standard constant evaluator which tries to resolve either
a Type or a Value, eagerly evaluating all code at compile time, and the backends. Yes, all backends at once!
Eventually the evaluator will run into intrinsics or even code without implementation (like a function without a block).
At that point it will attempt the following in order:

- If the intrinsic is correlated to a backend, like `ps2.fadd(expr, expr)` (the EE fpu EeFloat addition) it will
first try ask the ps2 backend for it, regardless of our current target.
- If not resolved it will next attempt to ask the target backend the same way.
- Finally it will attempt to resolve the intrinsic on its own, many core intrinsics like `#smul(expr, expr)`
(the intrinsic behind `Int.multiply(self, with: Self)`) have a standardized specification
the evaluator already knows how to solve.

We can't just evaluate everything at compile time however, that's where we can borrow the solution from
the world of functional programming, transparently to the programmer. This is what motivates
the third expression category, the Residual. It is the impure IO as seen in languages like Haskell,
but it's not handled explicitly, just an inherent property of how the language works under the hood. Any expression
which depends on a residual propagates, becoming residual itself. It is the residual tree which forms the actual
runtime logic of the program in the end.

A similar mechanism is used for unimplemented declarations. When the evaluator needs a function without a body block
for example, it will ask the target backend if it knows how to resolve it. Perhaps the backend will then
consider it, like if it has an `Extern` annotation, and provide a residual for an extern call.

Eventually the entire program is hopefully evaluated without errors when it is finally handed off to the target
backend with the final task: handle all residuals. It could mean lowering the residual program to code,
interpreting it, anything, anything really.

This means that this is a valid 6502 program:

```
module main

import ps2

@Extern
@Convention("C")
@unsafe SymbolName("print_number")
unsafe fun print(number: UInt<8>)

@Entry
pub fun main() {
    let a: EeFloat = 1.0
    let b: EeFloat = 1.0

    let result = UInt<8>(truncating: (a + b).radix) // Forms a Value eagerly, not a Residual
    unsafe print(number: result) // Residual call, but its argument is a Value.
}
```

Because the ps2 specific floats are evaluated entirely at compile time we never actually reach the condition where
the 6502 backend attempts to resolve a Residual of `ps2.fadd`.

A game for instance can just have a compile time conditional to different platform specific main loops,
and reuse most code across platforms without any friction, with the only requirement being that the required
residual expressions don't run into an intrinsic not supported by the current target backend.

This is a departure from traditional approaches to portability which simply see languages specify themselves
loosely enough that the language can subtly change meaning in order to forcibly compile between them.
That means the meaning of the program can change however, which is unacceptable, almost always introducing
hidden bugs. In Strawberry the semantics are exposed directly, and it is a compile error asking for manual intervention
when the platform can't satisfy the program, surfacing what would otherwise be a potentially
never discovered security vulnerability, waiting to be exploited.

It could be argued that this adds friction to porting code. It's actually not true, most languages simply
sweep it under the rug, but the issues are still there, implicitly ignored, creating the illusion of portability.
Strawberry is designed to guide the developer through the process, pointing out every problem without fail.

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
