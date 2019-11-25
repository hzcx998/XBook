#!/bin/sh
#

# The qemu's root directory.
export QEMU_DIR=$(cd `dirname $0`/.. ; pwd)

# Run qemu
setsid bash -c "gnome-terminal -x make qemudbg"
