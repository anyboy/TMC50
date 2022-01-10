#!/bin/bash

#仅供版本发布时使用

SDK_ROOT=`pwd`
TAG_NAME=$1
echo ${TAG_NAME}

release_dir=$PWD/../release_for_${TAG_NAME}
target_sample=bt_box
target_board=ats2853_dvb
target_sdk=SDK-${TAG_NAME}
release_firmware_dir=${release_dir}/Firmware-${TAG_NAME}
release_out_dir=${PWD}/samples/${target_sample}/outdir/${target_board}
BUILD_CONFIG_FILE=${SDK_ROOT}/.build_config
BOARD_CONFIG_SUBDIR_NAME_LIST='dvb dvb_full_function dvb_1m_nor_recovery dvb_background_bt_ota dvb_background_record dvb_gma dvb_i2s_out dvb_local_music dvb_a2dp_aac dvb_low_latency dvb_double_phone_ext_mode'

rm -rf ${release_dir}
echo "--${release_dir}--"
mkdir -p ${release_dir}

rm -rf ${PWD}/samples/${target_sample}/outdir

for i in $BOARD_CONFIG_SUBDIR_NAME_LIST; do

	echo "Building ${i} Firmware..."
	echo "# Build config" > ${BUILD_CONFIG_FILE}
	echo "BOARD=${target_board}" >> ${BUILD_CONFIG_FILE}
	echo "APPLICATION=${target_sample}" >> ${BUILD_CONFIG_FILE}
	echo "CONFIG=${i}" >> ${BUILD_CONFIG_FILE}
	mkdir -p ${release_dir}/${i}
	./build.sh
	cp -a ${release_out_dir}/zephyr.elf ${release_dir}/${i}/
	cp -a ${release_out_dir}/zephyr.bin ${release_dir}/${i}/
	cp -a ${release_out_dir}/zephyr.map ${release_dir}/${i}/
	cp -a ${release_out_dir}/zephyr.lst ${release_dir}/${i}/
	cp -a ${release_out_dir}/.config ${release_dir}/${i}/
	cp -a ${release_out_dir}/_firmware ${release_dir}/${i}/

	rm -rf ${PWD}/samples/${target_sample}/outdir
done

	echo "Building ${i} Firmware..."
	echo "# Build config" > ${BUILD_CONFIG_FILE}
	echo "BOARD=ats2853_dvb_512k" >> ${BUILD_CONFIG_FILE}
	echo "APPLICATION=${target_sample}" >> ${BUILD_CONFIG_FILE}
	echo "CONFIG=dvb_512K" >> ${BUILD_CONFIG_FILE}
	mkdir -p ${release_dir}/dvb_512k
	./build.sh
	cp -a ${PWD}/samples/${target_sample}/outdir/ats2853_dvb_512k/zephyr.elf ${release_dir}/dvb_512k/
	cp -a ${PWD}/samples/${target_sample}/outdir/ats2853_dvb_512k/zephyr.bin ${release_dir}/dvb_512k/
	cp -a ${PWD}/samples/${target_sample}/outdir/ats2853_dvb_512k/zephyr.map ${release_dir}/dvb_512k/
	cp -a ${PWD}/samples/${target_sample}/outdir/ats2853_dvb_512k/zephyr.lst ${release_dir}/dvb_512k/
	cp -a ${PWD}/samples/${target_sample}/outdir/ats2853_dvb_512k/.config ${release_dir}/dvb_512k/
	cp -a ${PWD}/samples/${target_sample}/outdir/ats2853_dvb_512k/_firmware ${release_dir}/dvb_512k/

	rm -rf ${PWD}/samples/${target_sample}/outdir

echo "All done"
