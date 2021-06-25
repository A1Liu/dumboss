# Dumb Operating SyStem
I'm making a stupid operating system. The goal is to be compatible with nobody,
and to break as much existing code as possible.


## Building This Project
You must have Docker and Go installed to build this project. To run this project
you also must have QEMU installed. Use `go run build.go build` to build the project,
and `go run build.go run` to build and then run it.

## ToDo
1. add page allocation.
2. add interrupt stuff and update logging to use it.
3. add support for atomics, as types, and use atomics in interrupt stuff.
4. add support for running (single threaded) programs!

## Ideas
1. 16-byte messages passed through `io_uring`-like interface
2. Larger messages passed through shared memory
3. Default program code is LLVM bitcode, certain included programs are flat binaries
   (LLVM bitcode is `.bc`, executables will probably be `.opq` for "opaque")
4. C support for async-await?
5. How lightweight can processes and threads be?
   [https://www.youtube.com/watch?v=KXuZi9aeGTw](https://www.youtube.com/watch?v=KXuZi9aeGTw)
6. Distinction between server-like and single-shot applications? Maybe not worth it,
   Can just build in fast syscall to check if process already exists
7. How much can be done in userspace drivers?

