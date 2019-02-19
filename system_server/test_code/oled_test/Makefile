
TARGET = oled_test

TARGET_USES_LOGD=true
WITH_MINGW :=

SRC_BASE := ../../

CSRC := $(SRC_BASE)util/arlog.c
CXXSRC += $(SRC_BASE)util/util.cpp
CXXSRC += $(SRC_BASE)util/stlog.cpp

CXXSRC += $(SRC_BASE)hw/oled_module.cpp
CXXSRC += $(SRC_BASE)hw/oled_light.cpp
CXXSRC += $(SRC_BASE)hw/ins_i2c.cpp
CXXSRC += $(SRC_BASE)hw/ins_gpio.cpp

#CXXSRC += $(SRC_BASE)oled_handler.cpp

CXXSRC += main.cpp

CFLAGS += -fexceptions -Wall -Wunused-variable -Werror
CFLAGS += -DHAVE_SYS_UIO_H -DHAVE_ANDROID_OS -DHAVE_PTHREADS
CXXFLAGS += -std=c++11 -frtti -fexceptions  -Wall 

LDFLAGS :=

INCLUDE_PATH = -I ../include 
INCLUDE_PATH += -I ../../include

LIBS = -lrt -lpthread -llog



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

	

