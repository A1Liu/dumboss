FROM alpine
USER root
WORKDIR /root/dumboss

RUN apk --no-cache add llvm10 clang lld nasm mtools

RUN dd if=/dev/zero of=kernel bs=1k count=1440
RUN mformat -i kernel -f 1440 ::
RUN mmd -i kernel ::/EFI
RUN mmd -i kernel ::/EFI/BOOT
RUN mmd -i kernel ::/BOOTBOOT
COPY ./bootboot.efi ./bootboot.efi
RUN mcopy -i kernel bootboot.efi ::/EFI/BOOT/BOOTX64.EFI

ENV CFLAGS='-c -Os --std=gnu17 -target x86_64-unknown-elf -ffreestanding \
        -mno-red-zone -fno-stack-protector -nostdlib -fPIC -Iinclude \
        -Wall -Wextra -Werror -Wconversion'

COPY ./include ./include
COPY ./common ./common
COPY ./src ./src
RUN clang $CFLAGS src/*.c common/*.c
RUN ld.lld -nostdlib --script src/link.ld -o os.elf ./*.o
RUN llvm-strip -s -K mmio -K fb -K bootboot -K environment -K initstack os.elf

RUN mcopy -i kernel os.elf ::/BOOTBOOT/INITRD

ENTRYPOINT ["cat", "kernel"]
