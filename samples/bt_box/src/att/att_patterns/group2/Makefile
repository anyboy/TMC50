.PHONY:  force_rm_linker rf_fcc rf_rx rf_tx clean

all: force_rm_linker rf_fcc rf_rx rf_tx

force_rm_linker:
	rm -rf link.ld

rf_fcc:
	cd ./rf_fcc && make || exit "$$?"

rf_rx:
	cd ./rf_rx && make || exit "$$?"

rf_tx:
	cd ./rf_tx && make || exit "$$?"

clean:
	cd ./rf_fcc && make clean
	cd ./rf_rx && make clean
	cd ./rf_tx && make clean
