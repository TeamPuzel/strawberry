# Draft Specification of the Strawberry Programming Language 🍓

The Strawberry Programming Language is an abstract meta language focused on portability and readability
through expressive compile time metaprogramming and a modular, extensible implementation.

This part of specification intentionally only deals with abstract semantics. The core library itself is not meant
to be reimplemented and therefore is not specified here; it is specified by its source. The fundamental idea
for Strawberry is to allow writing generic code with no need to rewrite libraries over and over across platforms.
A compiler simply has to provide the builtins satisfying the contracts described by the core library constructs.

In fact, the specification is really simple and not overly formal.

## Comments

Comments start as `//` with an arbitrary space terminated pattern following. Currently the language has
a few comment patterns defined but this reserved naming allows future expansion without source breakage.

```
// Comments should be capitalized and punctuated properly.
/// Documentation comments must precede a declaration and are used by the documentation generator.
```

To clarify, the leading space is in fact mandatory to reserve a place for language evolution if one day
more comment types are adopted by the industry.

> One such idea is **task comments**. Task comments are not fully designed yet but they are essentially TODO lists
one can attach to declarations. This would include syntax for expressing what something is being blocked by,
and in turn the tooling would be able to generate a graph helping the developer know what to focus on first,
as well as letting contributors pull the repository and see all the things which need work. This could be integrated
with external task management solutions in an editor to provide status on who's working on particular tasks.

## Line width

Lines should be up to 100-120 columns wide. The language must warn about crossing the 120 column boundary
as while not an error, the hard limit is implementation defined as anything equal to or greater than 120 columns.
This is to allow highly memory efficient implementations on heavily constrained host platforms, not style.

In terms of style, code should not break multiline content earlier than 100 columns for the sake of consistency.

## Indentation

It is an error to use tabs. Spaces can be used to freely align assignment etc but general indentation is otherwise
be warned about if it's not a multiple of 4.

## Naming

- Types are pascal case, such as `MyType`.
- Generic types are pascal case, not single letters; `<Key, Value>` not `<K, V>`.
  The exception to this would be code so generic that there's no good name, then feel free to use a letter.
- Functions, constant and value bindings are snake case, such as `fun draw(_ plane: some Plane, from origin: Origin)`

Pascal case should follow modern convention of capitalizing acronyms, like `Ascii` not `ASCII`.

This naming convention effectively ensures that types, constants and members practically never collide.
Compilers should warn if these conventions are not followed, this is critical as ambiguity can interfere with
category disambiguation syntax such as `value.Category.function()`

As a rule of thumb however, if a function has a name with multiple words it's probably badly named. The language
uses argument labels which mix the name and arguments more naturally.
Notice that instead of the unnatural, split the naming is visually associated with arguments at point of use:

```
fun position_in_space(_ shape: Shape, _ space: CoordinateSpace) -> Point
fun position(of shape: Shape, in space: CoordinateSpace) -> Point

position_in_space(shape, space)
position(of: shape, in: space)
```

If this usage looks natural then a function is generally designed well, and idiomatic usage of the language is
all about finding excellent naming which remains readable.

In fact it's fine to use more labels

Additionally the language is very permissive when parsing so that unambiguous use of keywords is allowed:

```
fun fun(mut mut: Int) {
    let let = mut
}
```

Obviously this example is rather unreadable but it serves as a clear example of how permissive keywords are.

## Source encoding

Source encoding is specified as ASCII for backwards compatibility or STR-8 as the modern format.
Compilers should not accept any other encoding as source portability is core to the language.

Additionally, by Unix convention, source files must end on a newline.

## Pure, lazy primitives

The Strawberry language is abstract and does not assume any architecture specific data types or memory, and the
default portable form of the language is entirely logical, including a lack of addressing.

Most of primitive data types are instead implemented using calls into the compiler or backend.
The syntax looks like this:

```
#Type
#call()
```

They describe directly invoking a special request to the compiler, and if the compiler does not handle it
the request falls through to the backend. If it falls through completely a compile error is raised, but this
is done lazily; if a compiler invocation is not needed on a compile-time or runtime path it's ignored.

