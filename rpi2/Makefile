CC = gcc
CFLAGS = -Wall -pthread
LIBS = -lmosquitto -lpthread

TARGET = gate_node
SRCS = main.c gate_controller.c switch_handler.c mqtt_client.c ultrasonic.c buzzer.c
OBJS = $(SRCS:.c=.o)

# RPI2 배포 설정
RPI2_USER = pi
RPI2_IP   = 10.10.10.12
RPI2_DIR  = ~/

.PHONY: all clean scp scp-src

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

scp:
	scp $(TARGET) $(RPI2_USER)@$(RPI2_IP):$(RPI2_DIR)

scp-src:
	scp *.c *.h Makefile $(RPI2_USER)@$(RPI2_IP):$(RPI2_DIR)rpi2/
