
```str
// The issue of `mut` bindings is not just that they're very rarely needed as Swift
// demonstrated by removing the `var` binding without issue, but because
// they are a grammatical nightmare with argument labels for both humans and
// parsers. One could imagine putting the `mut` after the colon but that is
// very unintuitive, a mismatch with the rest of the language.
//  It is not viable, and not needed, since one can just shadow bind it
// to the same name, moving the value (optimized away so no issue here).
// Functions can still specify projections, but they always have explicit type
// signatures so the syntax is always the same and part of the declaration not
// the type expression. In fact, projections should be dropped from expression
// grammar and only affect bindings, we do not want C++ value category hell
// or Rust pointer like fully manual references. Much like Swift the best approach
// is probably reference wrappers that just store projected bindings inside
// when escaping is required.
fun foo(of bar: ByValue, of baz: &ByRef, of foo: mut &ByMut)

// Destructuring tuples always has either of the following, the syntax only has one
// let but it makes sense, it is not a pattern match so it actively shouldn't
// look like one! There is no let-else, that's what the dedicated `guard` is for
// avoiding grammar ambiguity.
let (a, mut b, &c, mut &d) = foo()
let (a: Int, mut b: Int, &c: Int, mut &d: Int) = foo() // they can also mix and match types and inference which is not shown

// normally it's just this too.
let a
let mut a
let &a
let mut &a

// Patterns are quite self explanatory as we just discussed them, they use `let` bindings
// for individual names, not the entire pattern. This keeps the grammar context free,
// something Rust's syntax actually fails at completely despite requiring less keywords
// for the initial declaration. That's not an issue for Strawberry because keywords are
// not forbidden as identifiers unless genuinely potentially ambiguous.
if .Some(let value) = optional { }
guard .Some(let value) = optional else { <diverge> }

// Closures just follow the same logic as destructuring, for both arguments
// and captures.
|a, mut b, &c, mut &d| [ca,
// Notably they do allow `mut` by owned value unlike functions. That is because
// they are actually objects after all, and also support a generator transformation,
// something used to implement basically all iterators.
// Semicolons chain expressions on one line eagerly in clean block-less variants,
// this is a multi expression loop.  The syntax is extremely expressive, and mutable
// bindings are very valuable to make it possible.
let iota = |mut i: Int| loop do let result = i; i += 1; yield i

// The monadic loop also uses the destructuring rules.
// in this case iota is not even returning a monad so the
// loop is infinite and could be used with a `break result` to assign
// to a variable from the loop expression.
for i in iota do print(i) // prints indefinitely (well, until the overflow panics)

// Types can project members too but it requires lifetime binding to self in the initializer,
// which in turn requires tracking lifetime of these wrappers.
struct FooProjectionWrapper {
    let &foo: Foo

    init(of foo: &Foo) 'foo {
        self.foo = foo
    }

    // Methods use syntax similar to structured binding on self, but they
    // still disallow `mut` on its own for consistency. It is very stupid for the caller
    // to see owned `mut` in the signature like in Rust as it only has meaning internally!
    // similarly, destructuring is not allowed for function arguments but it is for
    // closure arguments.
    fun bar(&self) { ... }

    // there is no way to customize the type of self to a wrapper like in Rust,
    // to make a method for `Optional<of: Int>` you just `extend Optional where Inner == Int`
}
```

```str
|a, mut b, &c, mut &d, e: Int, mut f: Int, &g: Int, mut &h: Int| [ca, mut cb, &cc, mut &cd, ce.clone()] async throws A, B, C -> (x: Int, y: Int) {}
```
Strawberry doesn't do multiline type inference like in Rust, so unless we know the expected result type for the expression the closure needs explicit types for its arguments. The return type can still be inferred from the result no matter what however.

Closures don't currently plan to support implicit captures in Strawberry.
It is unclear what the semantics of those would have to be. For example Swift's `$member` prefix operator syntax used in SwiftUI would likely be a bit verbose to declare in the capture list instead of directly inline. Immutable owned binding seems most reasonable for a default.

Trailing closures also work and allow dropping the pipe token if using a block expression, because a normal block expression can't ever immediately follow another expression on the same line without a semicolon.
```str
foo() || expr
foo() || { expr }
foo() { expr }
foo || expr
foo || { expr }
foo { expr }
```

module core.memory

/// A pointer in Strawberry is a raw, escapeable reference to one or multiple values. It is not
/// necessarily implemented as a traditional C style integer representing a linear memory address,
/// because Strawberry does not depend on linear memory for its specification.
///
/// Linear memory operations are possible on pointers, but such use is not guaranteed to be available
/// on certain platforms so it should be avoided in favor of intrinsic messages instead. These extensions
/// are intended for bridging code such as interacting with C or other foreign code where the program
/// is already inherently tied to a linear memory model.
pub struct Pointer<to: Pointee>: not Sendable {
    @Hidden
    pub let raw: #Pointer(Pointee)

    @Hidden
    pub init(raw: #Pointer(Pointee)) {
        self.raw = raw
    }

