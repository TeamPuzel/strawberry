# Control flow for the Strawberry Programming Language 🍓

This document specifies control flow available to the language.

## Blocks

Mosts blocks in the language, such as a function body, implicitly return the result of the last expression
on any given code path. The return keyword should be warned about when used for standard control flow as
it only exists for early returns.

## Exceptions

Exceptions are special control flow dispatched with `throw <expr>` syntax, and they are effectively just
an early return with the error case of the underlying union, unless of course the exception is unhandled and the throw
compiles to a panic instead, but that's irrelevant to control flow.

## Simple branching

Branching has a fairly sugared form in order to look more readable and allow the implementation to optimize
more heavily without needing to be aware of operator implementations. In fact, operators in the language
are functions and can't define short-circuit behavior expected from programming languages.

The prefix `!` as well as infix `&&` and `||` are implemented as normal functions, but they do not use
any precedence nor do they have short-circuit semantics. Instead, keywords `not`, `and` and `or` are used
for most expressions and provide them as a language feature without complicating the entire language
with precedence and associativity rules.

```
// A boolean expression is defined as a sequence of expressions convertible to a boolean
// where each chunk can be inverted. There are two keywords used for chaining, `and` and `or`, and each expression
// can be prefixed with `not` to invert it. The precedence and associativity rules for boolean logic are conventional.
// Specifically, `not` is the highest and right associative to the boolean expression, `and` is the middle and `or`
// is of course the lowest of them all. This logic has lower precedence than normal operators defined in the language.
// Parentheses can of course be used for grouping.
not <boolexpr> and not <boolexpr> or not <boolexpr>

// Evaluates the expression if the boolean expression is true. There is both a normal and ternary form.
if <boolexpr> then <expr>
if <boolexpr> { <statements> }

// Multiple branches can be specified as usual in both forms.
// Different branches can't use different forms, it's either all normal or all ternary.
if <boolexpr> then <expr> else if <boolexpr> then <expr> else <expr>
if <boolexpr> { <statements> } else if <boolexpr> { <statements> } else { <statements> }

// If the last expression of every branch returns convertible types such that the result can be inferred,
// and if the branching is exhaustive, it is possible to treat branching as an expression itself.
// Otherwise branching evaluates to an empty tuple.
let f: F = if foo() then .A else .B
let f: F = if foo() { .A } else { .B }

// Boolean expressions when used directly in branch expressions can also include pattern matching.
if let .Some(v) = optional and v == 1 then print(v)

// This branching can happen in a guard without having to introduce a new block scope.
// The guard has to diverge control flow.
let .Some(v) = optional and v == 1 else return
```

## Loops

Loops are far more self explanatory than branching.

```
// The simple loop is written simply as loop.
loop <expr>
loop { <statements> }

// Unconditional loops return the Never type, unless they have a path which reaches a break statement.
// A naked break statement returns an empty tuple from the loop, as does any other situation where the loop ends.
// The simple loop allows breaking with a value which becomes the result of the entire loop expression.
let a = loop { break 0 }

// A while loop repeats while a condition is true.
while <boolexpr> do <expr>
while <boolexpr> { <statements> }

// While loops, much like branching, can have boolean expressions which contain patterns.
while let .Some(v) = iterator.next() do print(v)

// A repeat loop is much like a while loop but the check happens after each iteration not before.
repeat <expr> while <boolexpr>
repeat { statements } while <boolexpr>

// A for loop is sugar specific to the Sequence category. It chooses an appropriate implementation
// based on the binding style.
for element in sequence do <expr>
for element in sequence { <statements> }

// For loops can introduce additional filtering conditions to the element in question.
for element in sequence where element > 0 do <expr>

// For loop bindings can destructured like any other.
for (index, element) in sequence.enumerated() do <expr>
```

### For loop binding variants

For loops can iterate sequences by consuming, borrowing or aliasing the values based on the value binding.

```
// Consuming variant
for element in sequence do <expr>

// Consuming variant with mutable binding
for mut element in sequence do <expr>

// Borrowing variant
for &element in sequence do <expr>

// Aliasing variant
for mut &element in sequence do <expr>
```

### Break

Loops can be broken out of with `break`, `break :label`, `break <expr>` or `break :label <expr>`
If the statement has no label it breaks out of the most enclosing loop. Simple loops can break with an expression.

### Continue

The continue statement ends the current loop iteration early and continues to the next
unless it was the final iteration.

## Matching

The generic form of pattern matching is a match expression, and it looks like this:

```
v: T = match <expr> {
    <pattern> -> <expr -> T>
    <pattern> where <boolexpr> -> {
        <statements>
        <expr -> T>
    }
}
```
