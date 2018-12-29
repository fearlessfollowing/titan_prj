# Makefile for update_tool

TARGET = update_titan_tool

TARGET_USES_LOGD=true
WITH_MINGW :=


CXXSRC := update_tool.cpp
CXXSRC += md5.cpp

CFLAGS += -fexceptions -Wall -Wunused-variable -Werror
CFLAGS += -DHAVE_SYS_UIO_H -DHAVE_ANDROID_OS -DHAVE_PTHREADS
CXXFLAGS += -std=c++11 -frtti -fexceptions  -Wall 

LDFLAGS :=

INCLUDE_PATH = -I ./include 
INCLUDE_PATH += -I ../include 
INCLUDE_PATH += -I ../ui_service/include
LIBS = -lrt -lpthread 


TOOLCHAIN =
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

	
