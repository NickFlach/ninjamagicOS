# ninjamagic_larry.mk — Product configuration for OnePlus Nord N30
# ninjamagicOS build target: ninjamagic_larry-userdebug

$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit_only.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# Device-specific configuration
$(call inherit-product, device/oneplus/larry/BoardConfig.mk)

PRODUCT_NAME := ninjamagic_larry
PRODUCT_DEVICE := larry
PRODUCT_BRAND := ninjamagicOS
PRODUCT_MODEL := NinjaMagic Nord N30
PRODUCT_MANUFACTURER := OnePlus

# ninjamagicOS version
NINJAMAGIC_VERSION := 0.1.0
PRODUCT_PROPERTY_OVERRIDES += \
    ro.ninjamagic.version=$(NINJAMAGIC_VERSION) \
    ro.ninjamagic.device=larry \
    ro.ninjamagic.soc=snapdragon_695 \
    ro.ninjamagic.agent.model=llama-3.2-1b-q4_k_m \
    ro.ninjamagic.agent.accel=hexagon_dsp \
    ro.ninjamagic.msi.lanes_max=512 \
    ro.ninjamagic.msi.topics_max=32768

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
    vendor/models/llama-3.2-1b-q4_k_m.gguf:$(TARGET_COPY_OUT_VENDOR)/models/llama-3.2-1b-q4_k_m.gguf \
    vendor/models/all-MiniLM-L6-v2.onnx:$(TARGET_COPY_OUT_VENDOR)/models/all-MiniLM-L6-v2.onnx \
    vendor/models/intent-classifier.tflite:$(TARGET_COPY_OUT_VENDOR)/models/intent-classifier.tflite

# Init scripts
PRODUCT_COPY_FILES += \
    device/oneplus/larry/init.ninjamagic.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/init.ninjamagic.rc

# SELinux policy
BOARD_SEPOLICY_DIRS += device/oneplus/larry/sepolicy
