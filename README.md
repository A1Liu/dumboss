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
1. remap some stuff in the kernel to be less scary (meh do it later i guess)
2. add real kernel panic stuff
3. update logging to use serial interrupts instead of like busy-waiting
4. add better support for multi-threading stuffs
   (https://wiki.osdev.org/Brendan%27s_Multi-tasking_Tutorial)
5. add file system (https://wiki.osdev.org/AHCI)
6. add support for running programs! (syscalls or something idk)
7. add modified round robin scheduling
8. some drivers baby! (https://wiki.osdev.org/Virtio)
9. hardware resource permissions? Capabilites?
0. lightweight thread creation and user-level scheduling?
1. process shared memory and simple thread sleeps and wakeups

## Ideas
1. ?-byte messages passed through `io_uring`-like interface
2. Larger messages passed through shared memory
3. How lightweight can processes and threads be?
   [https://www.youtube.com/watch?v=KXuZi9aeGTw](https://www.youtube.com/watch?v=KXuZi9aeGTw)
4. Microkernel with monolithic kernel manager thingy in userspace.
5. Maybe don't map all of physical memory lmao
6. Just do Ext but without directories and filenames. Programs ask for raw inodes.
   Then, we can add a filesystem thingy on top if we want.
7. Permissions are based (at least partly) on programs.
8. Maybe never support dylibs in the kernel.
9. Allocator is global source of truth for what memory is being used.

## Notes
1. No need for Linux zones, memory is memory.
2. No async-await stuff. Tasks are just functions with associated state.

## Scrapped Ideas
1. Integer divide-by-zero always results in a zero. (THIS IS NOT POSSIBLE ON X64 FEELSBADMAN)

