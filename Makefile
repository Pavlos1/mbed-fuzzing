CC := gcc
CFLAGS := ""
BASE_TARGETS := launcher.o controller.o util.o elf.o

default: all

test: all test.o
	$(CC) $(BASE_TARGETS) test.o -o $@
	./$@
	

all: $(BASE_TARGETS)

%.o: %.c
	$(CC) -c $(CLAGS) -o $@ $<

clean:
	rm -f *.o
