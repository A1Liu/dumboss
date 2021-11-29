package main

import (
	"context"
	"io/fs"
	"os"
	"path/filepath"
	"strings"

	. "a1liu.com/dumboss/util"
)

func main() {
	if len(os.Args) < 2 {
		print("expected 'run' subcommand, or a Makefile target\n")
		os.Exit(1)
	}

	config := readConfig()

	ctx := context.Background()
	switch os.Args[1] {
	case "run":
		runMakeTarget(ctx, "build")
		runQemu(ctx, config.QemuArgs)
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

func runQemu(ctx context.Context, params []string) {
	// TODO In 20 years when this OS finally has a GUI, we'll need this to make
	// serial write to stdout again: "-serial", "stdio",
	// TODO We should be able to modify these parameters without editing the build script
	args := append(params, "-bios", filepath.Join(BuildDir, "OVMF.bin"),
		"-drive", "file="+filepath.Join(DotBuildDir, "out", "kernel")+",format=raw",
		"-D", filepath.Join(DotBuildDir, "out", "qemu-logs.txt"))

	RunCmd("qemu-system-x86_64", args)
}

func runMakeTarget(ctx context.Context, target string) {
	for _, dir := range []string{OutDir, CacheDir, DepsDir, ObjDir} {
		err := os.MkdirAll(dir, fs.ModeDir|fs.ModePerm)
		CheckErr(err)
	}

	makeArgs := []string{"-f", filepath.Join(ProjectDir, "Makefile"), target}
	RunImageCmd(ctx, "make", makeArgs)
}

type Config struct {
	QemuArgs []string
}

func readConfig() Config {
	data, err := os.ReadFile(filepath.Join(BuildDir, "config.txt"))
	CheckErr(err)

	var config Config
	config.QemuArgs = make([]string, 10)[:0]

	lines := strings.Split(string(data), "\n")
	for _, line := range lines {
		if len(line) == 0 {
			continue
		}

		lineParts := strings.SplitN(line, "=", 2)
		config.QemuArgs = append(config.QemuArgs, lineParts...)
	}

	return config
}
