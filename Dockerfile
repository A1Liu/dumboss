FROM alpine:latest

USER root
WORKDIR /root/dumboss

RUN apk update
RUN apk add clang lld nasm

COPY ./bootloader.asm ./bootloader.asm
RUN nasm -f elf32 -o boot.o bootloader.asm

COPY ./include ./include
COPY ./src ./src
RUN clang ./src/*.c -c \
        --target=i386-unknown-unknown-elf --include-directory=/root/dumboss/include \
        -nostdlib -ffreestanding -Wall -Wextra -Werror -Wconversion

COPY ./link.ld ./link.ld
RUN ld.lld --oformat binary ./*.o -v --script link.ld -o kernel

ENTRYPOINT ["cat", "kernel"]
