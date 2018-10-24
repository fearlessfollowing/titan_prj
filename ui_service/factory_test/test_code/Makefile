# Makefile for liblog.so

TARGET = factory_test

TARGET_USES_LOGD=true
WITH_MINGW :=

CSRC := ../util/arlog.c
CXXSRC := ../util/ARLooper.cpp
CXXSRC += ../util/ARMessage.cpp
CXXSRC += ../util/bytes_int_convert.cpp
CXXSRC += ../util/util.cpp
CXXSRC += ../util/stlog.cpp
CSRC += ../util/cJSON.c
CXXSRC += ../util/pro_version.cpp

CXXSRC += ../hw/oled_module.cpp
CXXSRC += ../hw/oled_light.cpp
CXXSRC += ../hw/battery_interface.cpp
CXXSRC += ../hw/ins_i2c.cpp
CXXSRC += ../hw/ins_gpio.cpp

#CXXSRC += trans/SocketClient.cpp
#CXXSRC += trans/SocketListener.cpp
#CXXSRC += trans/FrameworkListener.cpp

CXXSRC += factory_test.cpp
CXXSRC += trans.cpp
#CXXSRC += UiCore.cpp
CXXSRC += main.cpp

CFLAGS += -fexceptions -Wall -Wunused-variable -Werror
CFLAGS += -DHAVE_SYS_UIO_H -DHAVE_ANDROID_OS -DHAVE_PTHREADS
CXXFLAGS += -std=c++11 -frtti -fexceptions  -Wall 

LDFLAGS :=

INCLUDE_PATH = -I ./include 
INCLUDE_PATH += -I ./
INCLUDE_PATH += -I ../include
INCLUDE_PATH += -I ../../include

LIBS = -lrt -lpthread -llog -lev



TOOLCHAIN =
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
clean:
	rm *.o $(TARGET)

	