The laziness of intrinsics is especially useful because it avoids the need for conditional compilation for constructs
that use them. Code is free to reason about concepts that can't be lowered as long as they are optimized out
or generally unused, only otherwise is an error actually raised. For example if a construct only has one method
that does not work, the rest of the code still works, and so does the method itself if used in compile time context.

Backends are allowed to implement intrinsics arbitrarily, that includes generating calls to runtime functions which
may require linking additional runtime support; for instance an NES backend may emit calls to arithmetic procedures
as the system itself only supports 8 bit arithmetic at instruction set level.

The intention is to make the interaction between logical constructs and evaluation/lowering obvious, supporting
extending the compiler with more backends easier than with most other toolchains. A lone hobbyist should be
able to port their indie game to an arbitrary system without wasting too much time, or an embedded developer
could get code generation for whatever esoteric hardware they want to support.

This does not mean dead code is not checked. It still is, but it does not need lowering.
With that, Strawberry has conventional ergonomics, yet it remains abstract.

Another such use case could be this:

```
pub type ClassRepr<T> = if #target("jvm") then #JvmClass<T> else Rc<T>

pub base class MyClass: ClassRepr<Self> { ... }
```

changing the class memory management strategy depending on the platform, conditionally applying the intrinsics.

For even better ergonomics the language server should ideally inform the user if the current target can lower
the feature, or offer a list of which backends can/can't do that. For generic code it is important that it does
not accidentally depend on concrete constructs.

## Ownership

Semantically the language provides nonescapeable references as a primitive. They are more limited than
some languages because to be a primitive feature something has to be implementable on exotic architectures, like GPUs.
Pointers don't exist on GPUs or on the JVM for example.

These more constrained references are implementable as simple sugar in fact.

The syntax for owned binding is as follows:
- `Type` is owned and maintains destructive move semantics or trivial copyability.
- `&Type` is borrowed, and the least privileged form of access. The syntax is brief as this is very common.
- `mut &Type` is aliased, any mutation is defined to be replicated in the original value.

Ownership semantics uphold the law of exclusivity;
Aliases can't overlap, there can only be one mutable path to a value at a time.

Again, these are *not* pointers. A borrowed value could still be passed in-register if small and an alias could
actually be rewired into a pure functional update with not even the stack being required.
What actually happens is calling convention defined.

Here's an example:
```
fun foo(bar: &String, mut baz: &String) {
    baz = bar
}

test foo "call" {
    let a: String = "Hello"
    mut b: String = ""

    // Forming a mutable alias requires explicit notation, borrowing does not.
    foo(bar: a, baz: &b)
}

test foo "error" {
    // This is an error because exclusivity is not upheld.
    mut str: String = "Hello"
    foo(bar: str, baz: &str)
}
```

In terms of the type system they are binding modifiers, not standalone types.

Still, this is purely lexically scoped aliasing, and that's not enough to implement loops efficiently.
The language requires a way to tie lifetimes together:

```
category Iterator<Element> {
    fun next(&mut self) -> Element?
}

struct IndexingIterator<'a, Target> where Target: IndexedCollection {
    target: &Target
    mut cursor: Target.Index

    pub init(_ target: Target) {
        self.target = target
        self.cursor = target.
    }
}

extend IndexingIterator: Iterator<Target.Element> {
    pub fun next(mut &self) -> Target.Element? {
        if self.target.contains(cursor) {
            self.target.next_index()
        }
    }
}

extend IndexedCollection {
    fun iterator(&self) -> some Iterator<'self, Element> { ... }
}

pub fun main() {
    mut nums: Array<Int> = [1, 2, 3, 4]

    for num in nums {
        num *= 2
    }
}
```

## Namespaces

Free functions are almost never used and labels exist, which is why the language does not need standalone namespacing,
instead relying on nesting and association. If a module has name collisions within itself generally the naming is bad
or not scoped correctly.

