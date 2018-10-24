# Makefile for update_tool

TARGET = udisk_test

TARGET_USES_LOGD=true
WITH_MINGW :=

CSRC := ../util/arlog.c

CXXSRC := udisk_test.cpp
CXXSRC += ../util/stlog.cpp


CSRC += ../../liblog/logd_write.c
CSRC += ../../liblog/log_event_write.c
CSRC += ../../liblog/logprint.c
CSRC += ../../liblog/event_tag_map.c
CSRC += ../../liblog/log_read.c

CSRC += ../../libcutils/system_properties.c

CXXSRC += ../../liblog/log_time.cpp

CFLAGS += -fexceptions -Wall -Wunused-variable -Werror
CFLAGS += -DHAVE_SYS_UIO_H -DHAVE_ANDROID_OS -DHAVE_PTHREADS
CXXFLAGS += -std=c++11 -frtti -fexceptions  -Wall 

LDFLAGS :=

INCLUDE_PATH = -I ../include 
INCLUDE_PATH += -I ../../include
INCLUDE_PATH += -I ../../include/init

LIBS = -lrt -lpthread



TOOLCHAIN = aarch64-linux-gnu-
#TOOLCHAIN = /home/xlb/work/customer_24r5/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
CC=$(TOOLCHAIN)gcc
CXX=$(TOOLCHAIN)g++

COBJS=$(CSRC:.c=.o)
CXXOBJS=$(CXXSRC:.cpp=.o)

%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE_PATH)


%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE_PATH)

all:$(TARGET)

$(TARGET): $(COBJS) $(CXXOBJS)
	$(CXX) -o $@ $(LDFLAGS) $^ $(LIBS)
	#cp $(TARGET) ../../out/
	#cp $(TARGET) ../../out/update/
clean:
	rm $(shell find -name *.o)
	rm $(TARGET)

	
