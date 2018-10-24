# Makefile for update_tool

TARGET = update_tool

TARGET_USES_LOGD=true
WITH_MINGW :=


CXXSRC := update_tool.cpp
CXXSRC += update_util.cpp
CXXSRC += md5.cpp

CFLAGS += -fexceptions -Wall -Wunused-variable -Werror
CFLAGS += -DHAVE_SYS_UIO_H -DHAVE_ANDROID_OS -DHAVE_PTHREADS
CXXFLAGS += -std=c++11 -frtti -fexceptions  -Wall 

LDFLAGS :=

INCLUDE_PATH = -I ./include 
INCLUDE_PATH += -I ../pro2_service/include

LIBS = -lrt -lpthread 
#-llog -lev



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
	cp $@ /usr/local/bin
clean:
	rm *.o $(TARGET)

	
