package dumboss

import (
	"fmt"
	"github.com/karrick/godirwalk"
	"io"
	"io/fs"
	"os"
	"os/exec"
	"path"
	"path/filepath"
	"runtime"
	"strings"
	"time"
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

func DirWalk(dirname string) []string {
	fileNames := make([]string, 10)

	walker := func(osPathname string, de *godirwalk.Dirent) error {
		if de.IsRegular() {
			fileNames = append(fileNames, osPathname)
			fmt.Printf("%s %s\n", de.ModeType(), osPathname)
		}

		return nil
	}

	err := godirwalk.Walk(dirname, &godirwalk.Options{
		FollowSymbolicLinks: false,
		Unsorted:            false,
		Callback:            walker,
	})
	CheckErr(err)

	return fileNames
}

func CacheIsValid(filePath string) bool {
	abs, err := filepath.Abs(filePath)
	CheckErr(err)

	if strings.HasPrefix(abs, CacheDir) {
		panic("caching a file in the cache directory")
	}

	rel, err := filepath.Rel(ProjectDir, abs)
	CheckErr(err)

	cachePath := filepath.Join(CacheDir, rel)

	pathStat, err := os.Stat(rel)
	if os.IsNotExist(err) {
		return false
	}
	CheckErr(err)

	cacheFileStat, err := os.Stat(cachePath)
	if os.IsNotExist(err) {
		err = os.MkdirAll(path.Dir(cachePath), fs.ModeDir|fs.ModePerm)
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

func CheckErr(err error) {
	if err != nil {
		panic(err.Error())
	}
}
