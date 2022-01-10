.PHONY: make_bin_dir copy_conf_h gen_apis gen_dir lib drivers group1 group2 make_cfg make_atf

all: clean make_bin_dir copy_conf_h gen_apis gen_dir lib drivers group1 group2 make_cfg make_atf

BOARD ?= ats2853_dvb
Firmware_Release_Dir ?= ./../../../../../../Firmware_Release_ats2853_dvb

make_bin_dir:
	mkdir -p ./../att_bin/outbin

copy_conf_h:
	cp -rf $(Firmware_Release_Dir)/autoconf.h ./include

gen_apis:
	readelf -s -W $(Firmware_Release_Dir)/zephyr.elf > symbols.txt
	gcc ./scripts/gen_att_export_apis.c -o ./scripts/gen_att_export_apis.out
	./scripts/gen_att_export_apis.out ./symbols.txt ./include/att_export_apis.h
	rm -rf ./symbols.txt

gen_dir:
	mkdir -p ./../att_bin/outbin

lib:
	cd ./lib && make || exit "$$?"

#drivers:
#	cd ./drivers && make || exit "$$?"

group1:
	cd ./group1 && make || exit "$$?"

group2:
	cd ./group2 && make || exit "$$?"

make_cfg:
	gcc ./scripts/make_cfg.c -o ./scripts/make_cfg.out
	./scripts/make_cfg.out ./../att_bin/atf_make.cfg ./include/att_export_apis.h ./../att_bin/atf_make_out.cfg

make_atf:
	python ./scripts/build_atf.py -c ./../att_bin/atf_make_out.cfg -t $(Firmware_Release_Dir)/fw_build_time.bin -o ./../att_bin/actionstest_285a.atf

clean:
	cd ./lib && make clean
#	cd ./drivers && make clean
	cd ./group1 && make clean
	cd ./group2 && make clean
	rm -rf ./scripts/*.out
	rm -rf ./include/att_export_apis.h
	rm -rf ./include/autoconf.h
	rm -rf ../att_bin/atf_make_out.cfg
