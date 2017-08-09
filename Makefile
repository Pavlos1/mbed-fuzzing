CC := gcc
CFLAGS := ""

default: all

test: all test.o
	$(CC) controller.o launcher.o test.o -o $@
	

all: launcher.o controller.o

%.o: %.c
	$(CC) -c $(CLAGS) -o $@ $<

clean:
	rm -f *.o
