#Common headers
common_includes := hardware/qcom/display-legacy/libgralloc
common_includes += hardware/qcom/display-legacy/libgenlock
common_includes += hardware/qcom/display-legacy/liboverlay
common_includes += hardware/qcom/display-legacy/libcopybit
common_includes += hardware/qcom/display-legacy/libqdutils

ifeq ($(TARGET_USES_POST_PROCESSING),true)
    common_flags     += -DUSES_POST_PROCESSING
    common_includes += $(TARGET_OUT_HEADERS)/pp/inc
endif


#Common libraries external to display HAL
common_libs := liblog libutils libcutils libhardware

#Common C flags
common_flags := -DDEBUG_CALC_FPS -Wno-missing-field-initializers

ifeq ($(TARGET_USES_ION),true)
    common_flags += -DUSE_ION
endif

ifeq ($(ARCH_ARM_HAVE_NEON),true)
    common_flags += -D__ARM_HAVE_NEON
endif

ifeq ($(TARGET_NO_HW_VSYNC),true)
    common_flags += -DNO_HW_VSYNC
endif
