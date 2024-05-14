LOCAL_SDK_PATH=/opt/pico-sdk
CONCURRENT_NUM=4

.PHONY: default build clean

default : build-on-fresh-sdk

build-on-local-sdk :
	cd build ; PICO_SDK_PATH=$(LOCAL_SDK_PATH) cmake -DPICO_COPY_TO_RAM=1 ..
	make -C build -j $(CONCURRENT_NUM)

build-on-fresh-sdk :
	cd build ; PICO_SDK_FETCH_FROM_GIT=1 cmake -DPICO_COPY_TO_RAM=1 ..
	make -C build -j $(CONCURRENT_NUM)

clean :
	rm -rf build/*
