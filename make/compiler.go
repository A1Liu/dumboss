package dumboss

import (
	"context"
	"fmt"
	"strings"
	"time"
)

var (
	// Changing these flags sometimes seem to result in hard-to-understand compiler
	// errors.
	ClangFlags = []string{
		"-c",
		"-Os",
		"--std=gnu17",
		"-target",
		"x86_64-unknown-elf",
		"-Iinclude",
		"-MD",

		"-ffreestanding",
		"-fno-stack-protector",
		"-fmerge-all-constants",
		"-fPIC",

		"-mno-red-zone",
		"-nostdlib",

		"-Wall",
		"-Wextra",
		"-Werror",
		"-Wconversion",

		"-Wno-unused-function",
		"-Wno-gcc-compat",
	}

	// Changing these flags sometimes seem to result in hard-to-understand compiler
	// errors.
	LdFlags = []string{
		"-nostdlib",
		"--script",
		"kernel/link.ld",
	}
)

func RunMakeTarget(ctx context.Context, target string) {
	cxxFlags := "CXXFLAGS=" + strings.Join(ClangFlags, " ")
	ldFlags := "LDFLAGS=" + strings.Join(LdFlags, " ")

	makeArgs := []string{"-f", ".build/Makefile", cxxFlags, ldFlags, target}

	begin := time.Now()
	RunImageCmd(ctx, "make", makeArgs)
	fmt.Printf("the target `%v` took %v seconds\n", target, time.Since(begin).Seconds())
}
