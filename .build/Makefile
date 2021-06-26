SRC_DIR := ./src
BUILD_DIR := ./.build
OBJ_DIR := $(BUILD_DIR)/obj
OUT_DIR := $(BUILD_DIR)/out
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))
LDFLAGS := -nostdlib --script src/link.ld
CXXFLAGS := -c -Os --std=gnu17 -target x86_64-unknown-elf -Iinclude -MD        \
            -ffreestanding -fno-stack-protector -fmerge-all-constants -fPIC    \
            -mno-red-zone -nostdlib                                            \
            -Wall -Wextra -Werror -Wconversion

-include $(OBJ_FILES:%.o=%.d)

kernel: $(OUT_DIR)/kernel

clean:
	rm -rf $(OUT_DIR) $(OBJ_DIR)

$(OUT_DIR)/kernel: $(OBJ_FILES)
	ld.lld $(LDFLAGS) -o $(OUT_DIR)/os.elf $^
	llvm-strip -s -K mmio -K fb -K bootboot -K environment -K initstack $(OUT_DIR)/os.elf
	mcopy -i /root/kernel $(OUT_DIR)/os.elf ::/BOOTBOOT/INITRD
	@# copies from image to mount
	mv /root/kernel $(OUT_DIR)/kernel

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(OBJ_DIR)/.gitkeep $(OUT_DIR)/.gitkeep
	clang $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/.gitkeep:
	mkdir -p $(OBJ_DIR)
	touch $(OBJ_DIR)/.gitkeep

$(OUT_DIR)/.gitkeep:
	mkdir -p $(OUT_DIR)
	touch $(OUT_DIR)/.gitkeep

