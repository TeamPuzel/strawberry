# Categories for the Strawberry Programming Language 🍓

The intention of this document is to specify the semantics of categories as a language construct in a way
that avoids unsound behavior or devolving into an unconstrained template system which can't be introspected by itself.

Categories derive from Haskell typeclasses, Swift protocols, Rust traits and C++ templates, attempting to find
a good balance between expressiveness and clarity.

Fundamentally their purpose is to model type hierarchies as abstractly as possible, imposing little
restriction on implementations.

## Nominal conformance

A category is a namespaced, nominal class of types assigning semantics to a protocol of concrete signatures.

A concrete type can declare conformance to a set of categories, promising to uphold not only the functionality
of the category but also the semantics, such as `RandomAccessCollection` types having `O(1)` complexity for indexed
subscript access. Violating these semantics, which can be referred to as laws of such a category, is a logic
error and can produce invalid results, but not undefined behavior.

Categories which rely on laws which guard
against undefined behavior are marked `unsafe` and require explicit opt-in to declare conformance,
due to the severity of any mistake undermining the soundness of the language, for example implementing
synchronization or allocation primitives.

Categories are used as follows:

```
category A {
    fun foo(self) -> Int
}

struct S: A {
    pub fun foo(self) -> Int { 0 }
}

fun bar() {
    let s = S()
    s.A.foo()
}
```

A category can be accessed implicitly if there is no ambiguity, meaning no derivation from two different categories
containing conflicting signatures or a reasonable overload resolution for the usage.

```
fun bar() {
    let s = S()
    s.foo()
}
```

Suppose an ambiguity is introduced however:

```
category B {
    fun foo(self) -> Int
}

extend S: B {
    pub fun foo(self) -> Int { 1 }
}
```

It can no longer be accessed without qualification. However, this can also be resolved in any situation where
the type is erased.

```
fun bar() {
    let s = S() as A // s is of erased category type A
    s.foo()          // We can only see the protocol of category A so no ambiguity here
}
```

This system appears simple and unambiguous so far, except for implementation of multiple categories inherently
or in an extension. To clarify, both cases will work as follows:

```
// Default case, where a collision is resolved by using the implementation to satisfy both categories.
struct M: A, B {
    pub fun foo(self) -> Int { 0 }
}
```

This is currently the only way to do so, and if the intention is to provide different implementations
they should just be implemented individually. This ambiguity should be exceptionally rare in the first place
so it does not justify introducing more disambiguation syntax.

## Inheritance, shadowing and overload resolution

### Defaults and derived functionality

Categories can derive defaults in terms of themselves.

```
category Add {
    operator + fun add(&self, to other: &Self) -> Self
    operator += fun(mut &self, other: &Self)
}

extend Add {
    pub operator += fun(mut &self, other: &Self) {
        self = self + other
    }
}
```

This can be simplified to an inline form.

```
category Add {
    operator + fun add(&self, to other: &Self) -> Self
    operator += fun(mut &self, other: &Self) {
        self = self + other
    }
}
```

Both cases provide a default, they can still be overriden. An override of such a function will still be used
even if type erased.

This is not the case for extensions which provide a signature not defined by the category. Those can only be shadowed,
which works only when access is aware of their concrete existence, otherwise such as if they are type erased
the resolution will pick the unshadowed category implementation.

```
category Add {
    operator + fun add(&self, to other: &Self) -> Self
}

extend Add {
    // This implementation can't be overriden because it is not part of the specification.
    // It is purely derived functionality in terms of the category.
    pub operator += fun(mut &self, other: &Self) {
        self = self + other
    }
}

```

### Inheritance

Categories can inherit from each other, which introduces a covariant relationship between them:

```
category C: A, B
```

Such a category will:
- Inherit the requirements from the supercategories.
- Have the ability to override defaults or provide new ones.

## Generic parameterization

What categories allow is static polymorphism and constraints. An erased type can only be used within the
bounds of known categories, so unlike template based generics all generic code can be validated in isolation.

### Concrete generics

At face value generics look much like in any other language and by default parameterize on types:

```
struct Array<T> {}

let a: Array<Int>
```

However there are some additional capabilities to aid inference, specifically labeled parameters.

```
struct Array<of: Element> {}

let a: Array<of: Int>
```

This reads more naturally and aids the language in more complex scenarios:

```
struct Array<const count: Integer, of: Element> {}

let a: Array<count: 3, of: Int> = [1, 2, 3]

// Because the parameters are labeled we can provide only some of them and leave others to be inferred,
// regardless of the order they are declared in. Here count can be inferred from the literal initializer.
let a: Array<of: Int> = [1, 2, 3]
```

In some languages this would look more like `Array<_, Int>` since the language can't disambiguate between them.
This is especially a problem in C++ which can't even do that, forcing designs to order parameters
in a way that may be less intutive, and sometimes no ordering is ideal.

Much like argument labels, constant value parameters can provide separate labels for the inside and outside binding.

```
struct A<const count: Integer>    // normal case, the label matches the constant name: A<count: 1>
struct A<const _ count: Integer>  // anonymous case, there is no label, terse but more ambiguous: A<1>
struct A<const of count: Integer> // named case, the label and constant name differ: A<of: 1>
```

Type parameters can only provide a label or not, since they inherently already introduce their name.

```
struct B<of: Element>
struct B<Element>
```

Simple constructs can omit the label if it does not add much to readability.

In both value and type argument cases, defaults can be provided much like with arguments.

```
struct C<const count: Integer = 0, of: Element = Int>
```

### Category generics

Generics on categories work a little differently because categories are constraints, not concrete types themselves,
which permits a higher degree of inference.

## Static dispatch

Once categories are defined, there are

## Existential dispatch

The language can build tables to allow for runtime polymorphism. This is more complicated than a simple vtable
because of the need to dynamically handle parameterization.

Currently this remains unspecified.

## Variadic generics

## Derivable categories

Some categories describe properties which can be derived in a structural way, for example equality.

```
category Equatable<to: Other = Self> {
    operator == fun equal(&self, to other: &Other) -> Boolean
    operator != fun equal(&self, to other: &Other) -> Boolean { not self.equal(to: other)) }
}

// A default derivation of equality for all types which destructure into tuples.
// This depends on reflection which is not specified yet.
derive Equatable where (T...) = Self, (T: Equatable)... {
    operator == fun equal(&self, to other: &Other) -> Boolean {
        for field of Self::Type::fields do if self::field != other::field then return false
        true
    }
}
```

Simply conforming to such a category will provide these as overrideable defaults.
