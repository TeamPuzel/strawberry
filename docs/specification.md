# Introduction
The most fundamental principle of the language is
that it is a self contained specification. A program
written in Strawberry will always have the exact
same semantics. Undefined behavior is possible
through unsafe constructs as at some point there
has to be a way to integrate with the computation
environment, but the Strawberry language must
never, under any circumstances, have a far more
severe design mistake: unspecified behavior.

A fundamental flaw with many programming languages,
such as C/C++ or Rust, is that the meaning of the
program can change based on unspecified context.
If Strawberry code is to be truly portable, to
platforms of the past and future, this is not
a mistake that can be made.

Any change to the language that would break this
rule is fundamentally incompatible and is to be
rejected without any consideration. Strawberry is
a singular language, not a specification of a closed
(or worse, open) set of programming languages.

How should the language actually handle platform
differences then? Instead of handwaving the issue
away, the semantics must be preserved. If a platform
does not reflect a capability (such as the PS2 using
a non compliant floating point unit) a backend has
exactly two choices when implementing intrinsics:
- Implement the semantics in a different, possibly
less efficient but conformant way such as software.
- Refuse to compile the program if a usage exposing
non compliant semantics was residualized.

If the host is incapable of evaluating something
at compile time then it must also raise a hard error.

To provide platform specific host or target semantics
distinct types with distinct intrinsics could be made.
Support for the PS2 for example would implement
a distinct EeFloat. It is up to code to accurately deal
with such constructs. Changing the interpretation
of the specification, unspecified behavior, is
just toolchain level monkey patching.

Strawberry must be defined so precisely that
only one interpretation can exist (within reason).
The core library and the eventual self implementation
is a continuation of this specification in terms of
the already well defined basics here.

# Zero cost?
There is no such thing as defining cost. The language
aims to be as efficient as possible under the
strict purity constraints, but the same semantics
can have wildly different "cost" depending on the
target backend or anything else. Strawberry is
a language designed to abstractly represent logic
completely independent of how that is achieved, if
at all. A Strawberry program can be written on paper
and translated into some assembly by hand and there
must be no ambiguity as to the results.

Effectively, Strawberry is a unified mathematical
notation which describes meaning, not behavior.

Ultimately, the language is to be described by itself,
not any kind of abstract machine.

# Evaluation order
Strawberry is evaluated top to bottom, left to right.
This rule is absolute, a serious language can't
have any ambiguous syntax which could affect the
result of a program.

This is different from other languages which often
do not guarantee the order expressions are evaluated.
in C and its derivatives it is common for example
that `foo() + bar()` leaves the order unspecified
so that a more naive compiler can be made.
This is obviously bad from a formal verification
perspective; code should be written once and work
until the rest of time, so it can't have valid
grammatical expression which does not have a
strict unambiguous meaning.

This is most important for mutation:
```
new Player(
    x: reader.read<Fixed>(),
    y: reader.read<Fixed>()
)

This is very straightforward code, a function call,
specifically an initializer of a class type.
In Strawberry the order is defined very simply:

The grammatical structure, resolved with the
precedence rules into a syntax tree as presented
in the standard s-expression like notation,
is by definition already aligned to the evaluation
order. The evaluator must resolve this tree without
changing the order everything was parsed in.
```

# Primitives
The fundamental primitives have to be more abstract
than in most programming languages, but it is
inevitable that some dependency between the core
library and implementation will exist. All of that
will be exclusively done through having the language
communicate intent however, no actual knowledge
of the core library will be hardwired
by the specification and implementation.

There is a large amount of constructs which can
be defined purely abstractly however.

# Modules and source units
Strawberry is described through named modules
which are split into source units. Modules can
be nested and qualify declarations with their path.

Modules consist of source units. A source unit is not
specified to be a file, it could be anything, even
a sheet of paper with handwritten code. It is a
purely a unique part of a module grammatically
starting with a `module qualified.name` declaration.

# Realms: types, values and their residuals
A value is an instance of a type. A type is
a description of the properties its instances have.
A residual is the promise of a value, but one that
can't be evaluated at compile time. As a program
is written, residuals will likely come from
IO with the execution environment and cascade down.
Anything which depends on a residual value for its
evaluation becomes residual itself. Eventually,
when the entire program is evaluated it is left
with one or more residual trees rooted in points
exposed externally (usually an entry function or
dynamic library boundaries).

These residual trees are what is then used by
a Strawberry backend to form a program,
usually a binary. It can be anything or nothing at all,
including VM bytecode for interoperability
with existing VM languages, a custom VM, machine
code for any architecture or a high level abstract
machine like that of WASM, even something yet
unknown which will be important in the future.
Strawberry produces residuals which are a high level
representation of computation, so the variety of
possible backends is intentionally not restricted.

