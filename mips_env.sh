#!/bin/bash -e

SDK_ROOT=`pwd`

export ZEPHYR_GCC_VARIANT=mipssde
export MIPSSDE_TOOLCHAIN_PATH=/opt/mips-2014.11
export BOARD=ats2837m_evb

source ${SDK_ROOT}/zephyr-env.sh
