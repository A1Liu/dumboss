package main

import (
	"bufio"
	_ "bytes"
	"context"
	"encoding/json"
	"errors"
	_ "flag"
	"fmt"
	"io"
	"os"
	"os/exec"
	"path"
	"path/filepath"
	"runtime"
	"strings"

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
		fmt.Println("expected 'run' or 'build' subcommands")
		os.Exit(1)
	}

	ctx := context.Background()
	switch os.Args[1] {
	case "run":
		runCmd(ctx)
	case "build":
		buildCmd(ctx)
	default:
		fmt.Println("expected 'run' or 'build' subcommands")
		os.Exit(1)
	}
}

func compileTarget(cli *client.Client, ctx context.Context, target string) int {
	tarOptions := archive.TarOptions{IncludeFiles: []string{}}
	buildImage(cli, ctx, "alpine.Dockerfile", "dumboss/alpine", &tarOptions)
	tarOptions.IncludeFiles = []string{".build"}
	buildImage(cli, ctx, "Dockerfile", "dumboss/tools", &tarOptions)

	// Build container
	containerConfig := container.Config{
		Image:      toolsImage,
		Cmd:        []string{"make", "-f", ".build/Makefile", "kernel"},
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

	// Run container
	fmt.Println("Running container")
	err = cli.ContainerStart(ctx, resp.ID, types.ContainerStartOptions{})
	checkErr(err)

	statusCh, errCh := cli.ContainerWait(ctx, resp.ID, container.WaitConditionNotRunning)
	var commandStatus int64
	select {
	case err := <-errCh:
		checkErr(err)
	case resp := <-statusCh:
		fmt.Printf("%#v\n", resp)
		commandStatus = resp.StatusCode
	}

	logOptions := types.ContainerLogsOptions{ShowStdout: true, ShowStderr: true}
	out, err := cli.ContainerLogs(ctx, resp.ID, logOptions)
	checkErr(err)

	stdcopy.StdCopy(os.Stdout, os.Stderr, out)
	return int(commandStatus)
}

func buildImage(cli *client.Client, ctx context.Context, path string, imageName string, tarOptions *archive.TarOptions) {
	tar, err := archive.TarWithOptions(".", tarOptions)
	checkErr(err)

	// Build image
	opts := types.ImageBuildOptions{
		Dockerfile: filepath.Join(".build", path),
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
	// if f.NArg() > 0 {
	// 	panic("run doesn't take any parameters right now")
	// }

	cli, err := client.NewClientWithOpts(client.FromEnv, client.WithAPIVersionNegotiation())
	checkErr(err)

	commandStatus := compileTarget(cli, ctx, "kernel")
	if commandStatus != 0 {
		os.Exit(commandStatus)
	}

	args := []string{"-smp", "4", "-pflash", ".build/OVMF.bin", "-serial", "stdio",
		"-hda", ".build/out/kernel"}
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

func buildCmd(ctx context.Context) {
	// if f.NArg() > 0 {
	// 	panic("build doesn't take any parameters right now")
	// }

	cli, err := client.NewClientWithOpts(client.FromEnv, client.WithAPIVersionNegotiation())
	checkErr(err)

	commandStatus := compileTarget(cli, ctx, "kernel")

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
		Aux         Aux         `json:"aux"`
	}

	lastStep := make([]byte, 50)[:0]
	currentStep := make([]byte, 50)[:0]
	iter := 0
	scanner := bufio.NewScanner(rd)
	for scanner.Scan() {
		buf := scanner.Bytes()

		var line StreamLine
		err := json.Unmarshal(buf, &line)
		if line.Value == "" && line.Aux.Id != "" {
			break
		}

		fmt.Print(line.Value)

		if err != nil || line.Value == "" {
			// fmt.Println("BUILD FAILED")
			// fmt.Println("Last step: ", string(lastStep))
			// fmt.Println("Current step: ", string(currentStep))

			errLine := ErrorLine{Error: line.Error, ErrorDetail: line.ErrorDetail}
			return errors.New(fmt.Sprintf("%#v", errLine))
		}

		if strings.HasPrefix(line.Value, "Step") {
			lastStep, currentStep = currentStep, lastStep
			currentStep = currentStep[:0]
			iter += 1
		}
		currentStep = append(currentStep, []byte(line.Value)...)
	}

	return scanner.Err()
}
