LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := main.out
LOCAL_SRC_FILES :=  main.cpp
LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv3
include $(BUILD_EXECUTABLE)

