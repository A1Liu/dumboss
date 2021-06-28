# This file describes how to compile the project, assuming you have the correct
# build-time dependencies installed. It is unlikely to work using anything except
# for `alpine:3.13.5`, as the official Clang documentation seems to contradict
# some of the flags used in this script, and following the documentation leads
# to compilation errors

# Directories
SRC_DIR := ./src
BUILD_DIR := ./.build
OBJ_DIR := $(BUILD_DIR)/obj
OUT_DIR := $(BUILD_DIR)/out
DIRS := $(OBJ_DIR) $(OUT_DIR)

# Files
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

# Changing these flags sometimes seem to result in hard-to-understand compiler
# errors.
FORMAT_STYLE := $(shell tr '\n' ',' < $(BUILD_DIR)/clang-format.yaml)
LDFLAGS := -nostdlib --script src/link.ld
CXXFLAGS := -c -Os --std=gnu17 -target x86_64-unknown-elf -Iinclude -MD        \
            -ffreestanding -fno-stack-protector -fmerge-all-constants -fPIC    \
            -mno-red-zone -nostdlib                                            \
            -Wall -Wextra -Werror -Wconversion

-include $(OBJ_FILES:%.o=%.d)

build: $(OUT_DIR)/kernel

clean: ;
	rm -rf $(OUT_DIR) $(OBJ_DIR)

format: $(OBJ_FILES:%.o=%.format)

$(OUT_DIR)/kernel: $(OBJ_FILES) $(OUT_DIR)/.dir
	ld.lld $(LDFLAGS) -o $(OUT_DIR)/os.elf $(OBJ_FILES)
	llvm-strip -s -K mmio -K fb -K bootboot -K environment -K initstack $(OUT_DIR)/os.elf
	mcopy -i /root/kernel $(OUT_DIR)/os.elf ::/BOOTBOOT/INITRD
	@# copies from image to mount
	mv /root/kernel $(OUT_DIR)/kernel

$(OBJ_DIR)/%.format: $(SRC_DIR)/%.c $(OBJ_DIR)/.dir
	clang-format -style='{$(FORMAT_STYLE)}' -i $<
	touch $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(OBJ_DIR)/.dir
	clang $(CXXFLAGS) -c -o $@ $<

$(DIRS:%=%/.dir):
	mkdir -p $(@:%/.dir=%)
	touch $@

