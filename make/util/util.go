package util

import (
	"fmt"
	"io"
	"io/fs"
	"os"
	"os/exec"
	"path"
	"path/filepath"
	"runtime"
	"strings"
	"time"

	dirwalk "github.com/karrick/godirwalk"
)

var (
	ProjectDir = func() string {
		_, filename, _, ok := runtime.Caller(0)
		Assert(ok)
		abs, err := filepath.Abs(filename)
		CheckErr(err)

		return path.Dir(path.Dir(path.Dir(abs)))
	}()

	UserDir    = filepath.Join(ProjectDir, "user")
	LibDir     = filepath.Join(ProjectDir, "lib")
	KernelDir  = filepath.Join(ProjectDir, "kernel")
	IncludeDir = filepath.Join(ProjectDir, "include")
	BuildDir   = filepath.Join(ProjectDir, ".build")

	DepsDir  = filepath.Join(BuildDir, "deps")  // dep files
	CacheDir = filepath.Join(BuildDir, "cache") // cache flag files
	ObjDir   = filepath.Join(BuildDir, "obj")   // intermediate files
	OutDir   = filepath.Join(BuildDir, "out")   // output files
)

func DirWalk(dirname string) []string {
	fileNames := make([]string, 10)

	walker := func(osPathname string, de *dirwalk.Dirent) error {
		if de.IsRegular() {
			fileNames = append(fileNames, osPathname)
			fmt.Printf("%s %s\n", de.ModeType(), osPathname)
		}

		return nil
	}

	options := dirwalk.Options{
		FollowSymbolicLinks: false,
		Unsorted:            false,
		Callback:            walker,
	}

	err := dirwalk.Walk(dirname, &options)
	CheckErr(err)

	return fileNames
}

func EscapeSourcePath(targetDir, filePath string, extras ...string) string {
	Assert(filepath.IsAbs(targetDir))

	abs, err := filepath.Abs(filePath)
	CheckErr(err)
	Assert(!strings.HasPrefix(abs, targetDir))

	rel, err := filepath.Rel(ProjectDir, abs)
	CheckErr(err)

	pathSep := string(os.PathSeparator)
	cachePathName := strings.Join(append([]string{"C" + rel}, extras...), pathSep)
	cachePathName = strings.Replace(cachePathName, pathSep, ".", -1)

	return filepath.Join(targetDir, cachePathName)
}

func CacheIsValid(filePath string, extras ...string) bool {
	_, callerPath, callerLine, ok := runtime.Caller(1)
	Assert(ok)
	callerRelPath, err := filepath.Rel(ProjectDir, callerPath)
	CheckErr(err)

	passThrough := append([]string{callerRelPath, string(callerLine)}, extras...)
	return CacheIsValidTyped(filePath, passThrough...)
}

func CacheIsValidTyped(filePath string, extras ...string) bool {
	cachePath := EscapeSourcePath(CacheDir, filePath, extras...)

	pathStat, err := os.Stat(filePath)
	if os.IsNotExist(err) {
		return false
	}
	CheckErr(err)

	cacheFileStat, err := os.Stat(cachePath)
	if os.IsNotExist(err) {
		err = os.MkdirAll(CacheDir, fs.ModeDir|fs.ModePerm)
		CheckErr(err)
		file, err := os.Create(cachePath)
		CheckErr(err)
		file.Close()
		return false
	}
	CheckErr(err)

	cacheModTime, pathModTime := cacheFileStat.ModTime(), pathStat.ModTime()

	currentTime := time.Now().Local()
	err = os.Chtimes(cachePath, currentTime, currentTime)
	CheckErr(err)

	return pathModTime.Before(cacheModTime)
}

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

func Assert(value bool) {
	if !value {
		fmt.Println("assertion failed")
		panic("")
	}
}

func CheckErr(err error) {
	if err != nil {
		fmt.Println(err.Error())
		panic("")
	}
}
