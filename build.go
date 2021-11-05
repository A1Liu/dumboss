package main

// This code is a wrapper around Docker functionality, in the hopes that it won't
// be that much of a pain to set up on windows. We will have to see though. It's
// been cobbled together using a random assortment of blogs and auto-generated
// Docker documentation.

import (
	"context"
	"fmt"
	"os"
	"path/filepath"
	"time"

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
	default:
		runMakeCmd(ctx, os.Args[1])
	}
}

func compileTarget(ctx context.Context, target string) int {
	begin := time.Now()
	cmdResult := RunImageCmd(ctx, "make", []string{"-f", ".build/Makefile", target})
	fmt.Printf("the target `%v` took %v seconds\n", target, time.Since(begin).Seconds())
	return cmdResult
}

func runQemu(ctx context.Context) {
	commandStatus := compileTarget(ctx, "build")
	if commandStatus != 0 {
		os.Exit(commandStatus)
	}

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

func runMakeCmd(ctx context.Context, target string) {
	commandStatus := compileTarget(ctx, target)

	os.Exit(commandStatus)
}
