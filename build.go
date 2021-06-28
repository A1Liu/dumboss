package main

// This code is a wrapper around Docker functionality, in the hopes that it won't
// be that much of a pain to set up on windows. We will have to see though. It's
// been cobbled together using a random assortment of blogs and auto-generated
// Docker documentation.

import (
	"bufio"
	_ "bytes"
	"context"
	"encoding/json"
	"errors"
	_ "flag"
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

	"github.com/docker/docker/api/types"
	"github.com/docker/docker/api/types/container"
	"github.com/docker/docker/api/types/mount"
	"github.com/docker/docker/client"
	"github.com/docker/docker/pkg/archive"
	"github.com/docker/docker/pkg/stdcopy"
)

const (
	toolsImage = "dumboss/tools"
)

var (
	projectDir = func() string {
		_, filename, _, _ := runtime.Caller(0)
		abs, err := filepath.Abs(filename)
		checkErr(err)

		return path.Dir(abs)
	}()
)

func main() {
	if len(os.Args) < 2 {
		fmt.Println("expected 'run' subcommand, or a Makefile target")
		os.Exit(1)
	}

	ctx := context.Background()
	switch os.Args[1] {
	case "run":
		runCmd(ctx)
	default:
		runMakeCmd(ctx, os.Args[1])
	}
}

func compileTarget(cli *client.Client, ctx context.Context, target string) int {
	begin := time.Now()
	tarOptions := archive.TarOptions{IncludeFiles: []string{}}
	buildImage(cli, ctx, "alpine.Dockerfile", "dumboss/alpine", &tarOptions)
	tarOptions.IncludeFiles = []string{".build"}
	buildImage(cli, ctx, "Dockerfile", "dumboss/tools", &tarOptions)

	// Build container
	containerConfig := container.Config{
		Image: toolsImage,
		Cmd:   []string{"make", "-f", ".build/Makefile", target},
		// Cmd:        []string{"make", "-d", "-f", ".build/Makefile", target},
		WorkingDir: "/root/dumboss",
	}
	hostConfig := container.HostConfig{
		Mounts: []mount.Mount{
			{
				Type:   mount.TypeBind,
				Source: projectDir,
				Target: "/root/dumboss",
			},
		},
	}
	resp, err := cli.ContainerCreate(ctx, &containerConfig, &hostConfig, nil, nil, "")
	checkErr(err)

	// TODO if someone docker prunes without also removing the cache flag, this will
	// fail because the docker image doesnt exist but the build script thinks it does
	err = cli.ContainerStart(ctx, resp.ID, types.ContainerStartOptions{})
	checkErr(err)

	fmt.Printf("docker stuff took %v seconds\n", time.Since(begin).Seconds())
	statusCh, errCh := cli.ContainerWait(ctx, resp.ID, container.WaitConditionNotRunning)
	var commandStatus int64
	select {
	case err := <-errCh:
		checkErr(err)
	case resp := <-statusCh:
		commandStatus = resp.StatusCode
	}

	logOptions := types.ContainerLogsOptions{ShowStdout: true, ShowStderr: true}
	out, err := cli.ContainerLogs(ctx, resp.ID, logOptions)
	checkErr(err)

	stdcopy.StdCopy(os.Stdout, os.Stderr, out)
	return int(commandStatus)
}

func needBuild(dockerfilePath, imageName string) bool {
	outDir := filepath.Join(projectDir, ".build", "out")
	err := os.MkdirAll(outDir, fs.ModeDir|fs.ModePerm)
	checkErr(err)

	escapedName := strings.Replace(imageName, string(os.PathSeparator), ".", -1)
	placeholder := filepath.Join(outDir, ".image-"+escapedName)
	dockerfilePath = filepath.Join(projectDir, dockerfilePath)

	dockerfileStat, err := os.Stat(dockerfilePath)
	checkErr(err)
	placeholderStat, err := os.Stat(placeholder)

	if os.IsNotExist(err) {
		file, err := os.Create(placeholder)
		checkErr(err)
		file.Close()
		return true
	} else {
		currentTime := time.Now().Local()
		err = os.Chtimes(placeholder, currentTime, currentTime)
		checkErr(err)

		return dockerfileStat.ModTime().After(placeholderStat.ModTime())
	}
}

func buildImage(cli *client.Client, ctx context.Context, dockerfilePath, imageName string, tarOptions *archive.TarOptions) {
	dockerfilePath = filepath.Join(".build", dockerfilePath)
	if !needBuild(dockerfilePath, imageName) {
		return
	}

	tar, err := archive.TarWithOptions(projectDir, tarOptions)
	checkErr(err)

	// Build image
	opts := types.ImageBuildOptions{
		Dockerfile: dockerfilePath,
		Tags:       []string{imageName},
		Remove:     true,
	}
	res, err := cli.ImageBuild(ctx, tar, opts)
	checkErr(err)
	err = printImageLogs(res.Body)
	checkErr(err)
	res.Body.Close()
}

func runCmd(ctx context.Context) {
	cli, err := client.NewClientWithOpts(client.FromEnv, client.WithAPIVersionNegotiation())
	checkErr(err)

	commandStatus := compileTarget(cli, ctx, "build")
	if commandStatus != 0 {
		os.Exit(commandStatus)
	}

	args := []string{"-smp", "4", "-pflash", filepath.Join(projectDir, ".build/OVMF.bin"),
		"-serial", "stdio", "-hda", filepath.Join(projectDir, ".build/out/kernel")}
	cmd := exec.Command("qemu-system-x86_64", args...)

	stdout, err := cmd.StdoutPipe()
	checkErr(err)
	stderr, err := cmd.StderrPipe()
	checkErr(err)
	go io.Copy(os.Stdout, stdout)
	go io.Copy(os.Stderr, stderr)
	err = cmd.Run()
	checkErr(err)
	os.Exit(0)
}

func runMakeCmd(ctx context.Context, target string) {
	cli, err := client.NewClientWithOpts(client.FromEnv, client.WithAPIVersionNegotiation())
	checkErr(err)

	commandStatus := compileTarget(cli, ctx, target)

	os.Exit(commandStatus)
}

func checkErr(err error) {
	if err != nil {
		panic(err.Error())
	}
}

func printImageLogs(rd io.Reader) error {
	type ErrorDetail struct {
		Message string `json:"message"`
		Code    int    `json:"code"`
	}

	type ErrorLine struct {
		Error       string      `json:"error"`
		ErrorDetail ErrorDetail `json:"errorDetail"`
	}

	type Aux struct {
		Id string `json:"ID"`
	}

	type StreamLine struct {
		Error       string      `json:"error"`
		ErrorDetail ErrorDetail `json:"errorDetail"`
		Value       string      `json:"stream"`
		Status      string      `json:"status"`
		Id          string      `json:"id"`
		Aux         Aux         `json:"aux"`
	}

	scanner := bufio.NewScanner(rd)
	for scanner.Scan() {
		buf := scanner.Bytes()

		var line StreamLine
		err := json.Unmarshal(buf, &line)
		if line.Value == "" && line.Aux.Id != "" {
			break
		}

		if line.Status != "" { // TODO what should we do here?
			continue
		}

		if err != nil || line.Value == "" {
			errLine := ErrorLine{Error: line.Error, ErrorDetail: line.ErrorDetail}
			return errors.New(fmt.Sprintf("%#v", errLine))
		}

		fmt.Print(line.Value)
	}

	return scanner.Err()
}
