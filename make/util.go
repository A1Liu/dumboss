package dumboss

import (
	"io"
	"os"
	"os/exec"
	"path"
	"path/filepath"
	"runtime"
)

var (
	ProjectDir = func() string {
		_, filename, _, _ := runtime.Caller(0)
		abs, err := filepath.Abs(filename)
		CheckErr(err)

		return path.Dir(path.Dir(abs))
	}()
	BuildDir = filepath.Join(ProjectDir, ".build")
	CacheDir = filepath.Join(BuildDir, "cache")
)

func RunCmd(binary string, args []string) {
	cmd := exec.Command(binary, args...)

	stdout, err := cmd.StdoutPipe()
	CheckErr(err)
	stderr, err := cmd.StderrPipe()
	CheckErr(err)
	go io.Copy(os.Stdout, stdout)
	go io.Copy(os.Stderr, stderr)
	err = cmd.Run()
	CheckErr(err)
}

func CheckErr(err error) {
	if err != nil {
		panic(err.Error())
	}
}
