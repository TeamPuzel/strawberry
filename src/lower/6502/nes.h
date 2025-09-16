#ifndef NES_H
#define NES_H
// This is a very esoteric platform something like LLVM is unlikely to ever support, and most languages are
// inherently incompatible with its constraints. Strawberry is, uniquely, a modern programming language designed
// in a way that's compatible with architectures like this. There is a notion that such constrained platforms
// are best targeted by handwritten assembly code, and to some extend that's true. C itself is already too concrete,
// making assumptions about arithmetic availability or memory. But I believe that a sufficiently abstract meta language
// would be perfect for a platform like this.
//
// The NES will be an important target for the language, a test proving the vision correct. If a platform
// this primitive and esoteric can work, the fundamental goal of true portability through semantic, generic programming
// can be said to be achieved.
//
// It should also not take too much work to implement such a backend. I would like Strawberry to become a toolchain
// a single developer can relatively easily adapt for whatever use case they want, such as homebrew for
// all sorts of abandoned hardware which tends to just have ancient third party gcc support, limiting developers
// to C and ancient versions of C++.

#endif