However, what intrinsics a backend can actually
lower the residuals of can vary. It is not expected
for a WASM or JVM backend for example to implement
linear memory residuals (like pointer arithmetic)
because it makes fundamentally no sense, and it is
the intention that portable, generic Strawberry code
will make use of abstract, semantic categories and
not concrete functionality for this reason.
One would use such unportable residuals
when writing low level platform specific code or
interacting with different language ABI, but it should
be contained to minimize impact on portability.

In the end Strawberry will be just as capable as
any other language in any particular domain it is
applied to, but the language itself is guarded
against inheriting any of it. It is late binding of
all leaf side effects to keep the language consistent.

There are a few fundamental types:
- Type, the type of types. It is also its own type.
- Enum, the supertype of all enumeration types.
- Class, the supertype of all class types.
- Struct, the supertype of all struct types.
- Annotation, the supertype of all annotation types.
- Self, which refers to the enclosing
enum, class, struct or annotation.
- Any, which refers to anything at all and is
generally only usable in const context.
- Tuples, structural types with dedicated notation
unlike that of nominal types.
- Functions are similarly structural types with
their own dedicated notation. The notation
supports more than just standard functions to
constrain more derived closure types.

The value and type realms have a bridge between
them in context of `const`. This means constructs
like annotations, const functions, const structs,
const expressions and so on
(contexts constrained to only compile time evaluation)
gain the meta access operator `::`.
The reason for this is to have distinct syntax which
will not collide with global type members.
It is much like the access operator `.` but produces
values reflecting the types themselves. The exact
specifics are specified later, but the general idea
looks like this:

```
fun print_members<of: T>() where T: Struct {
    // We access the const value of members which
    // makes this a const loop. We can then filter
    // the loop by a condition (which inherently is
    // also compile time), in this case
    for member in T::members
        where member is Property
    {
        print(member.name)
    }
}

...

@Entry
fun main() {
    // Will print "value" which stores the intrinsic
    // typed value of the int.
    print_members<of: Int>()
}

```

# Tuples
Tuples are a structural grouping of values. They
have a unique type notation, a parenthesized
comma separated expression where each element
evaluates to the type realm. Their values are
grouped with the same notation but where
each element evaluates to the value or residual
realm. For the sake of simplicity, if any element
of the tuple is residual the entire tuple becomes so,
even if the other elements are values.

```
let tuple: (Int, Int) = (0, 0)
print(tuple[0])
```

Tuples can also have their elements named:

```
let tuple: (x: Int, y: Int) = (x: 0, y: 0)
print(tuple.x)
```

But do note that the bound element names only care
about the current binding. Tuples are structural
in the following way:
- Tuples with the same size and element types
are equivalent.
- Tuples can gain and lose element labels, so
both cases are legal implicit conversions:
`let t: (x: Int, y: Int) = (0, 0)` and
`let t: (Int, Int) = (x: 0, y: 0)`
- It is not legal for the names to immediately
conflict, for example this is not allowed:
`let t: (x: Int, y: Int) = (w: 0, h: 0)`
This serves as a minor sanity feature helping
code avoid accidentally mixing such constructs.
- As a consequence of these conversion rules it
is possible to explicitly cast away the labels and
force the conversion:
`let t: (x: Int, y: Int) = (w: 0, h: 0) as (Int, Int)`

Furthermore there is the curious case of the
single element tuple, which has slightly different
semantics. If the tuple's size is known (it is
not if we are operating through a variadic pack)
and it is exactly one element, it immediately decays
to the value it contains. This is useful and very
recommended in order to name return types! Some
languages do allow naming multiple return values
(like Swift through its tuples) but unfortunately
do not support it for single returns.

```
// The actual signature of one of the Picotron
// print function overloads. It returns the pixel
// width of the printed text. While this style of
// code is not idiomatic for Strawberry, it is
// improved. There is no way to understand the
// result value from the signature itself
// without documentation, but thanks to the named
// single element return tuple it becomes clear!
@Discardable
pub fun print(
    _ string: &String,
    x: Int, y: Int,
    color: Color
) -> (width: Int) {
    unsafe Environment.print(
        string, x: x, y: y, color: color.id
    )
}

...
module main

import picotron

@Entry
fun main() {
    // This is exceptionally good for tooling as well.
    // For instance, when typing a call to
    // a function like this, an editor could suggest
    // a different autofill on held `alt`/`option` key
    // which would automatically insert the variable
    // with the correct name! Or for non discardable
    // results it should default to it, with the
    // alternate mode instead insert an
    // explicit discard `_ = foo()`.
    let width = print("Hey!", x: 8, y: 8, color: .red)
}
```

The last case of tuples is the empty tuple.
The empty tuple is an empty value; both the type
and value are written as `()` depending on the
expected expression realm.

