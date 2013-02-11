LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE  	      := scom
LOCAL_SRC_FILES       := assigners.cpp audio.cpp decoder.cpp encoders.cpp packeter.cpp scom.cpp spectrum.cpp sync.cpp util.cpp \
                         third_party/kiss_fft130/kiss_fft.c \
                         third_party/kiss_fft130/tools/kiss_fftr.c

LOCAL_C_INCLUDES      := $(LOCAL_PATH)/third_party/kiss_fft130 \
                         $(LOCAL_PATH)/third_party/kiss_fft130/tools

# for native audio
LOCAL_LDLIBS    += -lOpenSLES
# for logging
LOCAL_LDLIBS    += -llog
# for native asset manager
LOCAL_LDLIBS    += -landroid

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

include $(BUILD_STATIC_LIBRARY)