    pub unsafe get pointee(&self) -> Pointee 'self {
        #pointee(self.raw)
    }

    pub unsafe set pointee(&self, _ value: Pointee) {
        #pointee(self.raw) = value
    }

    pub unsafe mut(&self, _ mutation: (value: mut &Pointee) -> ()) {
        mutation(&#pointee(self.raw))
    }

    pub unsafe subscript get(&self, _ index: USize) -> Pointee 'self {
        #pointee_at(self.raw, index)
    }

    pub unsafe subscript set(&self, _ index: USize, _ value: Pointee) {
        #pointee_at(self.raw, index, value)
    }

    pub unsafe subscript mut(&self, _ index: USize, _ mutation: (value: mut &Pointee) -> ()) {
        mutation(&#pointee_at(self.raw, index))
    }

    pub const let null = Self(raw: #null_pointer())
    pub const let dangling = Self(raw: #dangling_pointer())
}

The intended semantics currently are like this:
```str
category Foo<A> // No default
category Bar<A = Int> // Default

fun foo(_ foo: Foo) // Allows any foo
fun foo(_ foo: Foo<Int>) // Allows only foo of Int
fun foo(_ foo: Foo<_>) // Allows any foo

fun bar(_ bar: Bar) // Allows only bar of Int
fun bar(_ bar: Bar<Int>) // Allows only bar of Int
fun bar(_ bar: Bar<_>) // Allows any bar
```

Labels make it easy to selectively skip these in many cases
`Foo<a: A = Int, b: B = Int>` where `Foo<b: _>` would relax the default constraint of B while keeping A.
Labels are cute for simple cases already, but actively required to disambiguate packs
`Foo<a: ...A = Int, b: ...B = Int>` where `Foo<b: (_, ...)>`

Dependency works like this:
```str
category Baz<A>
fun baz(_ baz: Baz) {
    print(Baz.A::name)
}
```

I have not considered associated types before because as far as I can tell this is more powerful?

Strawberry will use strict expression-bound inference. It will use the expected expression type to refine constraints as it recurses through the expression, and fail if inference is ambiguous.

So:
```str
fun foo() -> A // 1
fun foo() -> B // 2

let x: A = foo() // 1
let y: B = foo() // 2
let z = foo() as A // 1
let w = foo() // error
```
Strawberry will not attempt inference across multiple expressions. While that is convenient for Rust as it has for instance non generic integer primitives. It needs to defer `let x = 1` because it wouldn't want to just infer i32 and break later if we use it as an i64. It still results in a contradiction however if we use it as multiple types later.

Strawberry handles that better. A literal number forms the `const Integer` expressions of which eagerly decay `pub const decay type = if self.negative then Int<self.bits.max(Int.size)> else UInt<self.bits.max(UInt.size)>`
Because the decay selects a reasonably most constrained int (without going overboard and picking a 1 bit integer only to instantly throw overflow), with runtime implicit conversions between safe int sizes, and checked const implicit conversions across the board since they are caught at compile time, the ergonomics are as reasonably permissive as possible without breaking semantics and needing complex type inference.

In a sense the library programmer is authoring a very constrained domain specific set of inference rules by wrangling const semantics.

Strawberry inference would work like this:
```str
// we have no expected type at all so inference has nothing to go on,
// therefore the binding expression must evaluate unambiguously.
// We are accessing a known, knowably typed variable my_array, which
// means that we know what the element type is and so `map` constrains the expected
// signature for the closure. We now need to evaluate the closure expression,
// we know enough about `x` to determine what it is and know how to resolve`*` etc.
let mapped = my_array.map |x| { let y = x * 2; String(format: y) }
```

If the closure itself was however assigned to a variable first it would only be able to infer the return type and require an argument constraint:

```str
// If the expected type for `mapping` is `Any`, to the evaluator `x` is also not constrained enough
// to know if x even supports multiplication, so we need to add some categories.
let mapping = |x: AdditiveArithmetic and Formattable| { let y = x * 2; String(format: y) }
let mapped = my_array.map(mapping)
```

The reason Strawberry can just assign unspecialized things nonsensically is because of the eager evaluation. `mapping` is a constant expression, so we don't actually need to care, it's basically like we're assigning a constant of a normal generic function to a binding (though closures are technically structs with a call operator, so it is the call operator that's generic not the closure itself). The language does not care, it's purely because the evaluator will residualize an expression that calls something eventually, and the backend will have to deal with that. For closures it would be satisfactory to just make them all unique, but there is nothing stopping a backend from merging identical instantiations in its product. It would only be an error if we actually tried to leak observation of this unspecialized object to a runtime residual, since most backends are unlikely to know how to lower unspecialized generics (but it isn't impossible, Swift does support a fallback generics abi with witness tables and more recently a bytecode)

`(Int) -> String` itself is a function category, not a function pointer.

Strawberry at the moment has a completely unambiguous context free grammar. What comes out of the parser is deterministic and no transformations ever occur on the Ast. expanding the ast would at face value make the evaluator/backend simpler, but then you realize that perhaps unrolling massive variadic structures (like from a SwiftUI style UI framework) would be pretty bad for binary size. The language must actually perform cost analysis for the target to perhaps determine a better lowering, like vectorization or  some special compression in Osize mode. Furthermore some platforms like the JVM don't even have tuples, so they would lower to an array of objects, which the jvm backend can actually roll into a loop because Java is more dynamic, saving space.
Had the Ast just been expanded, the language would for example have to do vectorization by heuristically matching patterns broadly which has horrible complexity compared to restricting it to loops and expansions wich is generally O(n) per optimization pattern pass because it lost the important structure the original Ast already provided and now has to guess.