Constructs can be namespaced within other constructs. Instead of `SequenceMap<T>` polluting the global namespace
you can declare `Sequence.Map<T>`. This serves as namespacing without having to nest them, and
notably scopes to the nominal construct itself, not generic instances, so there is only one Map for all sequences.

Nesting with respect for generics is also possible but is instead done in an extension.

## Iterators

There are no iterators (as seen in Rust for example). The approach taken is to fix what Swift attempted, meaning it's:
- More powerful.
- More ergonomic.
- Just as efficient.

I will make the comparison to Rust here. In Rust traits are often treated as interfaces, not typeclasses, even
though they have the capability to do so. Iterators are a semi-concrete concept of iterating things sequentially,
and then there's a lot of utility functions like `map` which wrap iterators and are iterators themselves.

That's kind of confusing to me as it loses valuable semantic information.

I will simplify by only showing consuming examples now. This is the consuming iterator:

```
pub category Iterator<of: Element>  {
    fun next(mut& self) -> Element?
}
```

That's the complete definition. The iterator is an evaluator of the lazy wrapping, not the lazy wrapping itself.

Consider a design like this:

```
// We define the actual typeclass of iterable constructs.
pub category Sequence<of: Element, Iterator> {
    fun iterator(self) -> Iterator
}

// We can add non-overrideable sequence adapters now, here and not on iterators.
extend Sequence {
    pub fun map<Transform, R>(self, _ transform: Transform) -> Map<Self, R, Transform>
    where
        Transform: (Element) -> R
    {
        .init(inner: self, transform: transform)
    }
}

// The owning map itself would be something like this.
// We can namespace it to the category to avoid polluting the global namespace.
pub struct Sequence.Map<T, Transform, R>
where
    T: Sequence,
    Transform: (T.Element) -> R
{
    inner: T
    transform: Transform
}

// And it's of course a sequence too, now we can compose infinitely, statically. You can map on a map and so on.
// With generators there isn't even a need to implement the iterator manually.
extend Map: Sequence where T: Sequence {
    pub fun iterator(self) -> Iterator {
        gen {
            mut i = self.inner.iterator()
            while let .Some(next) = i.next() do yield self.transform(next)
        }
    }
}

// But it gets better because what's stopping a map from working on collections too?
// This way you get a wrapping where indexing into it is also mapped, making our mapping random access.
// I feel like this is just a more powerful way to express container operations in a generic way.
extend Map: IndexedCollection where T: IndexedCollection {
    ...
}
```

This is inspired by Swift, just done efficiently instead of eagerly returning [T] like it does.

You could imagine that any given operation could specialize to provide as much functionality as it can
based on its requirements and the capabilities of the underlying type.

With references as a standalone type this would already encapsulate mutation and borrowing, but that is not yet decided.
It might be worth it to lean a little bit into C++ and allow specifying reference types through generics.

## Modules

Strawberry does not depend on a filesystem structure or any complicated system, but it is advised to match the two.
Every source unit starts with a module declaration. Everything in the source unit will effectively be treated
as part of that module, so on conventional desktop platforms the language can operate on a simple search-path basis for
all the places source files are to be pulled from.

This may seem less structured than a lot of languages, but Strawberry is fundamentally opposed to dependency graph hell.

```
module core.draw
```

## Functions

```
// A simple function looks like this.
fun foo() -> () { ... }

// The empty tuple is implicit and the compiler should warn on such use. Instead, the syntax should look like this.
fun foo() { ... }

// Multiple return values are supported through tuples, ideally with named fields.
fun position() -> (x: Int, y: Int) { ... }
```

## Stackless coroutines

### Generators

There exists syntax sugar for generators, avoiding the need to implement them manually. Instead, the code
can be implemented as a coroutine and the compiler can rewire it into a struct.

```
fun asc(mut n: Int) -> Generator {
    gen {
        let next = n
        n += 1
        loop yield next
    }
}

pub fun main() {
    let g = asc(0)

    for n in g where n < 20 {
        print(n)
    }
}
```

### Async

