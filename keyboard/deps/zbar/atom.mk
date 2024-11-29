LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := zbar
LOCAL_DESCRIPTION := Zbar Library
LOCAL_CATEGORY_PATH := libs

LOCAL_AUTOTOOLS_VERSION := 0.10
LOCAL_AUTOTOOLS_ARCHIVE := zbar-$(LOCAL_AUTOTOOLS_VERSION).tar.bz2
LOCAL_AUTOTOOLS_SUBDIR := zbar-$(LOCAL_AUTOTOOLS_VERSION)

# Add the include path for zbar
LOCAL_CFLAGS += -I$(LOCAL_PATH)/zbar-$(LOCAL_AUTOTOOLS_VERSION)/include

LOCAL_EXPORT_LDLIBS := -lzbar

LOCAL_CFLAGS += -D_POSIX_C_SOURCE=199309L

LOCAL_AUTOTOOLS_CONFIGURE_ARGS := \
    --without-imagemagick \
    --without-gtk \
    --without-jpeg \
    --without-python \
    --without-qt \
    --disable-video \
    --enable-static \


include $(BUILD_AUTOTOOLS)
