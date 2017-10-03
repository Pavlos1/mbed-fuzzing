CC := gcc
CFLAGS := -O3
CPPFLAGS := -DLOG_WARN -DLOG_FATAL
BASE_TARGETS := launcher.o controller.o util.o elf.o memprotect.o

default: all

test: all test.o
	$(CC) $(BASE_TARGETS) test.o $(CFLAGS) $(CPPFLAGS) -o $@
	./$@
	

all: $(BASE_TARGETS)

%.o: %.c
	$(CC) -c $(CLAGS) $(CFLAGS) $(CPPFLAGS) -o $@ $<

clean:
	rm -f *.o
