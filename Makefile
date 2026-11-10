# Makefile for VMARC XAD client

CC ?= gcc
CFLAGS ?= -O2 -Wall -I. -Iinclude

all: test_vmarc

test_vmarc: test_vmarc.o VMARC.o
	$(CC) $(CFLAGS) -o $@ $^

test_vmarc.o: test_vmarc.c VMARC.h
	$(CC) $(CFLAGS) -c test_vmarc.c

VMARC.o: VMARC.c VMARC.h xadClient.h SDI_compiler.h ConvertE.c include/libraries/xadmaster.h
	$(CC) $(CFLAGS) -c VMARC.c

extheader.o: extheader.c include/libraries/xadmaster.h SDI_compiler.h
	$(CC) $(CFLAGS) -c extheader.c

test: test_vmarc
	./test_vmarc

clean:
	rm -f *.o test_vmarc

.PHONY: all test clean
