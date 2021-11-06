package dumboss

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
	"time"

	"github.com/docker/docker/api/types"
	"github.com/docker/docker/api/types/container"
	"github.com/docker/docker/api/types/mount"
	"github.com/docker/docker/client"
	"github.com/docker/docker/pkg/stdcopy"
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

	compileBegin := time.Now()
	fmt.Printf("docker stuff took %v seconds\n\n", compileBegin.Sub(begin).Seconds())

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

	stdcopy.StdCopy(os.Stdout, os.Stderr, out)

	removeOptions := types.ContainerRemoveOptions{
		RemoveVolumes: true,
		Force:         true,
	}
	err = cli.ContainerRemove(ctx, resp.ID, removeOptions)
	CheckErr(err)

	if commandStatus != 0 {
		os.Exit(int(commandStatus))
	}
}

func buildImage(cli *client.Client, ctx context.Context, dockerfileName, imageName string, forceBuild bool) {
	dockerfilePath := filepath.Join(BuildDir, dockerfileName)
	if CacheIsValid(dockerfilePath) && !forceBuild {
		return
	}

	cmd := exec.Command("docker", "build", "--platform=linux/amd64", "-f", dockerfilePath, "--tag", imageName, ".")

	stdout, err := cmd.StdoutPipe()
	CheckErr(err)
	stderr, err := cmd.StderrPipe()
	CheckErr(err)

	finished := make(chan bool)
	ioCopy := func(w io.Writer, r io.Reader) {
		io.Copy(w, r)
		finished <- true
	}

	go ioCopy(os.Stdout, stdout)
	go ioCopy(os.Stderr, stderr)
	err = cmd.Run()
	<-finished
	<-finished

	CheckErr(err)
}
