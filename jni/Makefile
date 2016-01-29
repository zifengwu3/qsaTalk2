######################################
#
#
######################################

GCC_PATH=/home/l/android/external-toolchain
  
#共享库文件名，lib*.a
TARGET_PATH := ../static_libs
TARGET  := libqsa_talk.a
SUPPORT_PATH := /media/HDDDISK/static_libs
TEST_PATH := ./TestDemo

#CROSS_COMPILE=$(GCC_PATH)/bin/arm-linux-gnueabi-
CROSS_COMPILE=
  
#compile and lib parameter
#编译参数
CC      := $(CROSS_COMPILE)gcc
AR      := $(CROSS_COMPILE)ar
RANLIB  := ranlib
LIBS    :=
LDFLAGS :=
DEFINES :=
INCLUDE := -I./include/ 
CFLAGS  := -g -Wall -O3 $(DEFINES) $(INCLUDE)
CXXFLAGS:= $(CFLAGS) -DHAVE_CONFIG_H
  
#source file
#源文件，自动找所有.c和.cpp文件，并将目标定义为同名.o文件
SOURCE  := $(wildcard *.c) $(wildcard *.cpp)
OBJS    := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))

.PHONY : everything objs clean veryclean rebuild
  
everything : $(TARGET)
  
all : $(TARGET)
  
objs : $(OBJS)
  
rebuild: veryclean everything gcc

clean:
	rm -fr *.o
    
veryclean : clean
	rm -fr $(TARGET)

gcc:
	$(CC) -pthread -o main $(TEST_PATH)/main.c \
						   $(TEST_PATH)/timer.c \
						   $(TEST_PATH)/cb_function.c \
						   $(TARGET)
ndk:
	ndk-build -B

run:
	./main
  
$(TARGET) : $(OBJS)
	$(AR) cru $(TARGET) $(OBJS)
	$(RANLIB) $(TARGET)
