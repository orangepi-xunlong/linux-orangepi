# 32-bit ARM hard float compiler
ifeq ($(MULTIARCH),1)
 TARGET_SECONDARY_ARCH := target_armhf
else
 TARGET_PRIMARY_ARCH   := target_armhf
endif
