# Android.mk — Build NinjaMagic RILD

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := ninjamagic-rild
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/bin

LOCAL_SRC_FILES := \
    main.cpp \
    msi_bridge.cpp \
    vendor_ril.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    hardware/ril/include

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libutils \
    libcutils \
    libdl

LOCAL_CFLAGS := \
    -Wall -Werror \
    -DANDROID \
    -DLOG_TAG=\"NinjaMagicRILD\"

LOCAL_INIT_RC := ../../../device/$(TARGET_DEVICE_DIR)/init.ninjamagic.rc

include $(BUILD_EXECUTABLE)
