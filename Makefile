CC := gcc
CFLAGS := ""

default: all

all: launcher.o controller.o

%.o: %.c
	$(CC) -c $(CLAGS) -o $@ $<

clean:
	rm -f *.o
