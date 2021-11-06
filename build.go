package main

import (
	"context"
	"fmt"
	"os"
	"path/filepath"

	. "a1liu.com/dumboss/make"
)

func main() {
	if len(os.Args) < 2 {
		fmt.Println("expected 'run' subcommand, or a Makefile target")
		os.Exit(1)
	}

	ctx := context.Background()
	switch os.Args[1] {
	case "run":
		runQemu(ctx)
	case "clean":
		runClean()
	case "dump":
		runObjDump(ctx)
	default:
		RunMakeTarget(ctx, os.Args[1])
	}
}

func runClean() {
	RunCmd("rm", []string{"-rf", CacheDir, OutDir, ObjDir})
}

func runObjDump(ctx context.Context) {
	elfPath := filepath.Join(OutDir, "os.elf")
	RunMakeTarget(ctx, elfPath)

	RunImageCmd(ctx, "llvm-objdump", []string{"--arch=x86-64", "-D", elfPath})
}

func runQemu(ctx context.Context) {
	RunMakeTarget(ctx, "build")

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
