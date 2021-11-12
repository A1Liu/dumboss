FROM dumboss/alpine
USER root
WORKDIR /root

# The `--no-cache` here makes the image smaller and also removes the need to first
# run `apk update`
RUN apk --no-cache add make llvm10 clang lld nasm mtools yq coreutils

RUN dd if=/dev/zero of=kernel bs=1k count=1440
RUN mformat -i kernel -f 1440 ::
RUN mmd -i kernel ::/EFI
RUN mmd -i kernel ::/EFI/BOOT
RUN mmd -i kernel ::/BOOTBOOT
COPY build/bootboot.efi ./bootboot.efi
RUN mcopy -i kernel bootboot.efi ::/EFI/BOOT/BOOTX64.EFI && rm ./bootboot.efi
