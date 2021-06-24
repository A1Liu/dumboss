FROM alpine
USER root
WORKDIR /root/dumboss

RUN apk update
RUN apk add llvm10 clang lld nasm mtools binutils \
        musl-dev compiler-rt compiler-rt-static

COPY ./include ./include
COPY ./common ./common
COPY ./build ./build
RUN clang -O2 --std=gnu17 -o dumboss-build -fuse-ld=lld --rtlib=compiler-rt \
        -Iinclude ./common/*.c ./build/*.c \
        -Wall -Wextra -Werror -Wconversion

ENTRYPOINT ["./dumboss-build"]
