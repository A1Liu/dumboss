package dumboss

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

		return path.Dir(path.Dir(abs))
	}()
	BuildDir = filepath.Join(ProjectDir, ".build")
	CacheDir = filepath.Join(BuildDir, "cache")
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

func CacheIsValid(filePath string) bool {
	_, callerPath, callerLine, ok := runtime.Caller(0)
	Assert(ok)
	callerRelPath, err := filepath.Rel(ProjectDir, callerPath)
	CheckErr(err)

	abs, err := filepath.Abs(filePath)
	CheckErr(err)
	Assert(!strings.HasPrefix(abs, CacheDir))

	rel, err := filepath.Rel(ProjectDir, abs)
	CheckErr(err)

	cachePathName := filepath.Join("C"+rel, callerRelPath, string(callerLine))
	cachePathName = strings.Replace(cachePathName, string(os.PathSeparator), ".", -1)

	cachePath := filepath.Join(CacheDir, cachePathName)

	pathStat, err := os.Stat(rel)
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
		panic("assertion failed")
	}
}

func CheckErr(err error) {
	if err != nil {
		panic(err.Error())
	}
}