Similarly to generators, composable async sugar is also defined, rewiring code into a struct.

```
async fun load_bytes(from url: &String) -> Buffer<Byte> {
    get_file_from_network(url).await
}
```

## Contracts

The language uses `where` syntax to specify constraints on compile time values. The same syntax specifies contracts
when applied to runtime known bindings.

Contracts are signature level assertions. They catch bugs at compile time and in safe compilation modes insert
checks into the code to abort execution. In unsafe compilation modes they are not enforced at runtime so violating
them is undefined behavior.

```
// If the caller gets the range wrong at compile time they will get a compile error,
// and if it only happens at runtime the code will panic.
fun random(from lower_bound: Int, to upper_bound: Int) where lower_bound < upper_bound { ... }
```

## Categories

Categories are a semantic contract, a class of behavior, a way to write generic code in terms of semantics
with no knowledge of the underlying concrete types.

First of all this is a new name so here's all the existing terms and why I had to avoid them:
- Interface, which express only a contract of functionality, not semantics. This is consistent with how this
  term is defined and used in programming languages.
- Trait, which is ambiguous in its definition. Rust and C++ use it but make it more ambiguous still by applying it
  to both inconsistently; Rust's Add for example is an interface but Ord a category with laws.
  The term is useless as it would imply a connection to a dangerously similar but fundamentally incompatible idea.
- Protocol, this is what Swift decided to name these. This conflicts with the existing definition of protocols in
  computing for no reason and I never found it intuitive either.
- Class, which is used by Haskell and makes perfect sense by its definition, but Simula descendant languages have
  appropriated the word to mean a vague grouping of data and functionality, which is useless and doesn't even
  preserve the actual object oriented meaning of classes.

Category is a very similar and slightly more concrete term compared to class, and has no widely standardized meaning
in programming. In math, category has a meaning far more constrained than the dictionary definition, but I don't care.

> TLDR: Everything else is either not a match by definition or alredy used in ways I wish to distance this feature from.

The actual semantics are very involved but they allow for very powerful generic programming making categories
the most important feature of the language. Categories neatly remove most code duplication by
modelling the very semantics of the *categories* types belong to and deriving most functionality.
This solves the same problem inheritance promises to, but with far more flexibility.

```
pub category PartiallyOrdered<with Other = Self>: PartiallyEquatable<to: Other> {
    fun ordering(&self, with other: &Other) -> Ordering?

    infix operator < fun less(&self, than other: &Other) -> Boolean {
        if let .Some(ord) = self.ordering(with: other) and ord == .Less then true else false
    }

    ...
}

// Categories can inherit from each other and override implementations, describing a useful subtyping relationhip.
pub category Ordered<with Other = Self>: Equatable<to: Other>, PartiallyOrdered<with: Other> {
    // When the signature is overloaded on the return type normal inference rules apply, however additional
    // priority is given to categories lower in the hierarchy.
    fun ordering(&self, with other: &Other) -> Ordering

    // When the signature is an exact match the subtype overrides it completely.
    infix operator < fun less(&self, than other: &Other) -> Boolean {
        let ord = self.ordering(with: other); ord == .Less
    }

    ...
}

// Default implementations can be provided in extensions, not just inline.
// In this case it's useful to derive the partial ordering from total ordering.
// The overload resolution rules will look for the most refined implementation, so the lower
// in the hierarchy the higher the priority. A concrete type is the lowest it can go so it will always be prioritized.
extend Ordered {
    pub fun ordering(&self, with other: &Other) -> Ordering? {
        self.ordering(with: other)
    }
}

// Categories are the primary way to work with generics. Without constraints you can't do much with erased types.
fun foo<T>(lhs: T, rhs: T) where T: Ordered -> Ordering {
    lhs.ordering(with: rhs)
}

// A lot of the time it's possible to use the sugar erasure form instead of explicitly writing the generics.
fun foo(bar: Debug) {
    print(debug: bar)
}

// Another option is using the unsized dynamically dispatched form which is rarely used but useful for
// more dynamic polymorphism in special cases. For the most part though dynamism is achieved through classes.
fun foo(bar: &dyn Debug) {
    print(debug: bar)
}
```

