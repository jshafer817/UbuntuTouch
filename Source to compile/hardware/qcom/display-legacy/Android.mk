ifeq ($(call is-vendor-board-platform,QCOM),true)
ifeq ($(TARGET_QCOM_DISPLAY_VARIANT),legacy)
display-hals := libgralloc libgenlock libcopybit
display-hals += libhwcomposer liboverlay libqdutils
ifneq ($(TARGET_PROVIDES_LIBLIGHT),true)
display-hals += liblight
endif
include $(call all-named-subdir-makefiles,$(display-hals))
endif
endif
