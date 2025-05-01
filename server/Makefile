# CC = aarch64-none-linux-gnu-gcc
CC ?=$(TARGET_CC)
# CROSS_COMPILE_FLAGS = -print-sysroot -v
CROSS_COMPILE_FLAGS = 
CFLAGS = -Wall -g
TARGET = aesdsocket

all: $(TARGET)

aesdsocket: aesdsocket.c
	 $(CC) $(CROSS_COMPILE_FLAGS) $(CFLAGS) ${LDFLAGS} -o $@ $<

clean:
	rm -f $(TARGET)
