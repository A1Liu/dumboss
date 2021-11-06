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
		runQemu(ctx)
	case "clean":
		runClean()
	case "dump":
		runObjDump(ctx)
	case "test":
		runTest(ctx)
	default:
		begin := time.Now()
		runMakeTarget(ctx, os.Args[1])
		fmt.Printf("docker stuff took %v seconds\n\n", time.Now().Sub(begin).Seconds())
	}
}

func runTest(ctx context.Context) {
	desc1 := RuleDescriptor{Kind: NoneRuleKind, Target: "desc1"}
	desc2 := RuleDescriptor{Kind: NoneRuleKind, Target: "desc2"}
	desc3 := RuleDescriptor{Kind: NoneRuleKind, Target: "desc3"}
	desc4 := RuleDescriptor{Kind: NoneRuleKind, Target: "desc4"}
	desc5 := RuleDescriptor{Kind: NoneRuleKind, Target: "desc5"}
	desc6 := RuleDescriptor{Kind: NoneRuleKind, Target: "desc6"}
	desc7 := RuleDescriptor{Kind: NoneRuleKind, Target: "desc7"}
	desc8 := RuleDescriptor{Kind: NoneRuleKind, Target: "desc8"}

	run := func(ctx context.Context, target string) {
		fmt.Println("FUCKO:", target)
	}

	var engine Engine
	engine.AddRule(Rule{
		RuleDescriptor: desc1,
		TargetKind:     FileTargetKind,
		Dependencies:   []RuleDescriptor{desc2, desc3},
		Run:            run,
	})
	engine.AddRule(Rule{
		RuleDescriptor: desc2,
		TargetKind:     FileTargetKind,
		Dependencies:   []RuleDescriptor{desc4},
		Run:            run,
	})
	engine.AddRule(Rule{
		RuleDescriptor: desc3,
		TargetKind:     FileTargetKind,
		Dependencies:   []RuleDescriptor{desc5, desc6, desc7, desc8},
		Run:            run,
	})
	engine.AddRule(Rule{
		RuleDescriptor: desc4,
		TargetKind:     FileTargetKind,
		Dependencies:   []RuleDescriptor{},
		Run:            run,
	})
	engine.AddRule(Rule{
		RuleDescriptor: desc5,
		TargetKind:     FileTargetKind,
		Dependencies:   []RuleDescriptor{desc4},
		Run:            run,
	})
	engine.AddRule(Rule{
		RuleDescriptor: desc6,
		TargetKind:     FileTargetKind,
		Dependencies:   []RuleDescriptor{},
		Run:            run,
	})
	engine.AddRule(Rule{
		RuleDescriptor: desc7,
		TargetKind:     FileTargetKind,
		Dependencies:   []RuleDescriptor{},
		Run:            run,
	})
	engine.AddRule(Rule{
		RuleDescriptor: desc8,
		TargetKind:     FileTargetKind,
		Dependencies:   []RuleDescriptor{},
		Run:            run,
	})

	engine.Make(ctx, desc1.Kind, desc1.Target)
}

func runClean() {
	RunCmd("rm", []string{"-rf", CacheDir, OutDir, ObjDir})
}

func runObjDump(ctx context.Context) {
	elfPath := filepath.Join(OutDir, "os.elf")
	runMakeTarget(ctx, elfPath)

	RunImageCmd(ctx, "llvm-objdump", []string{"--arch=x86-64", "-D", elfPath})
}

func runQemu(ctx context.Context) {
	runMakeTarget(ctx, "build")

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
