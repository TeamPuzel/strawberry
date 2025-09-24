#ifndef NES_H
#define NES_H
#include "../../parse/sir.h"
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
//
// The NES has fairly limited memory and we need to make clever use of it to work around instruction set limitations.
// The allocation is defined like this:
// 00 00...00 3F :: 8 64 bit pseudo registers, used to pass arguments before falling back on the stack.
// 00 40...00 FF :: default static allocation.
// 01 00...01 FF :: the hardware stack, only used for return addresses.
// 02 00...07 FF :: the software stack, used for argument overflow and local variables.
//
// Due to the limited stack space allocating large data structures on it is ill advised, so a cartridge with
// a lot of additional memory is recommended, managed by a global allocator.
//
// Strings are allocated in the rom instead as they are rather large.
// The recommended mapper configuration is MMC5 as it provides a lot of memory and rom.

// NES specific configuration flags.
typedef struct NesConfig {
    // Allows the backend to lower unofficial NES specific opcodes.
    // Occasionally this can result in less bytes or cycles used.
    bool use_unofficial_opcodes;
} NesConfig;

void lower_6502_nes(SirProgram program);

#endif
