# Makefile for setprop getprop

all: setprop getprop

TOOLCHAIN = aarch64-linux-gnu-
CC=$(TOOLCHAIN)gcc
CXX=$(TOOLCHAIN)g++

CFLAGS += -fPIC -O2 -g -Wall -Wno-unused-parameter
CFLAGS += -D__LP64__ -DHAVE_SYS_UIO_H -DHAVE_ANDROID_OS -DHAVE_PTHREADS

LIBS = -lcutils


INCLUDE_PATH = -I ../include
INCLUDE_PATH += -I ../include/init
INCLUDE_PATH += -I ./
INCLUDE_PATH += -I ./include

setprop: setprop.c
	$(CC) -o $@ $< $(CFLAGS) $(INCLUDE_PATH)  $(LIBS)

getprop: getprop.c
	$(CC) -o $@ $< $(CFLAGS) $(INCLUDE_PATH)  $(LIBS)

clean:
	rm -f *.o




