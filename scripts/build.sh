#!/bin/sh
set -e

export STM32_CUBE_G0_PATH=/project/ext/STM32CubeG0
export STM32_TOOLCHAIN_PATH=/usr
export STM32_TARGET_TRIPLET=arm-none-eabi

cd /build

cmake -GNinja /project
ninja install
