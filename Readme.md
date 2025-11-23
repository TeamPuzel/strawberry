# The Strawberry Programming Language 🍓

## Motivation

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

## Contributing

Currently there's no licensing. The language itself will be licensed very permissively, maybe in the public domain
but that seems to be legally problematic. I would like users to own their copy of the toolchain.

The default Rust bootstrap interpreter will probably just be MIT as it's a more replaceable soft dependency.

## Planned backends

The planned backends so far are:
- Interpreter (inherent, the language can't function without compile time evaluation anyway).
- 6502 (no LLVM), to prove that the language can scale to extreme targets and cross compile effortlessly.
- LLVM, to take advantage of existing platforms easily, there is no value in duplicating effort here as long as
  the language does not depend on LLVM, only that specific backend class.
- JVM, to prove the language can handle more unconventional object systems.
- JavaScript, because the web is an important low friction platform and cheap interoperability in a safe language
  could prove valuable, unless WASM becomes capable of replacing it completely.
- C, it could be a last resort target for low effort integration with hostile, proprietary SDK environments.

## Structure

- `archive` contains temporarily archived files that still have some value.
- `backend` contains the target backend implementations excluding the interpreter which is inherent.
- `bootstrap` contains the bootstrap interpreter used to build the compiler from scratch.
- `doc` contains project documentation.
- `modules` contains the default set of modules, including `core` which implements default primitives.
- `src` contains the default posix frontend.

## Bootstrapping

The language development generally requires multiple stages.
- Stage 0, the bootstrap interpreter. A Rust implementation is being developed.
- Stage 1, the interpreted compiler with no features hardcoded for bootstrapping, with no classes.
- Stage 2, the self hosted compiler capable of using classes.
- Stage 3, the native backend can now be used to obtain a native build of the compiler.

Initially the plan was to use C, but realistically Strawberry is focused on independence and cross compilation.
Rust is not terribly portable, even dropping x86 macOS support early,
but the low effort solution for older unsupported platforms like that would be to
just bootstrap on a different machine, or use a C transpiler backend. It is no longer a concern
since the language is going to be self hosted.
