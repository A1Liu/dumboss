# Dumb Operating SyStem
I'm making a stupid operating system. The goal is to be compatible with nobody,
and to break as much existing code as possible.

## Commands

#### Unix Systems
1. Build the kernel in a docker image

   ```
   docker build -t dumboss -f docker/Dockerfile.build .
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
1. Update system to have real bootloader that loads the kernel
2. add interrupt stuff and update logging to use it.
3. add paging and whatnot.
4. add support for atomics, as types, and use atomics in interrupt stuff.
5. add support for running (single threaded) programs!

