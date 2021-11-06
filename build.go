package main

import (
	"context"
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"time"

	. "a1liu.com/dumboss/make"
	. "a1liu.com/dumboss/make/engine"
	. "a1liu.com/dumboss/make/util"
)

func main() {
	if len(os.Args) < 2 {
		fmt.Println("expected 'run' subcommand, or a Makefile target")
		os.Exit(1)
	}

	ctx := context.Background()
	switch os.Args[1] {
	case "run":
		runMakeTarget(ctx, "build")
		runQemu(ctx)
	case "build":
		runBuild(ctx)
	case "clean":
		runClean()
	case "dump":
		runObjDump(ctx)
	case "make":
		runMakeTarget(ctx, os.Args[2])
	default:
		runMakeTarget(ctx, os.Args[1])
	}
}

func runBuild(ctx context.Context) {
	begin := time.Now()
	var engine Engine

	kernelDesc := AddKernelRule(&engine)
	fmt.Printf("engine setup took %v seconds\n", time.Now().Sub(begin).Seconds())

	engine.Make(ctx, kernelDesc)
	fmt.Printf("compilation took %v seconds\n", time.Now().Sub(begin).Seconds())
}

func runClean() {
	RunCmd("rm", []string{"-rf", CacheDir, OutDir, ObjDir, DepsDir})
}

func runObjDump(ctx context.Context) {
	elfPath := filepath.Join(OutDir, "os.elf")
	runMakeTarget(ctx, elfPath)

	RunImageCmd(ctx, "llvm-objdump", []string{"--arch=x86-64", "-D", elfPath})
}

func runQemu(ctx context.Context) {
	// TODO In 20 years when this OS finally has a GUI, we'll need this to make
	// serial write to stdout again: "-serial", "stdio",
	// TODO We should be able to modify these parameters without editing the build script
	args := []string{"-bios", filepath.Join(BuildDir, "OVMF.bin"),
		"-drive", "file=" + filepath.Join(BuildDir, "out", "kernel") + ",format=raw",
		"-D", filepath.Join(BuildDir, "out", "qemu-logs.txt"),
		"-d", "cpu_reset,int",
		"-smp", "4", "-no-reboot", "-nographic"}
	RunCmd("qemu-system-x86_64", args)
}

func runMakeTarget(ctx context.Context, target string) {
	cxxFlags := "CXXFLAGS=" + strings.Join(ClangFlags, " ")
	ldFlags := "LDFLAGS=" + strings.Join(LdFlags, " ")

	makeArgs := []string{"-f", ".build/Makefile", cxxFlags, ldFlags, target}

	begin := time.Now()
	RunImageCmd(ctx, "make", makeArgs)
	fmt.Printf("the target `%v` took %v seconds\n", target, time.Since(begin).Seconds())
}
