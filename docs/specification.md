# Draft Specification of the Strawberry Programming Language 🍓

The Strawberry Programming Language is an abstract meta language focused on portability and expressiveness
through a complete compile time metaprogramming and reflection system and a focus on generic programming.

### Disoriented Programming

A question I asked myself many times is how Strawberry should be "oriented". I concluded that the concept
of orienting oneself around a way of thinking is the very definition of thinking inside a box. In that sense,
the language should not be oriented at all, but rather a language for expressing complex type relationships and logic.

## Language constructs

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
