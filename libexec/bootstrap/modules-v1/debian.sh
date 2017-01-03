#!/bin/bash
# 
# Copyright (c) 2015-2017, Gregory M. Kurtzer. All rights reserved.
# 
# Copyright (c) 2016-2017, The Regents of the University of California,
# through Lawrence Berkeley National Laboratory (subject to receipt of any
# required approvals from the U.S. Dept. of Energy).  All rights reserved.
# 
# This software is licensed under a customized 3-clause BSD license.  Please
# consult LICENSE file distributed with the sources of this project regarding
# your rights to use or distribute this software.
# 
# NOTICE.  This Software was developed under funding from the U.S. Department of
# Energy and the U.S. Government consequently retains certain rights. As such,
# the U.S. Government has been granted for itself and others acting on its
# behalf a paid-up, nonexclusive, irrevocable, worldwide license in the Software
# to reproduce, distribute copies to the public, prepare derivative works, and
# perform publicly and display publicly, and to permit other to do so. 
# 
# 

## Basic sanity
if [ -z "$SINGULARITY_libexecdir" ]; then
    echo "Could not identify the Singularity libexecdir."
    exit 1
fi

## Load functions
if [ -f "$SINGULARITY_libexecdir/singularity/functions" ]; then
    . "$SINGULARITY_libexecdir/singularity/functions"
else
    echo "Error loading functions: $SINGULARITY_libexecdir/singularity/functions"
    exit 1
fi

if ! DEBOOTSTRAP_PATH=`singularity_which debootstrap`; then
    message ERROR "debootstrap is not in PATH... Perhaps 'apt-get install' it?\n"
    exit 1
fi

if uname -m | grep -q x86_64; then
    ARCH=amd64
else
    ARCH=i386
fi


Bootstrap() {
    if [ -z "${MIRROR:-}" ]; then
        echo "ERROR: MIRROR is not defined, have you configure 'MirrorURL'?"
        exit 1
    fi

    if [ -z "${VERSION:-}" ]; then
        echo "ERROR: VERSION is not defined, have you set 'OSVersion'?"
        exit 1
    fi

    # debootstrap will create the device entries it needs (or some versions fail)
    /bin/sh -c "rm -rf $SINGULARITY_ROOTFS/dev/*"

    INCLUDEPKGS=`echo "$*" | sed -e 's/[ ]*/,/g'`

    if [ -z "${INCLUDEPKGS:-}" ]; then
        INCLUDEPKGS=",$INCLUDEPKGS"
    fi

    # The excludes save 25M or so with jessie.  (Excluding udev avoids
    # systemd, for instance.)  There are a few more we could exclude
    # to save a few MB.  I see 182M cleaned with this, v. 241M with
    # the default debootstrap.
    if ! eval "$DEBOOTSTRAP_PATH --variant=minbase --exclude=openssl,udev,debconf-i18n,e2fsprogs --include=apt$INCLUDEPKGS --arch=$ARCH '$VERSION' '$SINGULARITY_ROOTFS' '$MIRROR'"; then
        exit 1
    fi

    if ! __runcmd apt-get update; then
        return 1
    fi

    if [ -n "${LANG:-}" ]; then
        if LOCALE_GEN=`singularity_which locale-gen`; then
            if __runcmd $LOCALE_GEN ${LANG:-}; then
                mkdir -p "$SINGULARITY_ROOTFS/etc/default/" >/dev/null 2>&1
                echo "LANG=${LANG:-}" > "$SINGULARITY_ROOTFS/etc/default/locale"
            fi
        fi
    fi

    __mountproc
    __mountsys
    __mountdev

    return 0
}


InstallPkgs() {
    if ! __runcmd apt-get -y --force-yes install "$@"; then
        return 1
    fi

    return 0
}

Cleanup() {
    if ! __runcmd apt-get clean; then
        return 1
    fi

    return 0
}
