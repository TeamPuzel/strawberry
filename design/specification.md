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
// Comments are capitalized and punctuated normally.
/// Documentation comments are markdown-like and must precede a declaration.
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
- Constants are upper case, such as 'MY_CONSTANT`.
- Functions and value bindings are snake case, such as `fun draw(_ plane: some Plane, from origin: Origin)`

Pascal case should follow modern convention of capitalizing acronyms, like `Ascii` not `ASCII`.

This naming convention effectively ensures that types, constants and members practically never collide.
Compilers should warn if these conventions are not followed.

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

Additionally the language is very permissive when parsing so that unambiguous use of keywords is allowed:

```
fun fun(mut mut: Int) {
    let let = mut
}
```

Obviously this example is rather unreadable but it serves as a clear example of how permissive keywords are.

## Source encoding

Source encoding is specified as ASCII or UTF-8 with Unix style encoding.
Compilers should not accept any other encoding as source portability is core to the language.

Additionally, by Unix convention, source files must end on a newline.

## Pure primitives

The Strawberry language is abstract and does not assume any architecture specific data types or memory.
The type system itself needs to be able to reason about numbers and logic however. For this reason there do exist
some pure compile time primitives.

```
// An arbitrarily precise signed whole number. It is an efficient compile time arithmetic value.
type Integer

// An arbitrarily precise binary coded decimal number. It is not as efficient but can express anything.
type Decimal

// A arbitrary boolean type representing either true or false.
type Boolean

// A source encoded string literal.
type StaticString

// A type with no values which therefore can't be instantiated. It usually marks something as unreachable
// and participates in exhaustiveness checking.
type Never
```

None of these types can be lowered into runtime representation. More concrete primitives should instead implement
checked implicit conversion from pure primitives.

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
trait Iterator<Element> {
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
    pub fun next(&mut self) -> Target.Element? {
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
or not just scoped correctly.

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

TLDR: Everything else is either not a match by definition or alredy used in ways I wish to distance this feature from.

## Structures

Unlike tuples which are simple structural records, structures are more complex nominal records. They tend to describe
more than the data itself, carrying additional semantics and generics as well as an entire namespace of their own.

```
// Fundamentally structs can just be used as a nominal alternative to tuples.
// Another core difference is how their data members behave, as by default they are immutable. An immutable binding
// of a struct is purely immutable, but unlike a tuple the struct can make members immutable no matter the context.
// Everything inside a struct is also inherently private
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
on binary size compared to stack unwinding tables but it depends on manual exception forwarding. Still, the expectation
is that most code will not abuse this for normal errors and will

The desugared form could be simplified to something like this to explain the mechanism, though the names
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
        .AllocationException(e) -> ret print("allocation failed")
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

There is no further inheritance, classes are either an abstract base or a final derived.

## Foreign objects

Strawberry can sugar objects from other languages, providing interoperability with Objective-C, C++, Swift, Java etc.
This is yet to be designed.

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
