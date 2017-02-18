/* 
 * Copyright (c) 2015-2017, Gregory M. Kurtzer. All rights reserved.
 * 
 * Copyright (c) 2016-2017, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 * 
 * This software is licensed under a customized 3-clause BSD license.  Please
 * consult LICENSE file distributed with the sources of this project regarding
 * your rights to use or distribute this software.
 *
 * NOTICE.  This Software was developed under funding from the U.S. Department of
 * Energy and the U.S. Government consequently retains certain rights. As such,
 * the U.S. Government has been granted for itself and others acting on its
 * behalf a paid-up, nonexclusive, irrevocable, worldwide license in the Software
 * to reproduce, distribute copies to the public, prepare derivative works, and
 * perform publicly and display publicly, and to permit other to do so. 
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h> 
#include <string.h>
#include <limits.h>

#include "config.h"
#include "util/util.h"
#include "util/message.h"
#include "util/file.h"
#include "util/registry.h"

#ifndef SYSCONFDIR
#error SYSCONFDIR not defined
#endif


int singularity_suid_init(void) {

#ifdef SINGULARITY_SUID
    singularity_message(VERBOSE2, "Running SUID program workflow\n");

    singularity_message(VERBOSE2, "Checking program has appropriate permissions\n");
    if ( ( is_owner("/proc/self/exe", 0) < 0 ) || ( is_suid("/proc/self/exe") < 0 ) ) {
        char *path = (char*) malloc(PATH_MAX);
        int len = readlink("/proc/self/exe", path, PATH_MAX - 1);
        if ( len <= 0 ) {
            singularity_abort(255, "Could not obtain link target of self\n");
        }
        if ( len == PATH_MAX - 1 ) {
            singularity_abort(255, "Link length error!\n");
        }
        path[len] = '\0';

        singularity_message(ERROR, "Installation error, run the following commands as root to fix:\n");
        singularity_message(ERROR, "    sudo chown root:root %s\n", path);
        singularity_message(ERROR, "    sudo chmod 4755 %s\n", path);
        if ( getuid() == 0 ) {
            singularity_message(INFO, "\n");
        } else {
            ABORT(255);
        }
    }

    singularity_message(VERBOSE2, "Checking configuration file is properly owned by root\n");
    if ( is_owner(joinpath(SYSCONFDIR, "/singularity/singularity.conf"), 0 ) < 0 ) {
        singularity_abort(255, "Running in privileged mode, root must own the Singularity configuration file: %s\n", joinpath(SYSCONFDIR, "/singularity/singularity.conf"));
    }

    singularity_message(VERBOSE2, "Checking if we were requested to run as NOSUID by user\n");
    if ( singularity_registry_get("NOSUID") != NULL ) {
        singularity_abort(1, "NOSUID mode has been requested... Aborting\n");
    }

#else
    singularity_message(VERBOSE, "Running NON-SUID program workflow\n");

    singularity_message(DEBUG, "Checking program has appropriate permissions\n");
    if ( is_suid("/proc/self/exe") >= 0 ) {
        singularity_abort(255, "This program must **NOT** be SUID\n");
    }

#endif /* SINGULARITY_SUID */

    return(0);
}

int singularity_suid_enabled(void) {
    if ( is_owner("/proc/self/exe", 0) < 0 ) {
        singularity_message(DEBUG, "Executable is not root owned\n");
        return(-1);
    }

    if ( is_suid("/proc/self/exe") < 0 ) {
        singularity_message(DEBUG, "Executable is not SUID\n");
        return(-1);
    }

    return(1);
}