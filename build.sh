#!/bin/bash -e
#
# Copyright (c) 2017 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0
#

unset MENU_CHOICES_ARRAY
add_item(){
	local new_item=$1
	local c
	for c in ${MENU_CHOICES_ARRAY[@]} ; do
		if [ "$new_item" = "$c" ] ; then
			return
		fi
	done
	MENU_CHOICES_ARRAY=(${MENU_CHOICES_ARRAY[@]} $new_item)
}

build_list() {
	unset MENU_CHOICES_ARRAY
	local file
	for file in ` ls -1 $1`
	do
		if [ -d $1"/"$file ]
		then
			add_item $file
		fi

	done
}

print_menu(){
	local i=1
	local choice
	for choice in ${MENU_CHOICES_ARRAY[@]}
	do
		echo "     $i. $choice"
		i=$(($i+1))
	done

	echo
}

read_choice() {
	local answer
	local result

	echo
	echo $2
	echo
	build_list $3
	print_menu
	echo -n "Which would you like? [${MENU_CHOICES_ARRAY[0]}] "
	read answer

	if [ -z "$answer" ]; then
		result=${MENU_CHOICES_ARRAY[0]}
	elif (echo -n $answer | grep -q -e "^[0-9][0-9]*$"); then
		if [ $answer -le ${#MENU_CHOICES_ARRAY[@]} ]; then
		    result=${MENU_CHOICES_ARRAY[$(($answer-1))]}
		fi
	else
		result=$answer
	fi

	if [ -z "$result" ]; then
		echo "Invalid select: $answer"
		return 1
	fi

	eval $1=$result
}

print_usage(){
	echo "Usage:"
	echo "  Build firmware"
	echo "    $0"
	echo "  Clean build out directory"
	echo "    $0 -c"
	echo
}

SDK_ROOT=`pwd`
source ${SDK_ROOT}/zephyr-env.sh

#ARCH=csky
#export ZEPHYR_GCC_VARIANT=gcccsky
#export GCCCSKYV2_TOOLCHAIN_PATH=/opt/csky-elfabiv2

ARCH=mips
export ZEPHYR_GCC_VARIANT=mipssde
#export MIPSSDE_TOOLCHAIN_PATH=/opt/mips-2014.11
export MIPSSDE_TOOLCHAIN_PATH=/opt/mips-mti-elf/2019.09-01

BOARD=
APPLICATION=
BUILD_CLEAN=0
BUILD_SDK_LIB=0
BACKUP_FW=0
SYNC_BUILD=0
ECHECK_FW_NAME=
BUILD_CONFIG_FILE=${SDK_ROOT}/.build_config
RECOVERY_CONF=prj_recovery.conf

# import old build config
if [ -f ${BUILD_CONFIG_FILE} ]; then
	source ${BUILD_CONFIG_FILE}
fi

if [ $# = 1 ]; then
# skip old config?
if [ "$1" = "-n" ]; then
	BOARD=""
	APPLICATION=""
# clean build directory?
elif [ "$1" = "-c" ]; then
	BUILD_CLEAN=1
elif [ "$1" = "-lib" ]; then
	BUILD_SDK_LIB=1
elif [ "$1" = "-p" ]; then
	PACK_FW_ONLY=1
elif [ "$1" = "-b" ]; then
	BACKUP_FW=1
	SYNC_BUILD=1
else
	print_usage
	exit 1
exit 1
fi
elif [ $# = 2 ]; then
BOARD=$1
APPLICATION=$2
fi

if [ $# = 1 ] && [ "$1" = "-n" ]; then
	BOARD=""
	APPLICATION=""
	CONFIG=""
fi

# select board and application
if [ "${BOARD}" = "" ] || [ "${APPLICATION}" = "" ]; then
	read_choice BOARD "Select board:" ${SDK_ROOT}/boards/${ARCH}
	read_choice APPLICATION "Select application:" ${SDK_ROOT}/samples
fi

if [ "${CONFIG}" = "" ] && [ -d ${SDK_ROOT}/samples/${APPLICATION}/app_conf ]; then
	read_choice CONFIG "Select application conf:" ${SDK_ROOT}/samples/${APPLICATION}/app_conf
fi

# write varibles to config file
echo "# Build config" > ${BUILD_CONFIG_FILE}
echo "BOARD=${BOARD}" >> ${BUILD_CONFIG_FILE}
echo "APPLICATION=${APPLICATION}" >> ${BUILD_CONFIG_FILE}
echo "CONFIG=${CONFIG}" >> ${BUILD_CONFIG_FILE}
echo "MIPSSDE_TOOLCHAIN_PATH=${MIPSSDE_TOOLCHAIN_PATH}" >> ${BUILD_CONFIG_FILE}

# generate echeck file name
PACKAGE_NAME=${BOARD%_*}
ECHECK_FW_NAME="E${PACKAGE_NAME^^}.FW"

PRJ_BASE=${SDK_ROOT}/samples/${APPLICATION}

if [ ! -d ${SDK_ROOT}/boards/${ARCH}/${BOARD} ]; then
	echo -e "\nNo board at ${SDK_ROOT}/boards/${ARCH}/${BOARD}\n\n"
	exit 1
fi

if [ ! -d ${PRJ_BASE} ]; then
	echo -e "\nNo application at ${PRJ_BASE}\n\n"
	exit 1
fi

if [ "${BUILD_CLEAN}" = "1" ]; then
	echo "Clean build out directory"
	rm -rf ${PRJ_BASE}/outdir/${BOARD}
	rm -rf ${PRJ_BASE}/outdir/${BOARD}_recovery
	rm -rf ${PRJ_BASE}/outdir/${BOARD}_second
	exit 0
fi

echo -e "\n--== Build application ${APPLICATION} for board ${BOARD} used config ${CONFIG} ==--\n\n"

# build application
if [ -d ${PRJ_BASE}/app_conf/${CONFIG} ]; then

	mkdir -p ${PRJ_BASE}/outdir/${BOARD}
	mkdir -p ${PRJ_BASE}/outdir/${BOARD}/sdfs

	if [ -f ${PRJ_BASE}/firmware.xml ] ; then
		echo -e "FW: Override the default firmware.xml"
		cp ${PRJ_BASE}/firmware.xml ${PRJ_BASE}/outdir/${BOARD}
	fi

	if [ -f ${PRJ_BASE}/app_conf/${CONFIG}/firmware.xml ] ; then
		echo -e "FW: Override the project firmware.xml"
		cp ${PRJ_BASE}/app_conf/${CONFIG}/firmware.xml ${PRJ_BASE}/outdir/${BOARD}/
	fi

	if [ -d ${PRJ_BASE}/app_conf/${CONFIG}/sdfs ]; then
		echo "sdfs: copy sdfs for ${CONFIG} ..."
		cp -rf  ${PRJ_BASE}/app_conf/${CONFIG}/sdfs/* ${PRJ_BASE}/outdir/${BOARD}/sdfs/
	fi;

	if [ "${PACK_FW_ONLY}" = "1" ]; then
		echo -e "\n--== Pack firmware for board ${BOARD} ==--\n\n"
		make -C ${PRJ_BASE} BOARD=${BOARD} CONF_FILE=${PRJ_BASE}/app_conf/${CONFIG}/prj.conf APP_CONFIG=${CONFIG} ECHECK_FW=${ECHECK_FW_NAME} firmware
		exit 0;
	fi;

	echo -e "\n--== Build main application for board ${BOARD} ==--\n\n"
	make -C ${PRJ_BASE} BOARD=${BOARD} CONF_FILE=${PRJ_BASE}/app_conf/${CONFIG}/prj.conf APP_CONFIG=${CONFIG} SYNC_BUILD=${SYNC_BUILD} -j4

	if [ -f ${PRJ_BASE}/app_conf/${CONFIG}/prj_second.conf ]; then
		echo -e "\n--== Build second application for board ${BOARD} ==--\n\n"
		make -C ${PRJ_BASE} BOARD=${BOARD} CONF_FILE=${PRJ_BASE}/app_conf/${CONFIG}/prj_second.conf APP_CONFIG=${CONFIG} O=${PRJ_BASE}/outdir/${BOARD}_second -j4
	fi

	if [ -f ${PRJ_BASE}/app_conf/${CONFIG}/${RECOVERY_CONF} ]; then
		echo -e "\n--== Build recovery application for board ${BOARD} ==--\n\n"
		# build recovery application
		make -C ${PRJ_BASE} BOARD=${BOARD} CONF_FILE=${PRJ_BASE}/app_conf/${CONFIG}/${RECOVERY_CONF} APP_CONFIG=${CONFIG} O=${PRJ_BASE}/outdir/${BOARD}_recovery -j4
	fi

else
	make -C ${PRJ_BASE} BOARD=${BOARD} -j4

	if [ -f ${PRJ_BASE}/${RECOVERY_CONF} ]; then
		echo -e "\n--== Build recovery application for board ${BOARD} ==--\n\n"
		# build recovery application
		make -C ${PRJ_BASE} BOARD=${BOARD} CONF_FILE=${RECOVERY_CONF} APP_CONFIG=${CONFIG} O=${PRJ_BASE}/outdir/${BOARD}_recovery -j4
	fi
fi

if [ "${BUILD_SDK_LIB}" = "1" ]; then
	echo "build private library"
	make -C ${PRJ_BASE} BOARD=${BOARD} update-private-library
	echo "build SDK library"
	make -C ${PRJ_BASE} BOARD=${BOARD} update-sdk-library
	echo "build SOC library"
	make -C ${PRJ_BASE} BOARD=${BOARD} update-soc-library
	exit 0
fi

# pack firmware
echo -e "\n--== Pack firmware for board ${BOARD} ==--\n\n"
make -C ${PRJ_BASE} BOARD=${BOARD} APP_CONFIG=${CONFIG} ECHECK_FW=${ECHECK_FW_NAME} firmware

#back up firmware
if [ "${BACKUP_FW}" = "1" ]; then
	backup_firmware_dir=${SDK_ROOT}/../Firmware_Release_${BOARD}
	mkdir -p ${backup_firmware_dir}
	echo "copy firmare to ${backup_firmware_dir} for autotest"
	cp -a ${PRJ_BASE}/outdir/${BOARD}/zephyr.elf ${backup_firmware_dir}/
	cp -a ${PRJ_BASE}/outdir/${BOARD}/zephyr.bin ${backup_firmware_dir}/
	cp -a ${PRJ_BASE}/outdir/${BOARD}/zephyr.map ${backup_firmware_dir}/
	cp -a ${PRJ_BASE}/outdir/${BOARD}/zephyr.lst ${backup_firmware_dir}/
	cp -a ${PRJ_BASE}/outdir/${BOARD}/_firmware/*.fw  ${backup_firmware_dir}/
	cp -a ${PRJ_BASE}/outdir/${BOARD}/_firmware/*raw.bin  ${backup_firmware_dir}/
	cp -a ${PRJ_BASE}/outdir/${BOARD}/_firmware/ota*.*  ${backup_firmware_dir}/
	cp -a ${PRJ_BASE}/outdir/${BOARD}/_firmware/fw_build_time.bin  ${backup_firmware_dir}/
	cp -a ${PRJ_BASE}/outdir/${BOARD}/include/generated/autoconf.h  ${backup_firmware_dir}/
fi;
