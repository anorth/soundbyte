LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := scomjni
LOCAL_SRC_FILES := scomjni.cpp droidlog.cpp

LOCAL_LDLIBS := -llog
LOCAL_STATIC_LIBRARIES += scom

include $(BUILD_SHARED_LIBRARY)

$(call import-module,native)
