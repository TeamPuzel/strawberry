#ifndef EVAL_H
#define EVAL_H
// This is the place for the interpreter implementation. The Strawberry language requires
// a fully functional interpreter for compile time evaluation, which can also be reused to evaluate
// runtime programs this way, negating the need for any backend to exist at the cost of inefficiency.
// The current plan is to evaluate the source tree directly without any optimization, but in the future optional
// optimizations could be introduced. The important part is to maintain modularity so that all such optimizations
// can be discarded keeping the core implementation simple and reusable.

#endif
