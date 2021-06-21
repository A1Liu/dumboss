FROM dumboss

# RUN llvm-objcopy -I binary -O elf64-x86-64 os os.elf
# RUN llvm-objdump --arch=x86-64 -D os.elf > dump

ENV TEMP_FLAGS='-S -Os -g --std=gnu17 -target x86_64-unknown-elf -ffreestanding \
        -mno-red-zone -nostdlib -fPIC -Iinclude -static \
        -Wall -Wextra -Werror -Wconversion'
RUN clang $TEMP_FLAGS src/*.c common/*.c
# RUN ld.lld --pie --script src/link.ld -o os ./*.o
# RUN llvm-objdump --arch=x86-64 -D os > dump

# RUN llvm-objcopy -I binary -O elf64-x86-64 BOOTX64.EFI boot.elf
# RUN llvm-objdump --arch=x86-64 -D boot.elf > dump

ENTRYPOINT ["cat", "page_tables.s"]