### Category qualification

```
// Types can implement the same category multiple times provided that the parameters are different.
// This has the potential to create collisions, so it's possible to refer to a specific category by name.
// Not a common situation as usually generic code will only be aware of the categories it requires,
// but for completeness it is something that can be disambiguated in two ways.

// Consider these two categories which both provide a generic element type.
category Sequence<of: Element>
category MutableSequence<of: Element>

// We can neatly be generic over such a category, no collision here.
fun print_all_values(of sequence: &Sequence) where Sequence.Element: Format {
    for element of sequence do print(element)
}

// But what about this edge case?
fun print_and_double_all(of sequence: Sequence and MutableSequence)

```

## Structures

Unlike tuples which are simple structural records, structures are more complex nominal records. They tend to describe
more than the data itself, carrying additional semantics and generics as well as an entire namespace of their own.

```
// Fundamentally structs can just be used as a nominal alternative to tuples.
// Another core difference is how their data members behave, as by default they are immutable. An immutable binding
// of a struct is purely immutable, but unlike a tuple the struct can make members immutable no matter the context.
// Everything inside a struct is also inherently private.
struct Rectangle {
    width: Int
    height: Int
}

// There exist visibility and mutability modifiers.
struct Rectangle {
    pub mut width: Int
    pub mut height: Int

    // Unlike tuples, structs can have methods. For structs the self argument is always explicit.
    pub fun area(self) -> Int {
        self.width * self.height
    }
}

```

## Tuples

Data can be bundled together structurally, without the need for a named type. One of the features this enables is
multiple return values, especially because tuple elements can (and generally should) be named.

```
extend Player {
    fun position(&self) -> (x: Fixed, y: Fixed) { ... }
}
```

They can be destructured in patterns:

```
let (x, y) = player.position()

match player.position() {
    (1, y) -> ...
    (x, y) -> ...
}
```

## Enumerations

Enumerations describe a fixed set of values and have most of the features of structs except they can
always be initialized with their cases, not necessarily an initializer (but those still exist).

```
// In their simplest form enums are a set of possible values.
enum Fruit {
    Strawberry, Apple, Pineapple, Mango
}

// Enums can choose to use a concrete tag which can be of any const Equatable type.
// This requires every case to be assigned a constant value, unless the type is const Strideable; in that case
// any omitted assignment will receive the preceding tag advanced by 1.
// To allow the initial tag to be implicit as well the type must implement const Default.
// Having any duplicate tags (two or more comparing equal to each other) is an error.
enum ExitCode: Int {
    Error = -1
    Success = 0
}

// Enums can be open, meaning non-exhaustive. All code handling such an enum must account for the possibility
// of encountering an unspecified tag. This is useful for backward compatibility.
open enum ExitCode: Int {
    Error = -1
    Success = 0
}

// Enums can have associated structured data, internally a tuple of (Tag, Data).
// Most of the time the tag will be smaller than the associated data, so it actually makes more sense to put
// the tag at the end so that the padding is in the tail an can be packed more tightly when nested in other types.
// Such inverse layout can be selected with an attribute.
enum Optional<Inner> {
    None
    Some(Inner)
}

// Much like normal tuples this data can be accessed through pattern matching, tuple indexing or name.
// Naming tuple elements works as usual.
enum Color {
    Red, Green, Blue
    Custom(r: UInt<8>, g: UInt<8>, b: UInt<8>, a: UInt<8>)

    // Enums can of course have methods.
    pub fun red(self) -> UInt<8> {
        match self {
            .Custom(r, g, b, a) -> r
            .Red -> 255
            _ -> 0
        }
    }
}

```

Enum specific attributes:
- `inverse`: Swaps the tag and data around.

## Extensions

Practically any nominal type can be extended, even structural types like tuples can be.
Extensions simply implement additional methods or traits for existing types. Traits themselves can be extended
which implements functionality for all implementations of that trait.
In fact, even the abstract Any type can be extended, implementing functionality for all types in existence, though
this should be used very rarely and with great caution.