Now that I think about it the language doesn't need distinct vector types in the core library, it would just extend tuples with the functionality across the board. I could even do a generic tuple swizzle pattern grammar `(x: 1, y: 1, z: 1, 1).(3, z, y, x)` which wouldn't work when trying to use nominal types. And the guard rail of checked labels would guard against mistakes like passing `(x:, y:)` as a `(w:, h:)` without an explicit cast or swizzle.

[hints]

Those hint annotations are purely an interoperability feature to request the backend to please make a best effort at preserving some concrete rule at ABI boundaries because it might be observed through (unsafe) FFI. Strawberry by virtue of being highly abstract does not have a default calling convention, so a backend can opt for whatever makes sense semantically, even differing conventions based on the types involved in an instantiation, or not having conventions at all, maybe we are compiling for a gpu.

A for a shader written in Strawberry it's pretty important that the compiler doesn't have irrelevant overconstraining semantics that prevent taking advantage of the hardware. This is similarly the case for something like the highly esoteric Emotion Engine which should have the semantic freedom to optimize EeFloat tuples

as expressions are evaluated a context is remembered "what exceptions are we catching".
The function throw list is a filter, anything not matched by the list is removed from the context when we chase and resolve the function.
If we witness a throw that throws a type in our context we forward it up (conceptually by destructive move), likely lowered as a tagged union monomorphising the call stack, but could just be an actual jvm throw, though it would need to still handle destructors manually.
If we witness a throw not in the context it just compiles to an immediate overload resolved call to `core.panic(exception:)`

```str
module core

pub fun panic(because reason: String) -> Never {
    #panic(reason)
}

pub fun panic<with: Exception>(exception: Exception) -> Never {
    panic(because: "uncaught exception: " + Exception::path)
}

pub fun todo() -> Never {
    panic(because: "todo")
}

```
One can imagine an additional overloads eventually for thrown values that conform to some kind of error category hierarchy, allowing them to provide extra diagnostics than just the qualified name, and the generic panic itself could use reflection to naively output the value (but that would bloat binary size)

A higher order function can retrieve and expand packs with the meta member access `(T::throws, ...)`

The fundamental reason why this entire system is defined like this is to balance checked exceptions without overhead but allow the programmer to just ignore them like for allocation or number overflow. Adding exceptions (but not removing or reordering) is also ABI safe, because the callers keep calling the old monomorphization, though this again can bloat shared libraries if applied to exported abi (which while not defined will usually be stable for a given compiler build on normal backends). Result types are for expected errors like network io, exceptions for handling behavior that diverges from expectation (like integer overflow, allocation failure)

This is also great because uncaught exceptions (panic invocation) at compile time are just compile errors.

The system is as out of the way as it can possibly be while being statically verifiable

