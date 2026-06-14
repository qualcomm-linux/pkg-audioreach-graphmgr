#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause-Clear
#
# Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
set -ex
echo "Running build script..."
# Build/Compile audioreach-graphmgr
source ${GITHUB_WORKSPACE}/install/environment-setup-cortexa7t2hf-neon-vfpv4-poky-linux-gnueabi
# make sure we are in the right directory
cd ${GITHUB_WORKSPACE}

# Run autoreconf to generate the configure script
autoreconf -Wcross --verbose --install --force --exclude=autopoint
autoconf --force
# Run the configure script with the specified arguments
./configure ${BUILD_ARGS}
# make
make DESTDIR=${GITHUB_WORKSPACE}/build install
