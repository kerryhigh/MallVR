# MallVR for Google Cardboard phones: the MallVR DOSBox-X tree + SDL2 + gl4es + Google Cardboard.
# vpix_render.cpp is its own module, built with _WIN32 + compat/ (fake Windows -> EGL / gl4es), so its Windows code runs as is.
LOCAL_PATH := $(call my-dir)
TOP := $(abspath $(LOCAL_PATH))
COMPAT := $(TOP)/../compat
CARDBOARD_SDK := $(CARDBOARD_SDK_DIR)
CARDBOARDINC := $(CARDBOARD_SDK)/include $(CARDBOARD_SDK)

include $(SDL)/Android.mk
include $(GL4ES)/Android.mk

# Google Cardboard's phone SDK, limited to its Android/OpenGL sources. The
# Gradle AAR supplies its Java classes/resources; this target supplies JNI.
LOCAL_PATH := $(CARDBOARD_SDK)
include $(CLEAR_VARS)
LOCAL_MODULE := GfxPluginCardboard
LOCAL_SRC_FILES := \
    $(patsubst $(CARDBOARD_SDK)/%,%,$(wildcard $(CARDBOARD_SDK)/qrcode/cardboard_v1/*.cc)) \
    $(patsubst $(CARDBOARD_SDK)/%,%,$(wildcard $(CARDBOARD_SDK)/*.cc)) \
    $(patsubst $(CARDBOARD_SDK)/%,%,$(wildcard $(CARDBOARD_SDK)/sensors/*.cc)) \
    $(patsubst $(CARDBOARD_SDK)/%,%,$(wildcard $(CARDBOARD_SDK)/sensors/android/*.cc)) \
    $(patsubst $(CARDBOARD_SDK)/%,%,$(wildcard $(CARDBOARD_SDK)/jni_utils/android/*.cc)) \
    $(patsubst $(CARDBOARD_SDK)/%,%,$(wildcard $(CARDBOARD_SDK)/util/*.cc)) \
    $(patsubst $(CARDBOARD_SDK)/%,%,$(wildcard $(CARDBOARD_SDK)/qrcode/android/*.cc)) \
    $(patsubst $(CARDBOARD_SDK)/%,%,$(wildcard $(CARDBOARD_SDK)/screen_params/android/*.cc)) \
    $(patsubst $(CARDBOARD_SDK)/%,%,$(wildcard $(CARDBOARD_SDK)/device_params/android/*.cc)) \
    $(patsubst $(CARDBOARD_SDK)/%,%,$(wildcard $(CARDBOARD_SDK)/rendering/opengl_*.cc))
LOCAL_C_INCLUDES := $(CARDBOARDINC)
LOCAL_CFLAGS := -O2 -w
LOCAL_CPPFLAGS := -std=gnu++17 -fexceptions -frtti
# The upstream export map also requires Unity JNI and Vulkan plugin entry points.
# This phone-only target builds Cardboard's C/OpenGL API and deliberately omits those plugins.
LOCAL_LDLIBS := -landroid -lGLESv2 -lGLESv3 -llog -ldl
include $(BUILD_SHARED_LIBRARY)
include $(CLEAR_VARS)
LOCAL_PATH := $(DBX)
LOCAL_MODULE := vpixrender
LOCAL_SRC_FILES := src/hardware/vpix_render.cpp
LOCAL_C_INCLUDES := $(COMPAT) $(GL4ES)/include $(DBX)/src/hardware $(CARDBOARDINC) $(DBX)/include $(SDL)/include
LOCAL_CFLAGS := -O2 -w -include $(COMPAT)/win32_prelude.h -fdeclspec -fms-extensions
LOCAL_CPPFLAGS := -std=gnu++14 -fexceptions -frtti
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_PATH := $(DBX)
LOCAL_MODULE := main
LOCAL_SRC_FILES := $(shell cat $(TOP)/../../dosbox_android/jni/sources.txt) \
    src/hardware/vpix.cpp src/hardware/vpix_hle.cpp src/hardware/vpix_net.cpp \
    $(patsubst $(DBX)/%,%,$(wildcard $(DBX)/src/hardware/su2000/*.cpp)) \
    src/hardware/vpix_android.cpp src/hardware/vpix_cardboard.cpp src/hardware/android_stubs.cpp
LOCAL_C_INCLUDES := $(TOP)/../../dosbox_android $(DBX)/include $(DBX)/src $(DBX)/src/cpu $(DBX)/src/aviwriter \
    $(DBX)/src/hardware/snd_pc98/cbus $(DBX)/src/hardware/snd_pc98/common $(DBX)/src/hardware/snd_pc98/generic \
    $(DBX)/src/hardware/snd_pc98/sound $(DBX)/src/hardware/snd_pc98/x11 $(DBX)/src/hardware/snd_pc98/sound/getsnd \
    $(SDL)/include $(DBX)/vs/libpng $(DBX)/src/hardware $(DBX)/src/hardware/su2000 $(COMPAT) $(CARDBOARDINC)
LOCAL_CFLAGS := -O2 -w -DNDEBUG -D_FILE_OFFSET_BITS=64 -DC_SDL2=1 -DRESDIR=\"/sdcard/MallVR\" -fno-strict-aliasing -DPNG_ARM_NEON_OPT=0 -D_stricmp=strcasecmp -D_strnicmp=strncasecmp
LOCAL_CPPFLAGS := -std=gnu++14 -fexceptions -frtti
LOCAL_STATIC_LIBRARIES := vpixrender GL
LOCAL_SHARED_LIBRARIES := SDL2 GfxPluginCardboard
LOCAL_LDFLAGS := -Wl,--error-limit=0 -Wl,--no-dependent-libraries
LOCAL_LDLIBS := -llog -landroid -lz -lEGL -lGLESv3 -ldl
LOCAL_SHORT_COMMANDS := true
include $(BUILD_SHARED_LIBRARY)
