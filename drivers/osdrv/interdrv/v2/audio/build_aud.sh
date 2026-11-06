#!/bin/bash

export KERNEL_DIR="$KERNEL_PATH"/"$KERNEL_OUTPUT_FOLDER"
export INSTALL_DIR="$SYSTEM_OUT_DIR"/ko

make clean
make
