.PHONY:  bt_attr_modify bt_attr_read bt_link rf_bqb gpio led aging_playback audio key bat exit_mode clean

all: bt_attr_modify bt_attr_read bt_link rf_bqb gpio led aging_playback key bat audio exit_mode

bt_attr_modify:
	cd ./bt_attr_modify && make || exit "$$?"

bt_attr_read:
	cd ./bt_attr_read && make || exit "$$?"

bt_link:
	cd ./bt_link && make || exit "$$?"

rf_bqb:
	cd ./rf_bqb && make || exit "$$?"
	
led:
	cd ./led && make || exit "$$?"

gpio:
	cd ./gpio && make || exit "$$?"
	
aging_playback:
	cd ./aging_playback && make || exit "$$?"

audio:
	cd ./audio && make || exit "$$?"

key:
	cd ./key && make || exit "$$?"

bat:
	cd ./bat && make || exit "$$?"

exit_mode:
	cd ./exit_mode && make || exit "$$?"

clean:
	cd ./bt_attr_modify && make clean
	cd ./bt_attr_read && make clean
	cd ./bt_link && make clean
	cd ./rf_bqb && make clean
	cd ./gpio && make clean
	cd ./led && make clean
	cd ./aging_playback && make clean
	cd ./audio && make clean
	cd ./key && make clean
	cd ./bat && make clean
	cd ./exit_mode && make clean
