package dumboss

import (
	"context"
	"io/ioutil"
	"os"
	"strings"

	. "a1liu.com/dumboss/make/engine"
	. "a1liu.com/dumboss/make/util"
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

func AddClangFile(path string) int {
	depPath := EscapeSourcePath(DepsDir, path, "dep")

	byteSlice, err := ioutil.ReadFile(depPath)
	if os.IsNotExist(err) {
	}
	CheckErr(err)

	return len(byteSlice)
}

func MakeClangRule(target string, depFiles ...string) Rule {
	Assert(strings.HasSuffix(target, ".o"))

	dependencies := make([]RuleDescriptor, len(depFiles))
	for index, element := range depFiles {
		dependencies[index].Kind = CompileRuleKind
		dependencies[index].Target = element
	}

	run := func(ctx context.Context, path string) {
		// flags := append([]string{}, ClangFlags..., "-MF", )
		RunImageCmd(ctx, "clang", ClangFlags)
	}

	return Rule{
		RuleDescriptor: RuleDescriptor{Kind: CompileRuleKind, Target: target},
		Dependencies:   dependencies,
		Run:            run,
	}
}
