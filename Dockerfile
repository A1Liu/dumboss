FROM alpine:3.13.5

USER root
WORKDIR /root/dumboss

RUN apk update
RUN apk add clang lld nasm mtools

# COPY ./bootloader.asm ./bootloader.asm
# RUN nasm -f elf32 -o boot.o bootloader.asm

ENV CFLAGS='-c -Os --std=gnu17 -target x86_64-unknown-windows -ffreestanding \
        -fshort-wchar -mno-red-zone \
        -nostdlib -Wall -Wextra -Werror -Wconversion \
        -Iinclude \
        -Ignu-efi/inc -Ignu-efi/inc/x86_64 -Ignu-efi/inc/protocol'

ENV LDFLAGS='-target x86_64-unknown-windows -nostdlib -Wl,-entry:efi_main \
        -Wl,-subsystem:efi_application -fuse-ld=lld-link'

COPY ./gnu-efi/inc ./gnu-efi/inc
COPY ./gnu-efi/lib/data.c ./gnu-efi/lib/data.c
RUN clang $CFLAGS -o data.o ./gnu-efi/lib/data.c

COPY ./include ./include
COPY ./src ./src
RUN clang $CFLAGS src/*.c

RUN clang $LDFLAGS -o BOOTX64.EFI ./*.o data.o

RUN dd if=/dev/zero of=kernel bs=1k count=1440
RUN mformat -i kernel -f 1440 ::
RUN mmd -i kernel ::/EFI
RUN mmd -i kernel ::/EFI/BOOT
RUN mcopy -i kernel BOOTX64.EFI ::/EFI/BOOT

ENTRYPOINT ["cat", "kernel"]
