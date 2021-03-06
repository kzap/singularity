// Copyright (c) 2018, Sylabs Inc. All rights reserved.
// This software is licensed under a 3-clause BSD license. Please consult the
// LICENSE.md file distributed with the sources of this project regarding your
// rights to use or distribute this software.

// +build singularity_runtime

package cli

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"

	"github.com/opencontainers/runtime-tools/generate"
	"github.com/spf13/cobra"
	"github.com/sylabs/singularity/docs"
	"github.com/sylabs/singularity/internal/pkg/buildcfg"
	"github.com/sylabs/singularity/internal/pkg/runtime/engines/config"
	"github.com/sylabs/singularity/internal/pkg/runtime/engines/config/oci"
	"github.com/sylabs/singularity/internal/pkg/runtime/engines/singularity"
	"github.com/sylabs/singularity/internal/pkg/sylog"
	"github.com/sylabs/singularity/internal/pkg/util/exec"
)

const (
	standardHelpPath = "/.singularity.d/runscript.help"
	appHelpPath      = "/scif/apps/%s/scif/runscript.help"
)

func init() {
	RunHelpCmd.Flags().SetInterspersed(false)

	RunHelpCmd.Flags().StringVar(&AppName, "app", "", "Get help info for specific app")
	RunHelpCmd.Flags().SetAnnotation("app", "envkey", []string{"APP"})

	SingularityCmd.AddCommand(RunHelpCmd)
}

// RunHelpCmd singularity run-help <image>
var RunHelpCmd = &cobra.Command{
	DisableFlagsInUseLine: true,
	PreRun:                sylabsToken,
	Args:                  cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		// Sanity check
		if _, err := os.Stat(args[0]); err != nil {
			sylog.Fatalf("container not found: %s", err)
		}

		// Help prints (if set) the sourced %help section on the definition file
		abspath, err := filepath.Abs(args[0])
		name := filepath.Base(abspath)

		a := []string{"/bin/cat", getHelpPath(cmd)}
		starter := buildcfg.LIBEXECDIR + "/singularity/bin/starter-suid"
		procname := "Singularity help"
		Env := []string{sylog.GetEnvVar()}

		engineConfig := singularity.NewConfig()
		ociConfig := &oci.Config{}
		generator := generate.Generator{Config: &ociConfig.Spec}
		engineConfig.OciConfig = ociConfig

		generator.SetProcessArgs(a)
		engineConfig.SetImage(abspath)

		cfg := &config.Common{
			EngineName:   singularity.Name,
			ContainerID:  name,
			EngineConfig: engineConfig,
		}

		configData, err := json.Marshal(cfg)
		if err != nil {
			sylog.Fatalf("CLI Failed to marshal CommonEngineConfig: %s\n", err)
		}

		if err := exec.Pipe(starter, []string{procname}, Env, configData); err != nil {
			sylog.Fatalf("%s", err)
		}
	},

	Use:     docs.RunHelpUse,
	Short:   docs.RunHelpShort,
	Long:    docs.RunHelpLong,
	Example: docs.RunHelpExample,
}

func getHelpPath(cmd *cobra.Command) string {
	if cmd.Flags().Changed("app") {
		sylog.Debugf("App specified. Looking for help section of %s", AppName)
		return fmt.Sprintf(appHelpPath, AppName)
	}

	return standardHelpPath
}
