package main

// This code is a wrapper around Docker functionality, in the hopes that it won't
// be that much of a pain to set up on windows. We will have to see though. It's
// been cobbled together using a random assortment of blogs and auto-generated
// Docker documentation.

import (
	"context"
	"fmt"
	"io"
	"os"
	"os/exec"
	"path/filepath"
	"sync/atomic"
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

func RunImageCmd(ctx context.Context, binary string, args []string) {
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

	targetBegin := time.Now()
	fmt.Printf("docker stuff took %v seconds\n", targetBegin.Sub(begin).Seconds())

	attachOptions := types.ContainerAttachOptions{
		Stream: true,
		Stdout: true,
		Stderr: true,
		Logs:   true,
	}
	running, err := cli.ContainerAttach(ctx, resp.ID, attachOptions)
	CheckErr(err)
	defer running.Close()

	_, err = stdcopy.StdCopy(os.Stdout, os.Stderr, running.Reader)
	CheckErr(err)

	statusCh, errCh := cli.ContainerWait(ctx, resp.ID, container.WaitConditionNotRunning)
	var commandStatus int64
	select {
	case err := <-errCh:
		CheckErr(err)
	case resp := <-statusCh:
		commandStatus = resp.StatusCode
	}

	removeOptions := types.ContainerRemoveOptions{
		RemoveVolumes: true,
		Force:         true,
	}
	err = cli.ContainerRemove(ctx, resp.ID, removeOptions)
	CheckErr(err)

	targetTime := time.Since(targetBegin).Seconds()

	if commandStatus != 0 {
		fmt.Printf("target failed after %v seconds\n", targetTime)
		os.Exit(1)
	}

	fmt.Printf("target took %v seconds\n", targetTime)
}

func buildImage(cli *client.Client, ctx context.Context, dockerfileName, imageName string, forceBuild bool) {
	dockerfilePath := filepath.Join(BuildDir, dockerfileName)
	cacheResult := CheckUpdateCache(dockerfilePath)
	if cacheResult.CacheIsValid && !forceBuild {
		return
	}

	fmt.Printf("building image %v...\n", imageName)
	begin := time.Now()

	// This uses the docker command because I couldn't figure out how to guarantee
	// that the dockerfile would run on AMD64 without it.
	cmd := exec.Command("docker", "build", "--platform=linux/amd64", "-f", dockerfilePath, "--tag", imageName, ProjectDir)

	stdout, err := cmd.StdoutPipe()
	CheckErr(err)
	stderr, err := cmd.StderrPipe()
	CheckErr(err)

	writeFinished := make(chan bool)
	commandStatus := int32(0)

	ioCopy := func(w io.Writer, r io.Reader) {
		time.Sleep(2 * time.Second)

		if !atomic.CompareAndSwapInt32(&commandStatus, 0, 2) && atomic.LoadInt32(&commandStatus) == 1 {
			return
		}

		io.Copy(w, r)
		writeFinished <- true
	}

	go ioCopy(os.Stdout, stdout)
	go ioCopy(os.Stdout, stderr)

	err = cmd.Run()
	if err != nil {
		<-writeFinished
		<-writeFinished

		// The err value is almost certainly useless, as it comes from go's command
		// runner. Let's ignore it.
		os.Exit(1)
	}

	if !atomic.CompareAndSwapInt32(&commandStatus, 0, 1) {
		<-writeFinished
		<-writeFinished
		fmt.Printf("\nimage %v took %v seconds\n\n", imageName, time.Since(begin).Seconds())
		return
	}

	// The spacing here aligns the word `image` with the previously printed line.
	fmt.Printf("         image %v took %v seconds\n\n", imageName, time.Since(begin).Seconds())
}
