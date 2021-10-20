# Dumb Operating SyStem
I'm making a stupid operating system. The goal is to be compatible with nobody,
and to break as much existing code as possible.

## Building This Project
You must have Docker and Go installed to build this project. To run this project
you also must have QEMU installed.

Use `go run build.go build` to build the project, and `go run build.go run` to
build and then run it.

You can also use `go install` to create a build script called `dumboss` that works
in the same way (`dumboss build` to build and `dumboss run` to run)

## ToDo
1. add GDT stuff
2. Update logging to use serial interrupts instead of like busy-waiting
3. add paging, make alloc return Buffer
4. add support for running programs! (syscalls or something idk)
5. add file system (https://wiki.osdev.org/AHCI)
6. Modified round robin scheduling
7. Some drivers baby! (https://wiki.osdev.org/Virtio)
8. Hardware resource permissions? Capabilites?
9. Lightweight thread creation and user-level scheduling?
0. Process shared memory and simple thread sleeps and wakeups

## Ideas
1. ?-byte messages passed through `io_uring`-like interface
2. Larger messages passed through shared memory
3. Default program code is LLVM bitcode, certain included programs are flat binaries
   (LLVM bitcode is `.bc`, executables will probably be `.opq` for "opaque")
4. C support for async-await?
5. How lightweight can processes and threads be?
   [https://www.youtube.com/watch?v=KXuZi9aeGTw](https://www.youtube.com/watch?v=KXuZi9aeGTw)
6. Microkernel with monolithic kernel manager thingy in userspace.
7. Switch to C++ because honestly some light template use would probably help
   with correctness.
8. Maybe don't map all of phyiscal memory lmao
9. No file-system. Just a k-v store, 128-bit key that maps to a set of blocks on the disk.
10. Permissions are based on programs, not users.

