# Makefile for VMARC XAD client

CC ?= gcc
CFLAGS ?= -O2 -Wall -I. -Iinclude

# vbcc support. Override VBCC/VBCCFLAGS for a MorphOS or OS4 target.
VBCC ?= vc
VBCCFLAGS ?= -O2 -I. -Iinclude -DAMIGA

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

# Compile the client with vbcc. Use VBCCFLAGS for other targets, e.g.
# make vbcc VBCCFLAGS="-O2 -I. -Iinclude -D__MORPHOS__ -DSDI_MORPHOSNOREG"
vbcc:
	$(VBCC) $(VBCCFLAGS) -c VMARC.c -o VMARC.vbcc.o
	$(VBCC) $(VBCCFLAGS) -c extheader.c -o extheader.vbcc.o

clean:
	rm -f *.o *.vbcc.o test_vmarc

.PHONY: all test vbcc clean
