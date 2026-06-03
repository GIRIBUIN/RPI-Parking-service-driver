# ARM 크로스 컴파일 설정 (32-bit Raspberry Pi OS)
ARCH          := arm
CROSS_COMPILE := arm-linux-gnueabi-
RPI_KDIR      ?= $(HOME)/linux-rpi

obj-m := gate_node.o
gate_node-objs := gate_main.o gate_hw.o

RPI2_USER = pi
RPI2_IP   = 10.10.10.12
RPI2_DIR  = ~/
RPI2_SRC  = ~/rpi2/

.PHONY: all module bridge clean get-headers scp scp-src help

# 기본: 크로스 컴파일로 ARM 커널 모듈만 빌드
all: module

# 크로스 컴파일로 gate_node.ko 생성
module:
	$(MAKE) -C $(RPI_KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

# Ubuntu 로컬 mqtt_bridge 빌드 (테스트용)
bridge: mqtt_bridge

mqtt_bridge: mqtt_bridge.c
	gcc -Wall -pthread -o mqtt_bridge mqtt_bridge.c -lmosquitto

clean:
	$(MAKE) -C $(RPI_KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
	rm -f mqtt_bridge *.o

# 최초 1회: RPI 커널 헤더를 Ubuntu로 복사
# 선행 조건: RPI에서 sudo apt install raspberrypi-kernel-headers 실행
get-headers:
	@RPI_KVER=$$(ssh $(RPI2_USER)@$(RPI2_IP) uname -r); \
	echo "RPI kernel version: $$RPI_KVER"; \
	mkdir -p $(RPI_KDIR); \
	rsync -avL $(RPI2_USER)@$(RPI2_IP):/lib/modules/$$RPI_KVER/build/ $(RPI_KDIR)/

# 크로스 컴파일된 gate_node.ko를 RPI2로 전송
scp: module
	scp gate_node.ko $(RPI2_USER)@$(RPI2_IP):$(RPI2_DIR)

# mqtt_bridge 소스를 RPI2로 전송 (RPI에서 직접 빌드)
scp-src:
	scp mqtt_bridge.c mqtt_client.c mqtt_client.h config.h $(RPI2_USER)@$(RPI2_IP):$(RPI2_SRC)

help:
	@echo "RPI2 Gate Node Module - Cross Compile Build"
	@echo ""
	@echo "=== 최초 1회 설정 ==="
	@echo "  1. sudo apt install gcc-arm-linux-gnueabihf"
	@echo "  2. RPI에서: sudo apt install raspberrypi-kernel-headers"
	@echo "  3. make get-headers  # RPI 커널 헤더 다운로드"
	@echo ""
	@echo "=== 빌드 및 배포 ==="
	@echo "  make              - 크로스 컴파일로 gate_node.ko 생성"
	@echo "  make scp          - gate_node.ko를 RPI2로 전송"
	@echo "  make scp-src      - mqtt_bridge 소스를 RPI2로 전송"
	@echo ""
	@echo "=== RPI2에서 ==="
	@echo "  sudo insmod gate_node.ko"
	@echo "  sudo rmmod gate_node"
	@echo ""
	@echo "=== 추가 타겟 ==="
	@echo "  make module       - 커널 모듈만 빌드"
	@echo "  make bridge       - mqtt_bridge만 빌드 (로컬 테스트)"
	@echo "  make clean        - 빌드 산출물 정리"
