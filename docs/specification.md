# Draft Specification of the Strawberry Programming Language 🍓

The Strawberry Programming Language is an abstract meta language focused on portability and expressiveness
through a message based compile time metaprogramming and reflection system and a focus on generic programming.

### Motivation

Strawberry is self describing and portable. It's primitives are implemented in the core module in terms
of intrinsic backend messages and category metaprogramming.
It is aiming to be the most advanced object oriented language yet, something most languages failed at.

It is the intention to allow anyone to easily add even esoteric backends, with no dependencies of any kind,
and almost no budget or time investment required. A portable language which can be freely cross compiled,
taking great care not to expose any concrete implementation details in the core libraries. Additionally,
constructs used without needing to lower intrinsic messages are allowed even if a message is not understood,
so the language should scale even to targets like an 8 bit 6502 or GPUs. It could even use a completely
different core library, completely changing the set of available primitives for the most extreme of targets.

Strawberry is a language derived from many existing languages, aiming to extract a simple language
capturing their best strengths and learning from their downsides.

Inspiration includes but is not limited to:
- Swift, a language that inspires Strawberry the most, one that also combines lots of great ideas
  in one. Unfortunately the implementation is incredibly complex and unportable, and over time
  there's been a lot of mistakes that detract from the language for me.
- Hylo, a really cool language aiming to further explore mutable value semantics as somewhat present in Swift.
- Smalltalk, a beautiful and self describing language, but it does so with extreme dynamism.
  Strawberry is aiming to provide completely customizable classes which can interoperate with
  any system like Java or Swift or use completely custom representation. Generally though
  it relies on generics and categories and move/value semantics and the default approach
  for classes is limited to larger systems (like games) as that's where live editing of the environment
  enables really amazing things.
- Haskell, a fascinating language that presents a very different world view. It is too restrictive by itself
  but the ideas are very applicable in less pure languages.
- C++, an incredibly powerful language always experimenting with overly ambitious ideas.
  Unfortunately it is the least portable language in existence, and so powerful that static analysis is barely there,
  not to mention it's trivial to write unsound or undecidable code. It also does not exist, because there is no
  one complete implementation of C++, all of them are incomplete in different ways.
- Zig, a language with near arbitrary compile time evaluation, but one that's intentionally preventing
  metaprogramming, which is simple in it's own way but very utilitarian. Is not fun or interesting to me,
  and does not carry forward great ideas of the past.
- Rust, a language that on the surface appears to address the goals of Strawberry but unfortunately
  does not model the world in a generic way, and takes esoteric shortcuts for convenience such as
  implementing default ordering for tuples (just generally treating traits as interfaces, disregarding laws/semantics).
  Lifetimes are also an overly complex solution to balancing efficiency with speed copared to mutable value semantics.

I love all these languages (and many more) so I wish to create one of my own that lets me have fun.
Maybe it turns out to be a nice language.

### Disoriented Programming

A question I asked myself many times is how Strawberry should be "oriented". I concluded that the concept
of orienting oneself around a way of thinking is the very definition of thinking inside a box. In that sense,
the language should not be oriented at all, but rather a language for expressing complex type relationships and logic.

## Language Concepts

### Comment Selectors

Strawberry expands on the idea of semantically significant comments such as TODO or documentation comments
as seen in many existing languages.

The language reserves comment selectors (characters past the `//` until the space). Currently the following comment
categories are being considered for inclusion in the language and are being tested in the standard modules:

- `//` Free comment with no semantic meaning.
- `///` Documentation comment, attached to the following declaration it provides additional context.
- `//?` Module comment, attached to the module/header combination it's located in.
- `//!` Justification comment, used for unsafe code or imports to justify their use.
- `//:` Task comment, specifies a TODO item and optional subtasks to track progress and assignment.
- `//-` Section comment, divides a source file into multiple documented sections.

The syntax so far is specified like this:

```
module bar
//? --- Foo ---
//? This is an implementation of Foo logic, a safe binding to to some very unsafe stuff.

//! Required by Foo for important stuff.
import unsafe_stuff

// A comment that doesn't do anything relevant, perhaps complaining about the design of the unsafe_stuff library :)
// These are not intended for tooling use in any way.

//- Foo Stuff ----------------------------------------------------------------------------------------------------------
//- This is where the stuff happens.

/// This object does stuff.
///
/// It can be used like this:
///
/// ```
/// Foo.bar() // Stuff happened!
/// ```
//: TODO(Lua): Implement remaining functionality.
//: [x] Implement the unsafe stuff invocation.
//: [ ] Find a safer API for bar.
pub object Foo {
    pub fun bar() {
        //! This is just calling some C code which does the stuff,
        //! the calling convention and signature are verified to be in line with specification.
        unsafe do_stuff()
    }
}

```

These comment types can be recognized by the tooling and assist with linting or other features. For instance,
one can imagine a tool which lists undocumented imports or unsafe usage, or a setting which makes those a warning.

### Ownership

### Projection

There are only a few concrete cases of "lvalue" like behavior which are required.
Unfortunately the variance relationships between them are rather complicated.

The three fundamental access categories are:

```
// --- Read ---
value[index]
value.property

// --- Write ---
value[index] = new
value.property = new

// --- Modify ---
value[index] += other
value.property += other
```

They are not different between accessors and collection subscripts so following examples will only show the former.

Notably for copyable types the last operation can be derived from the other two:

```
let mut temp = value.property // Copy
temp += other
value.property = temp // Move
```

This is not possible when dealing with non copyable types however.

For such types these operations branch out:

- Reading can produce either an owned or borrowed value.
- Writing is unchanged.
- Modifying requires direct access.

A solution to this problem is seen in C++ and Rust with lvalue references. Unfortunately that approach is
useless as it can't work with computed properties or subscripts.

The solution chosen by Swift is to use coroutines. Strawberry will instead just use closures.

Here are all the possible accessor signatures:

```
// --- Read ---
// Note that the borrowed return value does not imply an in place reference, the lifetime
// is tied but the language may choose to return by value for performance or implementation reasons.
get property(&self) -> Property { self.original + 2 } // More permissive owned variant
get property(&self) -> &Property { self.original + 2 } // Less permissive borrowed variant

