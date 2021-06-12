FROM alpine:latest

USER root
WORKDIR /root/dumboss

RUN apk update
RUN apk add clang lld nasm

COPY ./bootloader.asm ./bootloader.asm
RUN nasm -f elf32 -o boot.o bootloader.asm

COPY ./src ./src
RUN clang --target=i386-unknown-unknown-elf ./src/*.c -c -o kernel.o \
        -nostdlib -ffreestanding -Wall -Wextra -Werror

COPY ./link.ld ./link.ld
RUN ld.lld --oformat binary boot.o kernel.o -v --script link.ld -o kernel

ENTRYPOINT ["cat", "kernel"]
