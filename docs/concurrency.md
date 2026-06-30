# Generalizing continuations for concurrency.

Somehow structural tuple-like capture pairs give us easy to do CPS??

Async becomes trivial sugar for continuations thanks to concrete capture signatures.

```str
/// Sugar.
fun foo() async {
    let x: Int = 1

    let thing: Int = await wait_for_thing()

    print(x)
    print(y)
}

fun foo(continuation: () -> ()) {
    let x: Int = 1

    wait_for_thing(continuation: |thing: Int|:[continuation, x] {
        print(x)
        print(y)

        continuation()
    })
}
```

To support arguments to awaitable functions we use variadic packs.

```str
pub category ContinuationExistential<returning: Return> {
    implicit init(erasing continuation: (Return) -> ())

    fun resume(self, returning value: Return)

    fun resume(self) where T == () { self.resume(returning: ()) }
}

pub struct StaticContinuationExistential<returning: Return>: ContinuationExistential<returning: Return> {
    let invoke: (Return):[] -> ()
    let deinitialize: ():[] -> ()

    pub implicit init(erasing continuation: (Return) -> ()) {
        if not ::non_reentrant then panic(because: "the static context requires provable non-reentrance")

        let storage = Pointer<to: C>(segment: "RAM")

        storage.initialize(to: continuation)

        self.invoke = |r| unsafe storage->(r)
        self.deinitializer = || unsafe storage.deinitialize()
    }

    deinit {
        self.deinitialize()
    }

    pub fun resume(self, returning value: Return) {
        self.invoke(value)
    }
}
```
