obj-m += rpi1_parking.o

KDIR:=~/linux-rpi/
ARM:= ARCH=arm CROSS_COMPILE=/usr/bin/arm-linux-gnueabi-
PWD:= $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) $(ARM) modules
local:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) $(ARM) clean
scp:
	scp $(obj-m:.o=.ko) pi@10.10.10.12:~/
	scp Makefile pi@10.10.10.12:~/
	scp rpi1_parking_mqtt.c pi@10.10.10.12:~/
# 유저스페이스 데몬 빌드
daemon:
	gcc -Wall -o rpi_parking_mqtt rpi1_parking_mqtt.c -lmosquitto
