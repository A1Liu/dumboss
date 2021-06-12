# Dumb Operating SyStem
I'm making a stupid operating system. The goal is to be compatible with nobody,
and to break as much existing code as possible.

## Commands

Unix Systems
```
# Build the file
docker build -t dumboss .

# Copy the file out of the container
docker run --rm dumboss > kernel

# Run the kernel
qemu-system-x86_64 -serial stdio -fda kernel
```

## ToDo
1. switch to UEFI and remove this "bootloader" nonsense.
2. make a primitive form of logging, with some macros and stuff to make things
   easier and safer to write.
3. add interrupt stuff and update logging to use it.
4. add paging and whatnot.
5. add support for atomics, as types, and use atomics in interrupt stuff.
6. add support for running programs!

