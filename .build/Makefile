# This file describes how to compile the project, assuming you have the correct
# build-time dependencies installed. It is unlikely to work using anything except
# for `alpine:3.13.5`, as the official Clang documentation seems to contradict
# some of the flags used in this script, and following the documentation leads
# to compilation errors. The expected way to run it is using the go build script,
# which internally runs this makefile using the `.build/Dockerfile` dockerfile
#                                    - Albert Liu, Nov 11, 2021 Thu 15:31 EST

# Directories. These are created by the go script.
PROJECT_DIR := $(realpath .)
BUILD_DIR := ./.build
LIB_DIR := ./lib

DEPS_DIR := $(BUILD_DIR)/deps
CACHE_DIR := $(BUILD_DIR)/cache
OBJ_DIR := $(BUILD_DIR)/obj
OUT_DIR := $(BUILD_DIR)/out

# Functions
simple_path = $(shell realpath --relative-base=$(PROJECT_DIR) "$(1)")
output_path = $(call simple_path,$(2))/$(subst /,.,C$(call simple_path,$(1)))$(3)
obj_path = $(call output_path,$(1),$(OBJ_DIR),.o)
dep_path = $(call output_path,$(1),$(DEPS_DIR),.dep)

# Changing these flags sometimes seem to result in hard-to-understand compiler
# errors.
CFLAGS := -c --std=gnu17 -target x86_64-unknown-elf -MD                        \
          -ffreestanding -fno-stack-protector -fmerge-all-constants -fPIC      \
          -mno-red-zone -nostdlib -isystem./lib                                \
          -Wall -Wextra -Werror -Wconversion                                   \
          -Wno-unused-function -Wno-gcc-compat

# Kernel
KERNEL_DIR := ./kern
KERNEL_FILES := $(wildcard $(KERNEL_DIR)/*.c)
KERNEL_TMP_PREFIX := $(call output_path,$(KERNEL_DIR),$(OBJ_DIR))
KERNEL_OBJS := $(foreach kern_tmp,$(KERNEL_FILES),$(call obj_path,$(kern_tmp)))

-include $(foreach kern_tmp,$(KERNEL_FILES),$(call dep_path,$(kern_tmp)))

.PHONY: kern
kern: $(OUT_DIR)/kernel
	@# This silences make's "nothing to be done for target" message

$(OUT_DIR)/kernel: $(OUT_DIR)/os.elf
	@cp $(OUT_DIR)/os.elf $(OUT_DIR)/os
	@llvm-strip -s -K mmio -K fb -K bootboot -K environment -K initstack $(OUT_DIR)/os
	@mcopy -i /root/kernel $(OUT_DIR)/os ::/BOOTBOOT/INITRD && rm $(OUT_DIR)/os
	@mv /root/kernel $(OUT_DIR)/kernel # copies from image to mount
	@echo 'finished building kernel'

$(OUT_DIR)/os.elf: $(KERNEL_OBJS)
	@ld.lld -nostdlib --script $(KERNEL_DIR)/link.ld -o $(OUT_DIR)/os.elf $(KERNEL_OBJS)
	@echo 'finished linking $@'

$(KERNEL_TMP_PREFIX).%.c.o: $(KERNEL_DIR)/%.c
	@clang -Os -iquote$(KERNEL_DIR)/include $(CFLAGS) -MF $(call dep_path,$<) -o $@ $<
	@echo 'compiled $<'

# HEADER_FILES := $(wildcard $(INCLUDE_DIR)/*.h)
# FORMAT_FILES := $(OBJ_FILES:%.o=%.c.format) \
#                 $(patsubst $(INCLUDE_DIR)/%.h,$(OBJ_DIR)/%.h.format,$(HEADER_FILES))
#
# FORMAT_STYLE := $(shell tr '\n' ',' < $(BUILD_DIR)/clang-format.yaml)

# .PHONY: format
# format: $(FORMAT_FILES)
# 	@# This silences make's "nothing to be done for target" message

# $(OBJ_DIR)/%.c.format: $(KERNEL_DIR)/%.c $(BUILD_DIR)/clang-format.yaml
# 	@clang-format -style='{$(FORMAT_STYLE)}' -i $<
# 	@touch $@
# 	@echo "formatted $<"

# $(OBJ_DIR)/%.h.format: $(INCLUDE_DIR)/%.h $(BUILD_DIR)/clang-format.yaml
# 	@clang-format -style='{$(FORMAT_STYLE)}' -i $<
# 	@touch $@
# 	@echo "formatted $<"
