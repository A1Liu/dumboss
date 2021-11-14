package main

// This code is a wrapper around Docker functionality, in the hopes that it won't
// be that much of a pain to set up on windows. We will have to see though. It's
// been cobbled together using a random assortment of blogs and auto-generated
// Docker documentation.

import (
	"bytes"
	"context"
	"fmt"
	"io"
	"os"
	"os/exec"
	"path/filepath"
	"time"

	"github.com/docker/docker/api/types"
	"github.com/docker/docker/api/types/container"
	"github.com/docker/docker/api/types/mount"
	"github.com/docker/docker/client"
	"github.com/docker/docker/pkg/stdcopy"

	. "a1liu.com/dumboss/util"
)

const (
	alpineImage = "dumboss/alpine"
	toolsImage  = "dumboss/tools"
)

func RunImageCmdSingle(ctx context.Context, binary string, args []string) {
	runImageCmd(ctx, binary, args, true)
}

func RunImageCmd(ctx context.Context, binary string, args []string) {
	runImageCmd(ctx, binary, args, false)
}

// TODO: This eventually needs to only make one container and multithread it.
//																		- Albert Liu, Nov 06, 2021 Sat 16:22 EDT
func runImageCmd(ctx context.Context, binary string, args []string, printTime bool) {
	begin := time.Now()
	cli, err := client.NewClientWithOpts(client.FromEnv, client.WithAPIVersionNegotiation())
	CheckErr(err)

	buildImage(cli, ctx, "alpine.Dockerfile", alpineImage, false)
	buildImage(cli, ctx, "Dockerfile", toolsImage, false)

	// Build container
	cmdBinary := []string{binary}
	cmdBinary = append(cmdBinary, args...)
	containerConfig := container.Config{
		Image:      toolsImage,
		Cmd:        cmdBinary,
		WorkingDir: ProjectDir,
	}
	hostConfig := container.HostConfig{
		Mounts: []mount.Mount{
			{
				Type:   mount.TypeBind,
				Source: ProjectDir,
				Target: ProjectDir,
			},
		},
	}

	resp, err := cli.ContainerCreate(ctx, &containerConfig, &hostConfig, nil, nil, "")
	if resp.Warnings != nil && len(resp.Warnings) != 0 {
		fmt.Printf("%#v\n", resp.Warnings)
	}

	if err != nil {
		buildImage(cli, ctx, "alpine.Dockerfile", alpineImage, true)
		buildImage(cli, ctx, "Dockerfile", toolsImage, true)

		resp, err = cli.ContainerCreate(ctx, &containerConfig, &hostConfig, nil, nil, "")
		if resp.Warnings != nil && len(resp.Warnings) != 0 {
			fmt.Printf("%#v\n", resp.Warnings)
		}
	}
	CheckErr(err)

	err = cli.ContainerStart(ctx, resp.ID, types.ContainerStartOptions{})
	CheckErr(err)

	fmt.Printf("docker stuff took %v seconds\n", time.Since(begin).Seconds())

	statusCh, errCh := cli.ContainerWait(ctx, resp.ID, container.WaitConditionNotRunning)
	var commandStatus int64
	select {
	case err := <-errCh:
		CheckErr(err)
	case resp := <-statusCh:
		commandStatus = resp.StatusCode
	}

	logOptions := types.ContainerLogsOptions{ShowStdout: true, ShowStderr: true}
	out, err := cli.ContainerLogs(ctx, resp.ID, logOptions)
	CheckErr(err)

	_, err = stdcopy.StdCopy(os.Stdout, os.Stderr, out)
	CheckErr(err)

	removeOptions := types.ContainerRemoveOptions{
		RemoveVolumes: true,
		Force:         true,
	}
	err = cli.ContainerRemove(ctx, resp.ID, removeOptions)
	CheckErr(err)

	if commandStatus != 0 {
		panic(commandStatus)
	}
}

func buildImage(cli *client.Client, ctx context.Context, dockerfileName, imageName string, forceBuild bool) {
	dockerfilePath := filepath.Join(BuildDir, dockerfileName)
	cacheResult := CheckUpdateCache(dockerfilePath)
	if cacheResult.CacheIsValid && !forceBuild {
		return
	}

	cmd := exec.Command("docker", "build", "--platform=linux/amd64", "-f", dockerfilePath, "--tag", imageName, ".")

	stdout, err := cmd.StdoutPipe()
	CheckErr(err)
	stderr, err := cmd.StderrPipe()
	CheckErr(err)

	finished := make(chan bool)
	var logBuffer bytes.Buffer
	logs := io.Writer(&logBuffer)

	ioCopy := func(r io.Reader) {
		io.Copy(logs, r)
		finished <- true
	}

	go ioCopy(stdout)
	go ioCopy(stderr)

	err = cmd.Run()
	if err != nil {
		<-finished
		<-finished

		fmt.Print(logBuffer.String())
		panic(err)
	}
}
