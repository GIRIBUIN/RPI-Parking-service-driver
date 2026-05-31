CC = gcc
CFLAGS = -Wall -pthread
LIBS = -lmosquitto -lpthread

TARGET = gate_node
SRCS = main.c gate_controller.c switch_handler.c mqtt_client.c ultrasonic.c buzzer.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
