######################################
#
#
######################################
  
#共享库文件名，lib*.a
TARGET_PATH := ../static_libs
TARGET  := $(TARGET_PATH)/libqsa_talk.a
  
#compile and lib parameter
#编译参数
CC      := gcc
AR      = ar
RANLIB  = ranlib
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
  
rebuild: veryclean everything
                
clean :
	rm -fr *.o
    
veryclean : clean
	rm -fr $(TARGET)
  
$(TARGET) : $(OBJS)
	$(AR) cru $(TARGET) $(OBJS)
	$(RANLIB) $(TARGET)
