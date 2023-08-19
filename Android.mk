LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cpp .cc
LOCAL_SRC_FILES := main.cpp sa_scripting.cpp mod/logger.cpp
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
    LOCAL_MODULE    := SAUtils
    LOCAL_SRC_FILES += sautils.cpp sautils_2_10.cpp
else
    LOCAL_MODULE    := SAUtils64
    LOCAL_SRC_FILES += sautils_2_10_x64.cpp
endif
LOCAL_CFLAGS += -O2 -mfloat-abi=softfp -DNDEBUG
LOCAL_C_INCLUDES += ./include
LOCAL_LDLIBS += -llog # ARM64 library requires it so...
include $(BUILD_SHARED_LIBRARY)