```
struct Array<Element> { ... }

// Parameters are accessible implicitly when extending types and can be used for constraints.
extend Array: Equatable where Element: Equatable { ... }
```

## Pattern matching

## Exceptions

First of all, exceptions are exactly what the name suggests. Exceptional.
The language uses types like Optional and Result for error handling, and abusing this feature for it could have
impact on binary size and compile times.

Exceptions are not using stack unwinding, they are instead a templated feature, a no-op if not explicitly
handled or forwarded. The entire chain only materializes if anyone actually handles
the error case, so it's generic sugar for rewriting code to return an enum, allowing code which simply does not
need to care about issues like allocation failure to sweep it under the rug.

For example a game would catch on its allocations, but a compiler is not a destructive process so not handling them so
is not going to cause serious issues, and it's unlikely there's anything it could do anyway. The same library code
should be efficient for both cases without having to write anything twice.

```
fun get_entity() throws AllocationException -> Ptr<Entity> {
    let entity: Ptr<Entity> = allocate_entity()
    if entity != null then entity else throw AllocationException()
}

fun forwards_exception() throws AllocationException -> Ptr<Entity> {
    try get_entity() // Forwarded allowing the caller to decide how to process the error.
}

fun ignores_exception() -> Ptr<Entity> {
    get_entity() // Not handled in any way, `throw` will instead invoke the global panic handler.
}

fun caller() {
    let a = forwards_exception() catch {
        AllocationException e -> return print("allocation failed") // We must early return here or provide a default.
    }
    let b = ignores_exception()
}
```

The language will instantiate the throwing function with or without the exception handling. This has a minor effect
on binary size compared to stack unwinding tables but it depends on manual exception forwarding; that being said
I believe that explicit, checked exceptions are great because they fully specify what the program does in the signature.

The desugared form could be visualised to something like this, but the names
would of course require more involved mangling.

```
fun get_entity() -> Ptr<Entity> {
    let entity: Ptr<Entity> = allocate_entity()
    if entity != null then entity else panic(AllocationException())
}

enum GetEntityResult {
    Success(Ptr<Entity>)
    AllocationException(AllocationException) // One of these cases will exist for every forwarded/handled exception.
}

fun get_entity_except() -> GetEntityResult {
    let entity: Ptr<Entity> = allocate_entity()
    if entity != null then .Success(entity) else .AllocationException(AllocationException())
}

enum ForwardsExceptionResult {
    Success(Ptr<Entity>)
    AllocationException(AllocationException)
}

fun forwards_exception_except() -> ForwardsExceptionResult {
    // Try requires us to be forwarding at least all the same exceptions as the expression.
    // This is an implicit conversion and usually should just be a no-op if the layout matches, otherwise
    // the tag has to be adjusted which is trivial.
    get_entity_except()
}

fun ignores_exception() -> Ptr<Entity> {
    get_entity()
}

fun caller() {
    let a = match forwards_exception() {
        .Success(v) -> v
        .AllocationException(e) -> return print("allocation failed")
    }
    let b = ignores_exception()
}
```

Notably, if an exception is never chained into a catch expression, the signature is as if the exception was
not there at all. Unless an exception is explicitly handled there is no runtime overhead at all, which is nice.

## Tests

Tests can be associated with specific declarations.

## Classes

Strawberry does not use classes in its core libraries, and in fact, only frameworks should touch them. They are sugar
for flat inheritance from a base class used in terms of a smart pointer as the allocation strategy. They are meant
for special use cases like plugins, perhaps loading dynamic game entities from shared libraries at runtime to
facilitate live reloading for faster iteration.

In this sense, Strawberry enables the best features of objects without orienting the language around them.
Most code isn't going to use classes but the power is there to make it easy for tools to take advantage of dynamism.

All classes require a base class, which must be specified like this:

```
// The pattern is a newtype of a container generic over `Self`.
// The base class itself can also be generic and can be extended like any other type.
pub base class Entity: Rc<Self> {
    // Data members use normal visibility semantics but gain a protected modifier to only expose
    // declarations to subclasses.
    protected variable: Int

    // Initializers can be provided and one must be invoked by derived classes.
    pub init(_ v: Int = 0) {

    }

    // Dynamically dispatched members can be declared abstract or with a default implementation.
    // This is simply a function pointer following normal member layout within the base.
    // Methods do not need the explicit `mut &self` argument, and self is in fact implicit for classes.
    // Classes are sugar so the sugar goes all the way! :)
    pub dyn fun method()
}
```

What is `Self` to a base class when it's abstract itself? This syntax actually makes sense. The base *is* the container,
an unsized type which requires allocation to work. It's internally a tuple of members `(Base, Derived)`.

Subclasses can then be declared like this:

```
pub class Player: Entity {
    // Classes must implement all abstract members from the base class.
    dyn fun method() { ... }
}
```

## Foreign objects

Strawberry can sugar objects from other languages, providing interoperability with Objective-C, C++, Swift, Java etc.
This is yet to be designed.

## Text encoding

Strawberry has built in support for three encodings.
- ASCII, a legacy English-only format with very few glyphs and obsolete control codes.
- Unicode, specifically UTF-8, technically the modern format but obsolete by design, true to the expression
  jack of all trades and master of none. A terrible format with duplicate encodings of equivalent result making
  processing this format highly inefficient for the sake of supporting ancient script unsuitable for the digital era.
- Strawberry, STR-8, in the name of simplicity, a new encoding throwing away bad ideas documented further on.

# Draft Overview of the Strawberry Core Library 🍓

Generic programming is cool sure, but eventually something concrete has to fill in the generic parameters.
To get started wiring generic code up the core library provides a lot of concrete primitive implementations.

The modules are:
- `core`, the fundamental generic constructs and linear memory primitives used by most code.
- `core.c` C primitives for the target.
- `core.draw`, an extensible, lazy and composable abstraction for bitmap processing.
- `core.nes`, NES specific primitives.

Useful core primitives include:
- `Bool`
- `Int`
- `UInt`
- `Fixed`
- `Float`
- `Ptr<T>`
- `Box<T>`
- `Rc<T>`
- `Char`
- `String`
- `AsciiChar`
- `AsciiString`
- `Utf8String`
- `Array<N, T>`
- `Span<T>`
- `Buffer<T>`
- `RingBuffer<T>`
- `InlineBuffer<N, T>`
- `Map<K, V>`
- `Set<T>`
- `Optional<T>`
- `Result<T, E>`

Their availability depends on the target.

# Draft Specification of the Strawberry Text Encoding 🍓

## STR-8

STR-8 stands for the Strawberry 8 Bit Encoding and also phonetically resembles the word "straight" which
is a good fit for an encoding designed to be exceptionally straightforward while remaining powerful.

Strawberry is not a replacement to Unicode but a more well-constrained, efficient plain text format which
can *embed* unicode or in fact be customized with many other domain specific standards on top. In a sense
it's actually a container format.

It does not need backwards compatibility with ASCII or Unicode because it's trivial to just perform a conversion.
This is not the 1960s where that would be expensive, compatibility is a purely protocol issue not an encoding one.
For example specify the different format in the HTTP header, it's not that hard, and the encoding is very simple.

There are no emojis, instead there is an escape span, effectively allowing domain protocols to define
lightweight syntax for such features, including emoji, but without any limits.

The encoding will make an effort to place lower case letters and their upper case equivalents in an alternating fashion,
so that checking ranges within the alphabet can be performed by a simple range check and testing if the character
is even or odd. Changing the case is an increment or decrement, which may be less efficient on 50 year old hardware
compared to ASCII but it's an obvious choice now.

Alphabet encoding is by convention designed like this:
- The layout places lower case characters at even values and upper case at odd values.
- Checking without case sensitivity is just a range check `('a'...'Z').contains(x)`.
- Checking in lower case range adds a check for being even `x.is_even()`.
- Checking in upper case range adds a check for being odd `x.is_odd()`.
- Changing the case is an increment/decrement.