# Enums
Enumerations are a set of possible values.
The most basic of all things is nothing at all, and
enums can represent that:

```
enum Never {}
```

An enum with no cases is a type with no values.
It can therefore be used to represent impossible
state; the core library provides one called `Never`
and it should always be used for consistency to
properly communicate intent. It is not possible to
have stored data of empty enum types.

Enums with a single case are also a bit unique.
They can only ever be one possible value, so they
don't actually mean anything on the value level;
only the type level matters. Because Strawberry
cares purely about the meaning, lowering of these
is free to omit their existence as data entirely,
unless the case is a tuple case.

# Functions, closures and coroutines
Closures are anonymous callable objects which
convert to functions if they don't capture anything.
The notation is shared but differs in meaning
between category and type context.

The grammar is that of a postfix `->` after
a tuple type, `() -> ()`, expecting a second tuple
afterwards. This describes all functor signatures.

To illustrate:
- In concrete type context:
```
struct Foo {
    let bar: (Int) -> ()

    init(bar: (Int) -> ()) {
        self.bar = bar
    }
}
```
Here it represents a standard function signature.
It is not allowed to have any captures as it has
to have a static lifetime and it is polymorphic
at runtime. It is a function not closure object,
but closures without captures can convert into them.
- In category context:
```
struct Foo<of: Bar> where Bar: (Int) -> () {
    let bar: Bar

    init(bar: Bar) {
        self.bar = bar
    }
}
```
Here it represents a generic function signature.
It is allowed to be anything at all with this callable
signature as it instantiates the type, and it is
not constrained further.
It inherits the lifetime constraints
from how it references captures,
projections and mutable projections constrain
the entire closure object, and in this case
it is then inherited by Foo which is generic over it.

Function categories follow normal category rules,
so by default they are not Copyable; our Foo
supports closures which move captures and can't
declare Copyable category conformance. On top of
that calling Bar will consume it so we can only
call it once.
This can of course be changed by adding
the constraint to Foo:

```
struct Foo<of: Bar>
where
    Bar: (Int) -> (),
    Bar: Copyable
{
    let bar: Bar

    init(bar: Bar) {
        self.bar = bar
    }
}
```

# Loops
There are three kinds of loops in the language.
- loop (manual loop)
- while (pattern loop)
- for (monadic loop)

The manual loop simply iterates indefinitely:
```str
loop {
    print("I will forever print :)")
}

// Single expression loops can omit the
// block expression with the `do` keyword.
loop do print("I will forever print :)")

// It can be broken out of, potentially with a value.
let result = loop {
    break 0
}

// Breaking out of nested loops uses labels, and there
// is a continue keyword to skip the iteration.
:label loop {
    if condition then continue else loop {
        break :label
    }
}
```

The while loop iterates for as long as the
conditional pattern matches. The rules here
are identical to conditional expressions.

```str
// That's about it, there is not much else
// to these loops. It's just more often than not
// far easier to read than the manual loop with
// a nested match expression or pattern branch.
while .some(let value) = iterator() do print(value)
```

The monadic loop resembles iterator based loops
from other languages, but it is generalized not on
the iterator itself but the monad it returns.

Specifically, any functor is an iterator provided that
for it's return type (R):
- R conforms to Monad
- R conforms to Alternative

There is a slight added constraint depending on
the type of the alternative discussed later.

The for loop looks like this:
```str
for element in iterator {
    print(element)
}

// The above loop desugars roughly to this.
loop {
    let mut end = false
    iterator()
        .and_then(|element| print(element))
        .or_else(|_| end = false)
    if end then break
}

// In all cases the result type must be
// a Monad<of: Element> (to use and_then)
// The prior loop is also an example of
// an Alternative<of: ()> (empty alternative)
// For loops can have an `else` branch to
// perform custom alternative actions and
// they are a () expression, so a postfix `catch`
// can be used on the entire loop.
// The following is the same loop as before:
for element in iterator {
    print(element)
} else {
    break
}

// Note that not having the break would create
// an infinite loop which simply discards
// the alternatives.

// And for completeness a catch looks like this:
for element in iterator { foo() } catch { ... }
// And the clean `do` syntax is also possible
for element in iterator do print(element)

// Now, why is there an else branch on a loop?
// It might seem odd, but while useful in some
// cases already, it becomes far more powerful
// when used on an Alternative where the alternative
// type is not the empty tuple (like usual Optional).
// Case and point, the Result type:
fun listen() -> Result<of: Data, error: NetError>

for data in listen {
    print(data.message)
} else error {
    log(error)
}

// This is incredibly powerful and the else branch
// is of course required when the alternative
// is not of type ().

// For loops also support the continue keyword.

// Elements can be filtered out with a `where`
// expression much like that of conditionals.
for element in iterator where element.x > 2 { ... }

// Bindings can also be bound accordingly
// based on the usual binding rules, including
// the shorthand for projection qualification.
for element in iterator {}
for mut element in iterator {}
for &element in iterator {}
for mut &element in iterator {}
for element: ElementType in iterator {}
for mut element: ElementType in iterator {}
for element: &ElementType in iterator {}
for element: mut &ElementType in iterator {}

// The signature of the for loop is used for
// inference of the Iterable<with: Iterator> category,
// where the category implementation is chosen based
// on the inferred iterator functor signature.
for mut &element in collection do element *= 2

// Lastly, a for loop which handles errors and
// monomorphised exceptions looks something like this.
for data in listen {
	handle_data(data)
} else error {
	log(error)
} catch {
    let e: AllocationError -> {
        let success = try_recover_memory()
        if success then continue else break
    }
}

// Note that the catch is attached to the entire
// loop expression so it handles both branches
// of the for loop, but one can still catch inside
// the branches on individual expressions or
// entire blocks.

// Also note how this is a good example of the
// very important Exceptional rule. Exceptions
// are in fact exceptional and are not to be
// used for expected error handling. If not handled
// they implicitly monomorphise the caller to
// immediately panic on throw. This is great for
// very rare and unlikely on many platforms
// exceptions like memory allocation, but it is
// not the correct way to deal with most errors.
```

