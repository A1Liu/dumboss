FROM dumboss

RUN llvm-objcopy -I binary -O elf64-x86-64 os os.elf
RUN llvm-objdump --arch=x86-64 -D os.elf > dump

# RUN llvm-objcopy -I binary -O elf64-x86-64 BOOTX64.EFI boot.elf
# RUN llvm-objdump --arch=x86-64 -D boot.elf > dump

ENTRYPOINT ["cat", "dump"]