This is friendly to humans and reasonable for machines. A simple format to work with, making bugs far less likely.

## Encoding system

The encoding is fundamentally 8 bit, but it can include multibyte spans. The encoding takes advantage of
the generic metaprogramming ability of the Strawberry language, which can parameterize strings on our knowledge
about the contents, enabling for instance indexing with integers once it's certain that the string contains no spans.

There are only 6 special values:

- 00 :: null, a reserved value which is not valid and can be used as a sentinel.
- 01 :: newline, indicates the end of a line.
- 02 :: space, indicates a space, self explanatory.
- 03 :: tab, indicates a tab, self explanatory.
- 04 :: span start or end, indicates the span of a domain specific data sequence.
- 05 :: escape, reserved for future use, for example a multibyte superset of the encoding.

## Domain specific escapes

To support domain specific functionality such as emoji or legacy script, STR-8 can embed multibyte spans.
For the purposes of this document the backtick will be used to signify the escape character in examples.

This is highly efficient because STR-8 is an 8 bit format whose purity can be validated easily. It's simple, if there
are no escape characters then the string is trivial and can be indexed directly among other statically dispatched
optimizations. Legacy edge cases should not dictate performance.

Why include this at all? Couldn't text formats do this already? Well, this is still useful for embedding
other more complex formats in a structured way string types can rely on. No one is forced to use this feature and
it may not turn out to be very useful. I'll decide if I should keep this feature or not over time.

### Example 1, emoji

Rather than bloating the text format itself with domain specific or legacy features, such things should
have standards of their own, constrained to their own domain. Consider this example emoji markup:

```
Hello, world! `emoji.smile`
```

This could be handled transparently by the UI, with no functional difference from how text works today.

A more memory efficient approach would be to simply put unicode sequences in escapes, but that sacrifices the
plain text syntax which is very useful. This is outside the scope of a text format itself however.

### Example 2, terminal

A terminal protocol could define a control protocol and use spans to encode the control sequences.

```
Hello, `myterm.rgb(1.0, 0.5, 0.5)` world! `myterm.bell`
```

This makes far more sense than offering domain specific features like bell invocation as a dedicated code point
the way Unicode does. Instead protocols get to use their own namespaces.

## Encoding table

```
00 :: null
01 :: newline
02 :: space
03 :: tab
04 :: span
05 :: escape
06 :: 0
07 :: 1
08 :: 2
09 :: 3
0A :: 4
0B :: 5
0C :: 6
0D :: 7
0E :: 8
0F :: 9
10 :: a
11 :: A
12 :: b
13 :: B
14 :: c
15 :: C
16 :: d
17 :: D
18 :: e
19 :: E
1A :: f
1B :: F
1C :: g
1D :: G
1E :: h
1F :: H
20 :: i
21 :: I
22 :: j
23 :: J
24 :: k
25 :: K
26 :: l
27 :: L
28 :: m
29 :: M
2A :: n
2B :: N
2C :: o
2D :: O
2E :: p
2F :: P
30 :: q
31 :: Q
32 :: r
33 :: R
34 :: s
35 :: S
36 :: t
37 :: T
38 :: u
39 :: U
3A :: v
3B :: V
3C :: w
3D :: W
3E :: x
3F :: X
40 :: y
41 :: Y
42 :: z
43 :: Z
44 :: .
45 :: ,
46 :: :
47 :: ;
48 :: '
49 :: "
4A :: !
4B :: ?
4C :: (
4D :: )
4E :: [
4F :: ]
50 :: {
51 :: }
52 :: +
53 :: -
54 :: *
55 :: /
56 :: \
57 :: `
58 :: _
59 :: #
5A :: %
5B :: =
5C :: <
5D :: >
5E :: |
5F :: &
60 :: ^
61 :: ~
62 :: @
63 :: $
64...7F :: base reserved
80...FF :: superset reserved
```
