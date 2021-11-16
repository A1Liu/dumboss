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
1. remap some stuff in the kernel to be less scary
2. add GDT stuff (pseudo-done?)
3. add real kernel panic stuff
4. update logging to use serial interrupts instead of like busy-waiting
5. add better support for multi-threading stuffs
   (https://wiki.osdev.org/Brendan%27s_Multi-tasking_Tutorial)
6. add file system (https://wiki.osdev.org/AHCI)
7. add support for running programs! (syscalls or something idk)
8. add modified round robin scheduling
9. some drivers baby! (https://wiki.osdev.org/Virtio)
0. hardware resource permissions? Capabilites?
1. lightweight thread creation and user-level scheduling?
2. process shared memory and simple thread sleeps and wakeups

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
8. Maybe never support dylibs.
9. Allocator is global source of truth for what memory is being used.

## Notes
1. No need for Linux zones memory is memory.

## Scrapped Ideas
1. Integer divide-by-zero always results in a zero. (THIS IS NOT POSSIBLE ON X64 FEELSBADMAN)