== Blocks ==
Sometimes expressions are not enough and
statements are required. Blocks use the syntax `{}`
and are a sub-expression of many language features.
They are the lexical scopes of Strawberry.

# Experimental extended primitives
== Async ==
Async is not a finalized design and exists as an
experimental extension to the language intended
to more efficiently distribute work.

The design is heavily inspired by Swift which has
a great concurrency model really easy to integrate
with platform concurrency primitives.

Async introduces async functions:

```
// Concurrency usually requires less portable
// language constructs like addressable linear
// memory, but it is inherently tied to the platform
// in the first place. There is no concurrency without
// low level platform code becase that's what it is
// intended to abstract away in higher level
// frameworks which implement executors.
//
// In this case the async function wraps the
// callback of the 3DS vblank to schedule a new
// frame. While this is awaited the executor can
// freely execute background tasks on the main
// thread between the end of the frame and the
// start of the next without any special
// synchronization code as everything is unified.
fun wait_for_vblank() async {
    MainExecutor.assert()

    await with_continuation(|c: Continuation| {
        let box = Box(c)

        unsafe gspSetEventCallback(
            GSPGPU_EVENT_VBlank0,
            |userdata| {
                let box = unsafe Box(
                	raw: userdata
                	    .bind<to: Continuation>()
                )

                Task(
                    on: MainExecutor,
                    priority: .high,
                    ||:[box] box.resume()
                )
            },
            unsafe box
                .to_pointer()
                .as_raw()
        )
    })
}
```

TODO: Further async specification.

# Comments
Comments in Strawberry have a strict definition,
stricter than most languages. A comment is one
or more sequential lines or a single comment
at the end of a line. A comment is made of three
parts with semantic meaning:
- The comment prefix.
- The comment selector.
- The content.

Here is an example:

```
//! This is a safety documentation comment.
//! They are used to document the usage of
//! unsafe code in a structured way easy to
//! understand by tooling.
unsafe foo()
```

The above example contains the prefix `//` which all
comments start with. The selector in this case is
of course `!` which specifies a safety comment.
Comments with non empty selectors all have
hard constraints as to how and where they can be
used as well as unique syntax. The content follows,
but the space is mandatory. The language cannot
simply reserve a few selectors and treat others
as comments, it would be inconsistent and prevent
future evolution by adding new kinds of comments.
For this reason it is also a hard error to use
a selector which is not understood or used
in the wrong place.

Comments with an empty selector, normal comments,
have no semantic meaning and are discarded early
by the tokenizer itself and not even the parser
is aware of them. This simplifies the grammar
as there is absolutely no use case for keeping them.

Currently the supported comments are:
```
//  (pure comment)
/// (documentation comment)
//! (safety comment)
//: (todo comments)
```

== Documentation comments ==
TODO

== Safety comments ==
TODO

== Todo comments ==
TODO

# Grammar
Strawberry starts off as source code, but it is
resolved into a syntax tree in terms of tokens.
The Strawberry tokenizer has a few highly desirable
properties which are useful to note, in particular
that any unique line of valid Strawberry code
can be tokenized in isolation. That means there
are no multiline constructs such as comments
or strings, instead comments are written as
sequential lines the parser then resolves into
a single comment if it has a non empty selector,
otherwise discarding text until the next line.
Multiline strings on the other hand are similarly
individual lines with the prefix `\\` where
the parser concatenates them as well.

The tokens of the language are as follows:
