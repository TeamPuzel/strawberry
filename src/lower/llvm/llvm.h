#ifndef LLVM_H
#define LLVM_H
// We will need two fundamental implementations here, primarily to output llvm-ir in its text format but also
// potentially the binary bitcode format for efficiency. I'd rather not use LLVM as a dependency and just
// generate it myself for the sake of simplicity. One should only require LLVM to use the LLVM backend,
// not for building the toolchain itself. This is also important for easy cross compilation of the toolchain.
//
// LLVM is a good first backend for the language beyond reusing the interpreter for the runtime.
// It will provide excellent, highly optimized support for common architectures such as x86_64 which
// are too complicated for me to care to support on my own.
//
// Other from-scratch backends will be used for more exotic architectures, including ones LLVM isn't
// designed for in the first place. Strawberry does not even rely on linear memory after all.

#endif
