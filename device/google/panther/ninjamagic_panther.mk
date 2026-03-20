# ninjamagic_panther.mk — Product configuration for Pixel 7
# ninjamagicOS build target: ninjamagic_panther-userdebug

$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit_only.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# Device-specific configuration
$(call inherit-product, device/google/panther/BoardConfig.mk)

PRODUCT_NAME := ninjamagic_panther
PRODUCT_DEVICE := panther
PRODUCT_BRAND := ninjamagicOS
PRODUCT_MODEL := NinjaMagic Pixel 7
PRODUCT_MANUFACTURER := Google

# ninjamagicOS version
NINJAMAGIC_VERSION := 0.1.0
PRODUCT_PROPERTY_OVERRIDES += \
    ro.ninjamagic.version=$(NINJAMAGIC_VERSION) \
    ro.ninjamagic.device=panther \
    ro.ninjamagic.soc=tensor_gs201 \
    ro.ninjamagic.agent.model=llama-3.2-3b-q4_k_m \
    ro.ninjamagic.agent.accel=tpu \
    ro.ninjamagic.msi.lanes_max=1024 \
    ro.ninjamagic.msi.topics_max=65536

# MSI kernel module
PRODUCT_PACKAGES += \
    msi.ko

# NinjaMagic Agent
PRODUCT_PACKAGES += \
    ninjamagic-agent \
    msi-probe

# Radio
PRODUCT_PACKAGES += \
    ninjamagic-rild

# On-device LLM models (shipped in /vendor/models/)
PRODUCT_COPY_FILES += \
    vendor/models/llama-3.2-3b-q4_k_m.gguf:$(TARGET_COPY_OUT_VENDOR)/models/llama-3.2-3b-q4_k_m.gguf \
    vendor/models/all-MiniLM-L6-v2.onnx:$(TARGET_COPY_OUT_VENDOR)/models/all-MiniLM-L6-v2.onnx \
    vendor/models/intent-classifier.tflite:$(TARGET_COPY_OUT_VENDOR)/models/intent-classifier.tflite

# Init scripts
PRODUCT_COPY_FILES += \
    device/google/panther/init.ninjamagic.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/init.ninjamagic.rc

# SELinux policy
BOARD_SEPOLICY_DIRS += device/google/panther/sepolicy
