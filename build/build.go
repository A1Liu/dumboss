package main

import (
	"context"
	"fmt"
	"io/fs"
	"os"
	"path/filepath"
	"time"

	. "a1liu.com/dumboss/util"
)

func main() {
	if len(os.Args) < 2 {
		fmt.Println("expected 'run' subcommand, or a Makefile target")
		os.Exit(1)
	}

	ctx := context.Background()
	switch os.Args[1] {
	case "run":
		runMakeTarget(ctx, "kern")
		runQemu(ctx)
	case "clean":
		runClean()
	case "make":
		runMakeTarget(ctx, os.Args[2])
	default:
		runMakeTarget(ctx, os.Args[1])
	}
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
		"-drive", "file=" + filepath.Join(DotBuildDir, "out", "kernel") + ",format=raw",
		"-D", filepath.Join(DotBuildDir, "out", "qemu-logs.txt"),
		"-d", "cpu_reset,int",
		"-smp", "4", "-no-reboot", "-nographic"}
	RunCmd("qemu-system-x86_64", args)
}

func runMakeTarget(ctx context.Context, target string) {
	err := os.MkdirAll(OutDir, fs.ModeDir|fs.ModePerm)
	CheckErr(err)
	err = os.MkdirAll(CacheDir, fs.ModeDir|fs.ModePerm)
	CheckErr(err)
	err = os.MkdirAll(DepsDir, fs.ModeDir|fs.ModePerm)
	CheckErr(err)
	err = os.MkdirAll(ObjDir, fs.ModeDir|fs.ModePerm)
	CheckErr(err)

	makeArgs := []string{"-f", filepath.Join(ProjectDir, "Makefile"), target}
	begin := time.Now()
	RunImageCmdSingle(ctx, "make", makeArgs)
	fmt.Printf("the target `%v` took %v seconds\n", target, time.Since(begin).Seconds())
}
