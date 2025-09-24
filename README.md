# The Strawberry Project 🍓

Software from scratch for the sake of a simpler world.

## What is Strawberry

- Strawberry is a project with the goal of breaking convention and starting over, focused on long term sustainability.
- Strawberry is a meta programming language focused on extreme generic programming without using concrete primitives.
- Strawberry is an extensible, portable toolchain for working with portable software projects.
- Strawberry is a semantic plain text encoding for efficient and safe processing by default with extensible
  domain specific spans such as embedded UTF-8 or semantic emoji.
- Strawberry is a kernel and operating system focused on individual user freedom and fun.

This is the specification and implementation of the Strawberry Project.

## License

There is no license currently as I still have to find one I like best. The general spirit of the license will be:
- No use of the source for training AI.
- Do whatever you want where object distribution does not need to include the license, only source distribution does.

## Contribution

This project does not accept any contribution due to the annoying to keep track of copyright nonsense.

## Compatibility

Please do not call an incompatible implementation with the same or similar name that would imply it is
an implementation of the language. I would like for anyone using the language to be sure that a toolchain is conformant.

## Why yet another language?

Strawberry is an abstract language. It has no concept of pointers or even numbers of concrete semantics
or memory representation. In fact, Strawberry has no inherent concept of memory at all. It works by implementing
these concepts with the libraries themselves and communicating with the compiler backend through intrinsics, allowing
for a standardized syntax for heavily templated code.

This intention is unconventional. Strawberry is a simple meta language inspired by generics/templates and consteval
as seen to some extent in C++, Swift and Rust. It is a type system and an expression of logic
through abstract semantics with no particular concrete implementation.

What does this buy for the language? It means that while a Strawberry program may rely on primitives
only available for certain platforms, most code can be implemented in an abstract, truly portable way. It should
allow for reuse of the same abstract code on anything from a modern CPU, a GPU and architectures of the future or past.
Strawberry could be used for a Minecraft mod on the JVM, an 8 bit NES game, embedded programs, scripting, DSLs,
anything — because Strawberry is a self contained meta language designed purely with logic expression in mind.

I believe that this vision is not yet achieved by any language currently available. They all rely on some fundamental
idea like a "long" or "i32", or specific addressing semantics. That's fine but it makes everything built in terms
of these assumptions inherently unportable, and I'd like to explore the possibilities of avoiding this issue.

The main motivation for starting this project was trying to get any of my favorite modern languages to compile
a game for the PS2 and finding that it's actually impossible without years of work. At that point, I'd rather spend
these years working on a simpler, even if less efficient, solution to these problems.

That being said, Strawberry for any normal user should be about as simple and nice to use as Swift.
It is also cleaned up syntactically compared to something like Rust, with much prettier and more redable code.

## Values

Here are some of the values of this project.

### Simplicity

When faced with a compromise the intention is that Strawberry will prioritize simplicity. The language itself
is not a simple design, but ideally the project itself should be maintainable by a single talented developer.

It is not meant to be completely self contained but any dependency should be optional, for example a backend
taking advantage of the insane amount of work put into LLVM in order to support conventional platforms is a great idea —
but LLVM must not be a dependency for anything but that kind of disposable module. At its core it's a simple C program
because C is generally implemented pretty much everywhere and long into the past, allowing this reference toolchain
to work at least in some capacity on ancient platforms, which should make porting this core component easier.

### Fun

Programming should be fun and the programmer should be able to do whatever they want. Straberry will not avoid abilities
inherent to its expremely flexible semantics even when that means one could write terrible code. That is not for
the language to decide. There is nothing I hate more than design by subjective opinion.

### And more

As I work on it I'll try to fill in other values.
