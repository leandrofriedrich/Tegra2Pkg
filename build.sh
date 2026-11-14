# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2026, Leandro Friedrich.

# minimal build script
export PACKAGES_PATH=$PWD/../edk2:$PWD/../edk2-platforms:$PWD
export WORKSPACE=$PWD/workspace
. ../edk2/edksetup.sh

build -a ARM -t GCC5 -p Tegra2Pkg/Tegra2Pkg.dsc -b DEBUG