// --- Write ---
set property(mut &self, _ value: Property) { self.original = value - 2 }

// --- Modify ---
mut property(mut &self, _ mutation: (mut &Property) -> ()) {
    // Normally this code could just run the mutation on the stored value,
    // but the added power of this approach is that it still works with computed properties.
    let mut temp = self.original + 2
    mutation(temp)
    self.original - 2
}
```

The language can then choose the most appropriate implementation based on which kind of access is required,
and also derive missing ones in cases where such derivation is possible.

Specifically, modification is derived for copyable get/set pairs which would otherwise require boilerplate
conditional extensions on all categories that need to support in-place mutation.

### Tiered, templated exceptions

There are many approaches to error handling explored by all the languages so far. Some even claim to be
"zero cost" in name despite having plenty of cost in multiple areas. Some tried making them explicit,
checked statically even, but curiously always stumbled in refinement of this idea.

The confusion seems to be fundamental, a strange misinterpretation of what the name itself means. To be exceptional
is to be rare, near impossible, in a truly unexpected state. Exceptions are not for logical control flow.

To illustrate how to draw this line between errors and exceptions, consider some examples. An allocation error
for example. They're technically possible all over the place, but effectively impossible for most software to
ever encounter, such as under a modern operating system. Most software will also never handle them, simply
letting them fall through and terminate. This means a lot of useless machinery emitted into the program
for the sake of an exceptional situation. Conversely, a network error when reading from a socket is not exceptional.
It is expected, and guaranteed in many cases. It is not meant to have anything to do with exception handling.

Examples of exceptions include:
- Allocation failure.
- Platform failure, like Vulkan missing an extension or SDL failing to initialize.

Examples of exceptions do not include:
- Failure of an inherently unstable system like network communication (that's a result).
- Failure to parse a number from a string (that's an optional).
- Internal failure such as a broken invariant (that's a panic).

The possibilities are of course endless but this should provide some clarity on where the line is drawn.
Exceptions do not concern expected failure or things the user can't react to like assertions.

With this in mind, the simplest and compile time choice Strawberry makes is to use a tiered template system.
A procedure may specify a list of exceptions it can throw or rethrow. This expresses an explicit flow of exceptions,
however this is in fact an exception template. An exceptional path which has no actual point of being
potentionally caught is not even there, and the root throw of the exception path will simply be lowered
to a panic call. The sysem is tiered, because the panic implementation can fall back on a platform exception system,
such as trapping, aborting or unwinding. A backend could expose a flag to choose when a platform offers multiple
possible choices.

This feature introduces the following syntax for specifying, throwing, forwarding and catching exceptions:

```
// This will be our exception source.
fun get_entity() -> Ptr<Entity> throws AllocationException {
    let entity: Ptr<Entity> = allocate_entity()
    if entity != .null then entity else throw AllocationExeption()
}

// Exceptions compose perfectly. In a more complex situation they could be a comma separated list.
// They are written following the signature as the return type is more important.
fun forwards_exception() -> Ptr<Entity> throws AllocationException {
    try get_entity()
}

// They are exceptional, so ignoring them is implicit. That's the entire point.
// This will instantiate get_entity with a panic invocation instead of a throw.
fun ignores_exception() -> Ptr<Entity> {
    get_entity()
}

// Exceptions are caught with a catch expression, much like a normal match expression.
// They do not need to be exhaustive of course or could be combined with the try keyword to forward remaining ones.
fun caller() {
    let a = forwards_exception() catch {
        AllocationException -> return print("allocation failed")
    }
    let b = ignores_exception()
}
```

The desugared form could be visualised as something like this, greatly simplifying the details like
name mangling and such:

```
// The instantiation for the call without a reachable catch.
fun get_entity() -> Ptr<Entity> {
    let entity: Ptr<Entity> = allocate_entity()
    if entity != .null then entity else panic(AllocationException())
}

// The imaginary result type, an enumeration of success and possible exceptions.
enum GetEntityResult {
    Success(Ptr<Entity>)
    AllocationException(AllocationException)
}

// The instantiation for the call with a reachable catch.
fun get_entity_except() -> GetEntityResult {
    let entity: Ptr<Entity> = allocate_entity()
    if entity != .null then .Success(entity) else .AllocationException(AllocationException())
}

enum ForwardsExceptionResult {
    Success(Ptr<Entity>)
    AllocationException(AllocationException)
}

// Forwarding has no cost in this system, unless it's forced to instantiate multiple composed exception results.
fun forwards_exception_except() -> ForwardsExceptionResult {
    get_entity_except()
}

// Ignoring exceptions also has no cost, we simply recursively instantiate the panic case.
fun ignores_exception() -> Ptr<Entity> {
    get_entity()
}

// Notice how it's effectively templating result handling in an opt-in way.
fun caller() {
    let a = match forwards_exception() {
        .Success(let v) -> v
        .AllocationException -> return print("allocation failed")
    }
    let b = ignores_exception()
}
```

It's a lot of boilerplate which would have to be managed manually for the very few cases of software that
actually has to care. That's what templated exceptions solve.

The duplication of instantiation even when actually used, which is almost never, would still be a tiny amount of
code, a far smaller cost than the machinery required for traditional stack unwinding exceptions.

The cost is purely compile time, in line with the philosophy of the Strawberry Programming Language.
