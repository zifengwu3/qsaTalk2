LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE := qsatalk
LOCAL_MODULE_TAGS := optional

QSA_TALK_PATH := $(LOCAL_PATH)/../../../

LOCAL_C_INCLUDES := $(QSA_TALK_PATH)/include

LOCAL_SRC_FILES := $(QSA_TALK_PATH)/cb_function.c \
                   $(QSA_TALK_PATH)/init.c \
                   $(QSA_TALK_PATH)/talk.c \
                   $(QSA_TALK_PATH)/timer.c \
                   $(QSA_TALK_PATH)/udp.c

LOCAL_LDFLAGS :=  -ldl -lm -lc  -lz -llog

include $(BUILD_STATIC_LIBRARY)