The language can mathematically guarantee safe, optimized tail recursion, and non tail recursion will be a compile error unless the expression adds an explicit acknowledgement of possible unportability (I don't see gpus being able to handle recursion).

```str
fun foo() {
    recurse foo()
    bar()
}
```

This is similar to how non tail returns need the explicit `return` keyword.

I have also resolved the ambiguity of function object syntax. Normally callable signatures are a generic category constraint, but by allowing constraining concrete signature of only closures with specific capture types (or an empty capture list, effectively a thin function item) it unifies the grammar compared to Rust and the language can be runtime polymorphic on the signature (such as the cross-abi callback in the refined example)

```str
module main

import core.concurrency
import core.memory
import ca65

pub type Byte = UInt<8>
pub type Word = UInt<16>

@Isolated(to: MainExecutor)
pub object PictureProcessingUnit {
    pub let width = 32
    pub let height = 30

    pub type Tile = Byte
    pub type Palette = UInt<2>

    @MemoryLayout("packed")
    pub struct Sprite {
        pub let mut y: Byte
        pub let mut tile: Tile
        pub let mut options: Options
        pub let mut x: Byte

        pub init(
            x: Byte,
            y: Byte,
            tile: Tile,
            palette: Palette,
            priority: Boolean = false,
            mirror_x: Boolean = false,
            mirror_y: Boolean = false
        ) {
            self.x = x
            self.y = y
            self.tile = tile
            self.options = .init(
                palette: palette,
                priority: priority,
                mirror_x: mirror_x,
                mirror_y: mirror_y
            )
        }

        pub fun hide(mut &self) {
            self.y = 0xFF
        }

        @MemoryLayout("packed")
        pub struct Options {
            pub let mut palette: Palette
            let _: UInt<3> = 0 // Unused
            pub let mut priority: Boolean
            pub let mut mirror_x: Boolean
            pub let mut mirror_y: Boolean

            pub init(palette: Palette, priority: Boolean, mirror_x: Boolean, mirror_y: Boolean) {
                self.palette = palette
                self.priority = priority
                self.mirror_x = mirror_x
                self.mirror_y = mirror_y
            }
        }

        /// A default off-screen sprite.
        pub const default = Self(x: 0, y: 255, tile: 0, palette: 0)
    }

    // Initializers used from an extension of Pointer by the ca65 module, wrapping the intrinsics for us.
    let oam = Pointer<of: Sprite>(segment: "SHADOW_OAM", offset: 0)
    let oamaddr = Pointer<of: Byte>(address: 0x2003)
    let oamdma = Pointer<of: Byte>(address: 0x4014)

    pub subscript get(x: Byte, y: Byte) -> Tile { todo() }
    pub subscript set(x: Byte, y: Byte, _ tile: Tile) { todo() }

    pub subscript get(sprite: Byte) -> Sprite {
        unsafe oam.volatile(store: sprite, at: sprite)
    }

    pub subscript set(sprite: Byte, _ sprite: Sprite) {
        unsafe oam.volatile(load: sprite, at: sprite)
    }

    pub subscript mut(sprite: Byte, _ mutation: (sprite: mut &Sprite) -> ()) {
        unsafe oam.volatile(mutate: mutation, at: sprite)
    }

    // Synchronously synchronizes sprites at a very high speed by performing a dma transfer.
    pub fun synchronize_sprites_to_ppu() {
        unsafe oamaddr.volatile(store: 0x00)
        unsafe oamdma.volatile(store: 0x02) // The shadow oam is located on this page through the ld65 linker script.
    }

    pub fun initialize() {
        for sprite in 0..<64 do Self[sprite: sprite] = .default
    }

    @Extern
    @!SymbolName("rt_has_nmi_callback")
    fun has_nmi_calback() -> Boolean

    @Extern
    @!SymbolName("rt_set_nmi_callback_once")
    fun set_nmi_callback_once(userdata: OpaquePointer, _ callback: (userdata: OpaquePointer):[] -> ())

    pub fun vblank() async throws VBlankDoubleAwait {
        if unsafe has_nmi_callback() then throw VBlankDoubleAwait

        await with_continuation |c| {
            // We can use a static allocator because we guard re-entry.
            // It will resolve and reserve a free ram address for us statically and reuse it each time.
            unsafe set_nmi_callback_once(userdata: Box(c, in: unsafe StaticAllocator).erase()) |userdata| {
                let box = unsafe Box<of: c::Type>(recover: userdata)
                Task(on: MainExecutor, priority: .high) ||:[box] box->resume()
            }
        }
    }

    pub object VBlankDoubleAwait
}

@Entry
@Executor(MainExecutor)
fun main() async {
    PictureProcessingUnit.initialize()

    let mut x: Byte = 0

    PictureProcessingUnit[sprite: 0] = .init(x: x, y: 16, tile: 1, palette: 0)

    // Slide the sprite right on each frame.
    // The compiler will be able to implement efficient in place mutation through
    // inlining the projection closure synthesized from the "lvalue" syntax.
    loop {
        PictureProcessingUnit[sprite: 0].x = x.add(wrapping: 1) // Add without throwing overflow.

        // Wait and maybe let the executor do some other things until vblank.
        await PictureProcessingUnit.vblank()

        // We are synchronized to the start of a frame and can now transfer sprites during vblank.
        PictureProcessingUnit.synchronize_sprites_to_ppu()
    }
}

```

This looks useful to avoid SSA entirely on memory bound register based architectures
(basically, all modern CPUs):

https://en.wikipedia.org/wiki/Sethi–Ullman_algorithm

That is because Strawberry is all about immutable expressions, mutation is
not idiomatic for algorithms, only for the ovaraching simulation.

The syntax changed a little so I need to update the parser:

There was a mistake in the grammar (something I already fixed in the past but forgot about):
```
|| [a] // capture of a or returned list expression of a?
```
And the actual syntax was supposed to require a colon for the capture list `||:[]`

Additionally, there needs to be an expression (primarily for use in where clauses of declarations) `Type: Type`. This is basically just a boolean expression checking if subtyping, usually compile time only (though for classes it will work at runtime)

It should be unambiguous everywhere because the colon is not an operator and usable freely `if A: B then print("a is a subtype of b")

Additionally, the syntax for closures was finalized so it can be implemented.
It is an extension of parenthesized expressions and adds the following grammar:
```
(a: Int, b: Int) -> (x: Int, y: Int) // Callable category (generic)
(a: Int, b: Int):[Int] -> (result: Int) // Callable type (concrete)
(a: Int, b: Int):[] -> (result: Int) // Callable function (concrete, thin because there's no captures)

// Plus there are the keywords
() async -> ()
() throws A, B -> ()
() async throws -> ()
```
This nicely unifies the clean syntax between generic and concrete, and supports runtime polymorphism of functions.

On top of everything the language introduces a new grammar, `@` and `@!`.
Previously the grammar was just an `@` symbol for annotations followed by an expression, but that meant annotations with unsafe initializers were noisy and hard to read `@unsafe Annotation`. The exclamation version is the unsafe equivalent for annotation expressions, still easily searchable in the codebase but visually less cluttered. This extends to allowing `@` as a prefix expression, which will allow attaching annotations to expressions, such as `@Convention("c") ():[] -> Int`

No we can't use a terrible waker model like Rust, the model is that of continuations.

We have stateful closures in the language, with caregory (constraint) signatures `() -> ()` and concrete syntax for thin functions `():[] -> ()` which are just a closure with an empty capture list.

What is a continuation if not `()[AsyncContext] -> ()`?

`async` should be sugar not for state machines but control flow (closer to Rust's control flow divergence operator trait `?`)

It must be a continuation based model, where the await point invokes such divergence with the continuation callable of the rest of the function and the associated context, which there is already general support for (captures are really just an associated tuple after all), where the context is used to preserve variables of lexical lifetimes (noncopyable) (or used across the await in some other way). The resumption is done simply by calling the continuation callable. This also works perfectly because this already encodes ownership by virtue of being a concrete tuple associated type:
```
()[Int] -> () // copyable
()[MoveOnlyType] -> () // move only
()[&Int] -> () // lexically bound and nonescapable
```
This isn't like rust, it's closer to C++ just with proper polymorphic callable syntax (C++ is nominal which is kind of stupid), an owning closure can be called many times, because we use strict lexical lifetimes.

Continuations are much better (push) than Rust's Futures/Wakers (pull with a push thing on the side). They can be safe to implement,

I can see potential here but I can't quite wrap my head around it quite yet

To support arguments to awaitable functions we use tuples, the executor would have a generic function I suppose, generic over the pack, category and so on, and use compile time reflection to actually comprehend and enqueue it?
```
// We escape the closure, hence we must use a lifetime tie (which inherently only allows escapable closures).
// Any is the top type, making this very close to `auto` in C++ but verified with a rigid type variable.
fun enqueue(mut &self, continuation: Any, awaiting: Any, arguments: ...Any) throws EnqueueError
```

This model should even allow us to handle enqueue failure through templated monomorphized exceptions, like an allocation error, though they would be required to diverge?

```
await foo() async catch {
    enqueue: EnqueueError -> todo() // must diverge
}
```

# Old pointer

```
module core.memory

/// A pointer in Strawberry is a raw, escapable reference to one or multiple values. It is not
/// necessarily implemented as an integer representing a linear memory address,
/// because Strawberry does not depend on linear memory for its specification.
///
/// Code must be very careful with this type, and never rely on anything beyond the most naive
/// interpretation of the individual operations it provides.
pub struct Pointer<to: Pointee>: not Sendable {
    @Hidden
    pub let value: #Pointer(Pointee)

    @Hidden
    pub init(raw value: #Pointer(Pointee)) {
        self.value = value
    }

    /// Assume the pointee is uninitialized and initialize it with a value.
    pub unsafe fun initialize(self, to value: Pointee) {
        #pointer_initialize(self.value, value)
    }

    /// Assume the pointee is initialized and take ownership of the value.
    pub unsafe fun deinitialize(self) -> Pointee {
        #pointer_deinitialize(self.value)
    }

    /// Assume the slot is uninitialized and initialize it with a value.
    pub unsafe fun initialize(self, to value: Pointee, at index: UInt) {
        #pointer_initialize_at(self.value, index.value, value)
    }

    /// Assume the slot is initialized and take ownership of the value.
    pub unsafe fun deinitialize(self, at index: UInt) -> Pointee {
        #pointer_deinitialize_at(self.value, index,value)
    }

    /// Single element pointer access.
    pub unsafe get pointee(self) -> Pointee 'self {
        #pointer_get(self.value)
    }

    /// Single element pointer access.
    pub unsafe set pointee(self, _ value: Pointee) {
        #pointer_set(self.value, value)
    }

    /// Single element pointer access.
    pub unsafe mut pointee(self, _ mutation: (value: mut &Pointee) -> ()) {
        #pointer_mut(self.value, mutation)
    }

    /// The container projection operator `let x = self->member`.
    pub unsafe get self(self) -> Pointee 'self {
        self.pointee
    }

    /// The container projection operator `self->member = x`.
    pub unsafe set self(self, _ value: Pointee) {
        self.pointee = value
    }

    /// The in place container projection operator optimization `self->member += x`.
    pub unsafe mut self(self, _ mutation: (value: mut &Pointee) -> ()) {
        mutation(self.pointee)
    }

    /// The container projection operator `let x = self[index]`.
    pub unsafe subscript get(self, _ index: UInt) -> Pointee 'self {
        #pointer_get_at(self.value, index.value)
    }

    /// The container projection operator `self[index] = x`.
    pub unsafe subscript set(self, _ index: UInt, _ value: Pointee) {
        #pointer_set_at(self.value, index.value, value)
    }

    /// The in place container projection operator optimization `self[index] += x`.
    pub unsafe subscript mut(self, _ index: UInt, _ mutation: (value: mut &Pointee) -> ()) {
        #pointer_mut_at(self.value, mutation)
    }

    /// Write the data from another pointer into this one, transfering ownership.
    /// Assumes this pointer is has uninitialized data and the other pointer has initialized data.
    pub unsafe fun write(self, from other: Self, count: UInt) {
        #pointer_write(self.value, other.value, count.value)
    }

    /// Volatile access to the pointee.
    pub unsafe get volatile(self) -> Pointee 'self {
        #pointer_volatile_get(self.value)
    }

    /// Volatile access to the pointee.
    pub unsafe set volatile(self, _ value: Pointee) {
        #pointer_volatile_set(self.value, value)
    }

    /// The container projection operator with volatile semantics `let x = self[volatile: index]`.
    pub unsafe subscript get(self, volatile index: UInt) -> Pointee 'self {
        #pointer_volatile_get_at(self.value, index.value)
    }

    /// The container projection operator with volatile semantics `self[volatile: index] = x`.
    pub unsafe subscript set(self, volatile index: UInt, _ value: Pointee) {
        #pointer_volatile_set_at(self.value, index.value, value)
    }

    pub fun erase(self) -> OpaquePointer {
        .init(erasing: self)
    }

    pub init(recover opaque: OpaquePointer) {
        self.value = #pointer_recover(Pointee, opaque.value)
    }

    pub fun rebind<to: T>(self) -> Pointer<to: T> {
        .init(raw: #pointer_rebind(T, self.value))
    }

    pub static let null = Self(raw: #pointer_null(Pointee))
    pub static let dangling = Self(raw: #pointer_dangling(Pointee))
}

extend Pointer: Equatable {
    pub infix operator == fun equal(self, to other: Other) -> Boolean {
        #pointer_eq(self.value, other.value)
    }
}

extend Pointer: Equatable<to: OpaquePointer> {
    pub infix operator == fun equal(self, to other: Other) -> Boolean {
        #pointer_eq(self.value, other.value)
    }
}

pub struct OpaquePointer {
    @Hidden
    pub let value: #OpaquePointer()

    @Hidden
    pub init(raw value: #OpaquePointer()) {
        self.value = value
    }

    pub init(erasing pointer: Pointer) {
        self.value = #pointer_erase(pointer.value)
    }

    pub fun recover<to: T>(self) -> Pointer<to: T> {
        .init(recover: self)
    }

    pub static let null = Self(raw: #pointer_null())
    pub static let dangling = Self(raw: #pointer_dangling())
}

extend OpaquePointer: Equatable {
    pub infix operator == fun equal(self, to other: &Other) -> Boolean {
        #pointer_eq(self.value, other.value)
    }
}

extend OpaquePointer: Equatable<to: Pointer> {
    pub infix operator == fun equal(self, to other: &Other) -> Boolean {
        #pointer_eq(self.value, other.value)
    }
}

```


The language only allows tail recursion, requires a `recurse` keyword otherwise because that's unportable.

It must be able to do both, and so I would extend the syntax a bit:
```
// sugar
fun foo() async {}
// desugar
fun foo(continuation: () -> ())
```

So, a function that declares itself simply async is generic. It will work on arbitrary continuations, but it will require individual instantiations to achieve this.
But that is not always ideal. Sometimes on different platforms async will need to work across ABI boundaries. On macOS it will have to be able to integrate with Swift's async calling convention `@Convention("swiftasync") ():[SwiftAsyncContext] -> ()` for example, to take advantage of modern platform APIs.

The approach will work by establishing a contract for erasure:
```
// sugar
fun foo() async StaticAsyncContext {}
// desugar
fun foo(continuation: StaticAsyncContext)
```

When awaiting this concrete version of foo the language will instead expect this category to be satisfied or it will compile error:
```
pub category AsyncContext {
    implicit init<C, arguments: ...Arguments, captures: ...Captures>(continuation: C)
    where
        C: (Arguments...):[Captures...] -> ()
}
```

Effectively, we can provide a custom erasure contract of our own through multiple variadic generic packs enabled by our generic argument labels and compile time reflection:

```
pub struct StaticAsyncContext: AsyncContext {
    let

    init<C, arguments: ...Arguments, captures: ...Captures>(continuation: C)
    where
        C: (Arguments...):[Captures...] -> ()
    {

    }
}
```

This shifts the generic instantiation to the erasure container's initializer, which is inlinable, and we can do things like use the `::` operator as a prefix expression to reflect the current declaration itself, and query its properties, in this case to protect our precondition and compile error (because it's a constant expression so the panic will be compile time).

Strawberry doesn't wrap pointers in optionals, they're unsafe primitives. Pointers have a null constant for comparison. However, it would be valuable to let the language optimize for this kind of thing for higher level structures (like the Box). We could introduce an unsafe to apply annotation as a hint.
```
pub annotation Unrepresentable {
    pub let values: List<of: Any>

    pub unsafe init(_ values: ...Any) {
        self.values = [values, ...]
    }
}

@!Unrepresentable(OpaquePointer.null)
struct Box<of: Inner, in: Allocator> where Allocator: core.memory.Allocator { ... }
```

The one thing Strawberry must never do is hardcode strict magic core library awareness in the compiler. Currently there is actually none of it in the entire design, the compiler only cares about evaluating expressions and intrinsics. Reflection annotations are an exception because they are only hints, not a hard dependency.

The only one curiosity is how the heck do we form the literal types:
```
/// An arbitrarily precise compile time decimal.
///
/// Decimals do not decay because there is no objectively undisputable type to decay into.
pub const struct Decimal: unsafe Copyable, unsafe Sendable {
    @Hidden
    pub let value: #Decimal()

    @Hidden
    pub implicit init(raw value: #Decimal()) {
        self.value = value
    }
}

/// A raw, unescaped literal string of source encoding usually formed from literals.
///
/// Raw strings immediately decay to a `String`.
pub const struct RawString: unsafe Copyable, unsafe Sendable {
    @Hidden
    pub let value: #RawString()

    @Hidden
    pub implicit init(raw value: #RawString()) {
        self.value = value
    }

    pub const decay type = String
}
```

The rule will be "for all visible types there can only be one unambiguous type implicitly initializable from the raw intrinsic type corresponding to a literal". Not that it would be useful, but it does mean that if the core library for example renames RawString to LiteralString the compiler itself doesn't break (Swift does this a lot requiring matching changes the compiler and library, often in multiple places or the semantic analyzer will crash). Theoretically the library and compiler would be independently versioned as long as intrinsics are unbroken, and because the library is responsible for most semantics one could just use an older LTS core module, which only requires backporting security patches and not maintaining the entire old compiler. This will also allow the language to evolve in fundamental library breaking ways, like completely rearchitecting how allocators work. Because the code is generic the old implementation will remain compilable and correct, amortizing the migration.

the migration could even be incremental:
```
// In a partially migrated codebase which configures legacy `core.v1` as an autoimport instead of latest default `core`.
// This code unit (file) will override the implicit v1 import of the general codebase
// and be able to reference old constructs by their qualified path only for interoperation with legacy parts of the codebase
import qualified core.v1
// We now import core itself which will make this unit resolve newer literals if they were reimplemented etc.
import core
```

module core.memory

/// A highly unsafe category which provides raw blocks of storage sufficient for allocating a type.
///
/// Storage operated on by allocators does not handle initialization or deinitialization automatically.
pub unsafe category Allocator {
    /// Perform a new memory allocation.
    unsafe fun allocate<T>(mut &self, count: UInt = 1) -> Pointer<to: T>

    /// Free a memory allocation.
    unsafe fun deallocate<T>(mut &self, _ pointer: Pointer<to: T>, count: UInt = 1)

    /// Resize an existing memory allocation.
    unsafe fun reallocate<T>(
        mut &self,
        _ pointer: Pointer<to: T>,
        old_count: UInt,
        new_count: UInt
    ) -> Pointer<to: T> {
        let result = unsafe self.allocate<T>(count: new_count)
        unsafe result.copy(from: pointer)
        unsafe self.deallocate(pointer, count: old_count)
        result
    }
}

/// Const overloads for compile time memory allocation.
///
/// When allocation happens at compile time the language knows to form a compile time
/// memory allocation, which is tracked by the pointer intrinsic. The intrinsic is initialized
/// with knowledge of the concrete allocator type so that the backend will know to use it if it has to
/// materialize the allocation later.
///
/// Of course if the allocation is performed later that's rather unfortunate as we can't handle
/// exceptions, they will just monomorphize to a panic. Constant allocation can't fail so these
/// overloads don't actually throw `Exception`, so if `catch` reaches this they will intentionally not match.
extend Allocator {
    unsafe const fun allocate<T>(mut &self, count: UInt = 1) -> Pointer<to: T> {
        unsafe .init(raw: #pointer_allocate(Self, T, count))
    }

    unsafe const fun deallocate<T>(mut &self, _ pointer: Pointer<to: T>, count: UInt = 1) {
        #pointer_deallocate(T, pointer.raw, count)
    }

    unsafe const fun reallocate<T>(
        mut &self,
        _ pointer: Pointer<to: T>,
        old_count: UInt,
        new_count: UInt
    ) -> Pointer<to: T> {
        let result = unsafe self.allocate<T>(count: new_count)
        unsafe result.copy(from: pointer)
        unsafe self.deallocate(pointer, count: old_count)
        result
    }
}

/// An allocator which doesn't support runtime allocation.
/// It still satisfies the `Allocator` category because it is itself const only,
/// which means the const defaults are already exhaustive.
pub const object ConstAllocator: unsafe Allocator

/// The standard C memory interface implemented in terms of malloc, realloc and free.
pub object LibcAllocator: unsafe Allocator {
    @Convention("c")
    @Extern(library: "c")
    @unsafe SymbolName("malloc")
    unsafe fun malloc<T>(size: UInt) -> Pointer<to: T>

    @Convention("c")
    @Extern(library: "c")
    @unsafe SymbolName("free")
    unsafe fun free<T>(_ pointer: Pointer<to: T>)

    @Convention("c")
    @Extern(library: "c")
    @unsafe SymbolName("realloc")
    unsafe fun realloc<T>(_ pointer: Pointer<to: T>, new_size: UInt) -> Pointer<to: T>

    pub unsafe fun allocate<T>(mut &self, count: UInt = 1) throws AllocationFailure -> Pointer<to: T> {
        if #size_of(T) == 0 then return .dangling
        let result = unsafe malloc(size: count * #size_of(T))
        if result != .null then result else throw AllocationFailure
    }

    pub unsafe fun deallocate<T>(mut &self, _ pointer: Pointer<to: T>, count: UInt = 1) {
        if #size_of(T) == 0 then return
        unsafe free(pointer)
    }

    pub unsafe fun reallocate<T>(
        mut &self,
        _ pointer: Pointer<to: T>,
        old_count: UInt,
        new_count: UInt
    ) throws AllocationFailure -> Pointer<to: T> {
        if #size_of(T) == 0 then return .dangling
        let result = unsafe realloc(pointer, new_size: new_count * #size_of(T))
        if result != .null then result else throw AllocationFailure
    }

    pub object AllocationFailure
}

/// The default allocator for the target backend.
pub type DefaultAllocator = #backend() match {
    "llvm" -> LibcAllocator
    _ -> ConstAllocator
}

I also think I like how the overloading rules interact with exceptions here.

Categories do not specify exceptions (if the contract is failable by design that's what Optional and Result are for, like networking). That is very intentional because exceptions are just a templating mechanism specifically made for breaking those semantic contracts, otherwise categories would be a mess (Int implements AdditiveArithmetic, but Int throws arithmetic overflow and underflow because it breaks the mathematical properties true integers would have; AdditiveArithmetic could be generic over a variadic pack of exceptions for each method but that does not scale).

Instead, a throwing function still satisfies a category, because every throwing function (if catch is not used) simply instantiates:
```str
fun foo() throws E -> Int {
    throw E
}

fun bar() {
    foo()
    foo() catch e: E {}
}
```

Desugaring the exception template instantiation:

```str
fun foo() throws <> -> Int {
    throw E
}

// The return type at calling convention level would be a compiler intrinsic union of E and Int
fun foo() throws <E> -> Int {
    throw E
}

fun foo() throws <> -> Int {
    panic(with: E)
}

fun bar() {
    foo<>()
    // The postfix catch is a special pattern match on the compiler intrinsic union
    foo<E>() catch e: E {}
}
```

where:

```str
module core

pub fun panic(because reason: String) -> Never {
    loop {}
}

pub fun panic<with: Exception>(with exception: Exception) -> Never {
    panic(because: "uncaught exception: " + Exception::path)
}

pub fun todo() -> Never {
    panic(because: "todo")
}
```

The templating is conceptually forwarded if our exception list allows a given exception to filter through:
```str
foo bar() throws A { throw A }

fun foo() throws A {
    bar() // the exception is forwarded conceptually because foo's list allows through a catch of A, chaining the templates.
}
```

This system allows true function composition while preserving exceptions, even for higher order function categories using the reflection meta member operator to obtain the variadic pack of exceptions and allow a catch of them to transparently pass into foo:

```str
/// The caller of foo can use a catch expression to catch the forwarded exceptions.
fun foo(bar: () -> (), baz: () -> ()) throws (bar::throws, ...), (baz::throws, ...) { bar(); baz() }
```

So in the case of allocators an interesting edge case happens, where the const allocation doesn't throw, but choosing those const overloads if the user was actually using `allocator.allocate() catch { ... }` would eat the exception so the overloading rules pick the runtime allocation instead (which is what we want in this case).
When the language implicitly uses the allocator to reify a const allocation it doesn't catch which just panics, with the only potentially difficult thing to understand for new developers at first is why it panics in a very deferred place.
Not catching the exception initially is a contract to the compiler that you assume (semantically) that it doesn't happen.

So, exceptions exist to implement category semantics while potentially diverging from their laws, and the mechanism in place seems sound.

# Lifetime ties and universal projective types

We can make universal view types by applying lifetime ties to initializers.

A lifetime-tied initializer has some special rules:

- the tied type may not be mutable so we can't mutate it during the initializer (in)
- the tie binds the lifetime of the instance to the input tie
- the view type is immutable if the tied value is a borrowed projection
- after the projective type is deinitialized the member is written back (out)
- the view type may not mutate a potentially tied value in the deinitializer

Because the binding itself looks somewhat unassuming the mutability modifier is explicitly denoted as conditional.

```str
pub struct AsciiString<in: Allocator = DefaultAllocator>: StringLike where Allocator: core.memory.Allocator {
    pub(get) let mut? string: String<in: Allocator>

    pub init(from string: String<in: Allocator>) 'string throws String.NotAscii {
        if not string.is_ascii() then throw String.NotAscii
        self.string = string
    }
}
```

There is also the question of methods.

One solution would be to ban methods from mutating potentially mutable members by default, and forcing members that need it to declare it in the signature

```str
fun mutates(mut &self) { self.string = "" } // error
fun mutates(mut? &self) 'self.string { self.string = "" } // ok
```

The language would have to infer which `slice` we are using from the binding
I am not sure if this doesn't have ambiguous edge cases
```str
    pub fun slice(&&self, x: Int, y: Int, width: Int, height: Int) -> Slice<of: Self> 'self {
        .init(of: &&self, x: x, y: y, width: width, height: height)
    }

// the expected binding is a borrowed projection, so we prefer `&self` throughout the expression
let &a = plane
    .slice(x: 0, y: 0, width: 10, height: 10)
    .slice(x: 0, y: 0, width: 10, height: 10)
    .slice(x: 0, y: 0, width: 10, height: 10)

// the expected binding is owned, so we prefer `self` throughout the expression
let a = plane
    .slice(x: 0, y: 0, width: 10, height: 10)
    .slice(x: 0, y: 0, width: 10, height: 10)
    .slice(x: 0, y: 0, width: 10, height: 10)

// the expected binding is mutably projected, so we prefer `mut &self` throughout the expression
let mut &a = plane
    .slice(x: 0, y: 0, width: 10, height: 10)
    .slice(x: 0, y: 0, width: 10, height: 10)
    .slice(x: 0, y: 0, width: 10, height: 10)
```

Note that the `&&` may not be mutable, so if we need distinction (where the mutable version mutates) we would still write three overloads (but that should be rare)

The syntax also visually matches what C++ developers are already used to (forwarding the value category) though hopefully powered by bidirectional inference in this case.
