LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := main.out
LOCAL_SRC_FILES :=  main.cpp
LOCAL_CFLAGS += -O2 -DSTATIC_READ_AMOUNT=${STATIC_READ_AMOUNT}
LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv3
include $(BUILD_EXECUTABLE)

