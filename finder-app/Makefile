# CC = aarch64-none-linux-gnu-gcc
CC =gcc
# CROSS_COMPILE_FLAGS = -print-sysroot -v
CROSS_COMPILE_FLAGS = 
CFLAGS = -Wall -g
TARGET = writer

all: $(TARGET)

writer: writer.c
	 $(CROSS_COMPILE)$(CC) $(CROSS_COMPILE_FLAGS) $(CFLAGS) -o $(TARGET) writer.c

clean:
	rm -f $(TARGET)
