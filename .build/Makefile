# This file describes how to compile the project, assuming you have the correct
# build-time dependencies installed. It is unlikely to work using anything except
# for `alpine:3.13.5`, as the official Clang documentation seems to contradict
# some of the flags used in this script, and following the documentation leads
# to compilation errors

# Directories
KERNEL_DIR := ./kernel
INCLUDE_DIR := ./include
BUILD_DIR := ./.build
OBJ_DIR := $(BUILD_DIR)/obj
OUT_DIR := $(BUILD_DIR)/out
DIRS := $(OBJ_DIR) $(OUT_DIR)

# Files
KERNEL_FILES := $(wildcard $(KERNEL_DIR)/*.c)
HEADER_FILES := $(wildcard $(INCLUDE_DIR)/*.h)
OBJ_FILES := $(patsubst $(KERNEL_DIR)/%.c,$(OBJ_DIR)/%.o,$(KERNEL_FILES))
FORMAT_FILES := $(OBJ_FILES:%.o=%.c.format) \
  $(patsubst $(INCLUDE_DIR)/%.h,$(OBJ_DIR)/%.h.format,$(HEADER_FILES))

FORMAT_STYLE := $(shell tr '\n' ',' < $(BUILD_DIR)/clang-format.yaml)

# Changing these flags sometimes seem to result in hard-to-understand compiler
# errors.
LDFLAGS := -nostdlib --script kernel/link.ld
CXXFLAGS := -c -Os --std=gnu17 -target x86_64-unknown-elf -Iinclude -MD        \
            -ffreestanding -fno-stack-protector -fmerge-all-constants -fPIC    \
            -mno-red-zone -nostdlib                                            \
            -Wall -Wextra -Werror -Wconversion                                 \
            -Wno-unused-function -Wno-gcc-compat

-include $(OBJ_FILES:%.o=%.d)

build: $(OUT_DIR)/kernel
	@# This silences make's "nothing to be done for target" message

.PHONY: clean
clean: ;
	@rm -rf $(OUT_DIR) $(OBJ_DIR)

format: $(FORMAT_FILES)
	@# This silences make's "nothing to be done for target" message

.PHONY: objdump
objdump: $(OUT_DIR)/dump
	@echo "----------------------------------------------------------"
	@echo "OBJDUMP: file written to $(OUT_DIR)/dump"
	@echo "----------------------------------------------------------"

$(OUT_DIR)/dump: $(OUT_DIR)/os.elf
	@llvm-objdump --arch=x86-64 -D $(OUT_DIR)/os.elf > $(OUT_DIR)/dump

$(OUT_DIR)/kernel: $(OUT_DIR)/os.elf $(OUT_DIR)/.dir
	@cp $(OUT_DIR)/os.elf $(OUT_DIR)/os
	@llvm-strip -s -K mmio -K fb -K bootboot -K environment -K initstack $(OUT_DIR)/os
	@mcopy -i /root/kernel $(OUT_DIR)/os ::/BOOTBOOT/INITRD && rm $(OUT_DIR)/os
	@mv /root/kernel $(OUT_DIR)/kernel # copies from image to mount
	@echo "compiled kernel"

$(OUT_DIR)/os.elf: $(OBJ_FILES) $(OUT_DIR)/.dir
	@ld.lld $(LDFLAGS) -o $(OUT_DIR)/os.elf $(OBJ_FILES)

$(OBJ_DIR)/%.c.format: $(KERNEL_DIR)/%.c $(BUILD_DIR)/clang-format.yaml $(OBJ_DIR)/.dir
	@clang-format -style='{$(FORMAT_STYLE)}' -i $<
	@touch $@
	@echo "formatted $<"

$(OBJ_DIR)/%.h.format: $(INCLUDE_DIR)/%.h $(BUILD_DIR)/clang-format.yaml $(OBJ_DIR)/.dir
	@clang-format -style='{$(FORMAT_STYLE)}' -i $<
	@touch $@
	@echo "formatted $<"

$(OBJ_DIR)/%.o: $(KERNEL_DIR)/%.c $(OBJ_DIR)/.dir
	@clang $(CXXFLAGS) -c -o $@ $<
	@echo "compiled $<"

$(DIRS:%=%/.dir):
	@mkdir -p $(@:%/.dir=%)
	@touch $@
