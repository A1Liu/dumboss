# Dumb Operating SyStem
I'm making a stupid operating system. The goal is to be compatible with nobody,
and to break as much existing code as possible.

## Commands

Unix Systems
```
# Build the file
docker build -t dumboss .

# Copy the file out of the container
docker run --rm dumboss > build/kernel

# Run the kernel
qemu-system-x86_64 -fda build/kernel
```


