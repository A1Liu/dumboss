# Dumb Operating SyStem
I'm making a stupid operating system. The goal is to be compatible with nobody,
and to break as much existing code as possible.


## Building This Project
You must have Docker installed to build this project. Ideally you should also have
a C compiler installed.

#### Unix Systems
1. Build the kernel in a docker image

   ```
   docker build -f docker/build.Dockerfile -t dumboss .
   ```

2. Copy the file out of the image

   ```
   docker run --rm dumboss > kernel
   ```

3. Run the kernel in QEMU

   ```
   qemu-system-x86_64 -pflash OVMF.bin -serial stdio -hda kernel
   ```

## ToDo
1. add paging and whatnot.
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
8. Build system written in C that uses docker to build code, supports running code,
   etc.

