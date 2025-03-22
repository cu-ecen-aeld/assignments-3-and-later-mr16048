# CC = aarch64-none-linux-gnu-gcc
CC =gcc
# CROSS_COMPILE_FLAGS = -print-sysroot -v
CROSS_COMPILE_FLAGS = 
CFLAGS = -Wall -g
TARGET = aesdsocket

all: $(TARGET)

aesdsocket: aesdsocket.c
	 $(CROSS_COMPILE)$(CC) $(CROSS_COMPILE_FLAGS) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)
