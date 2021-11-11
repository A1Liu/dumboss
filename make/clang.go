package main

import (
	"context"
	"fmt"
	"io/fs"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"

	. "a1liu.com/dumboss/engine"
	. "a1liu.com/dumboss/make"
	. "a1liu.com/dumboss/util"
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
)

func addKernelRule(eng *Engine) RuleDescriptor {
	osElfDescriptor := addOsElfRule(eng)

	tempPath := filepath.Join(OutDir, "os")
	outputPath := filepath.Join(OutDir, "kernel")
	run := func(ctx context.Context, target string) {
		RunCmd("cp", []string{osElfDescriptor.Target, tempPath})
		stripArgs := []string{"-K", "fb", "-K", "bootboot", "-K", "environment", "-K", "initstack", tempPath}
		RunImageCmd(ctx, "llvm-strip", stripArgs)
		RunImageCmd(ctx, "cp", []string{"/root/kernel", outputPath})
		mcopyArgs := []string{"-i", outputPath, tempPath, "::/BOOTBOOT/INITRD"}
		RunImageCmd(ctx, "mcopy", mcopyArgs)
		fmt.Printf("Compiled kernel\n")
	}

	rule := Rule{
		RuleDescriptor: RuleDescriptor{Kind: CompileRuleKind, Target: outputPath},
		TargetKind:     FileTargetKind,
		Dependencies:   []RuleDescriptor{osElfDescriptor},
		Run:            run,
	}

	eng.AddRule(rule)
	return rule.RuleDescriptor
}

func addOsElfRule(eng *Engine) RuleDescriptor {
	err := os.MkdirAll(OutDir, fs.ModeDir|fs.ModePerm)
	CheckErr(err)

	outputPath := filepath.Join(OutDir, "os.elf")

	deps := make([]RuleDescriptor, 10)[:0]
	flags := []string{"-o", outputPath, "-T", "kernel/link.ld", "-nostdlib"}
	for _, file := range DirWalk(KernelDir) {
		if !strings.HasSuffix(file, ".c") || strings.HasPrefix(file, "/") {
			continue
		}

		childDescriptor := AddClangRule(eng, file)
		deps = append(deps, childDescriptor)
		flags = append(flags, childDescriptor.Target)
	}

	run := func(ctx context.Context, target string) {
		RunImageCmd(ctx, "ld.lld", flags)
		fmt.Printf("finished link step\n")
	}

	outputDescriptor := RuleDescriptor{Kind: CompileRuleKind, Target: outputPath}
	rule := Rule{
		RuleDescriptor: outputDescriptor,
		Dependencies:   deps,
		TargetKind:     FileTargetKind,
		Run:            run,
	}

	eng.AddRule(rule)
	return rule.RuleDescriptor
}

func AddClangRule(eng *Engine, sourcePath string) RuleDescriptor {
	rule := ClangRule(sourcePath, ClangFlags...)
	eng.AddRule(rule)
	return rule.RuleDescriptor
}

func AddMakeRule(eng *Engine, sourcePath string) RuleDescriptor {
	return RuleDescriptor{}
}

func ClangRule(sourcePath string, compilationFlags ...string) Rule {
	depPath := EscapeSourcePath(DepsDir, sourcePath, "dep")
	targetPath := EscapeSourcePath(ObjDir, sourcePath, "o")

	err := os.MkdirAll(DepsDir, fs.ModeDir|fs.ModePerm)
	CheckErr(err)
	err = os.MkdirAll(ObjDir, fs.ModeDir|fs.ModePerm)
	CheckErr(err)

	sourceDesc := RuleDescriptor{Kind: CompileRuleKind, Target: sourcePath}
	depDesc := RuleDescriptor{Kind: CompileRuleKind, Target: depPath}
	targetDesc := RuleDescriptor{Kind: CompileRuleKind, Target: targetPath}

	run := func(ctx context.Context, target string) {
		flags := append([]string{"-MF", depPath, "-o", targetPath, sourcePath}, compilationFlags...)
		RunCmd("clang", flags)
		fmt.Printf("Finished compiling `%v`\n", sourcePath)
	}

	rule := Rule{
		RuleDescriptor: targetDesc,
		TargetKind:     FileTargetKind,
		Run:            run,
	}

	fileBytes, err := ioutil.ReadFile(depPath)
	if os.IsNotExist(err) {
		rule.Dependencies = []RuleDescriptor{sourceDesc, depDesc}
		return rule
	} else {
		CheckErr(err)
	}

	deps := make([]RuleDescriptor, 10)[:0]
	for _, depPath := range ParseMakerule(fileBytes).Deps {
		if depPath[0] == '/' {
			continue
		}

		newDep := RuleDescriptor{Kind: CompileRuleKind, Target: string(depPath)}
		deps = append(deps, newDep)
	}

	rule.Dependencies = deps
	return rule
}
