# BoardConfig.mk — OnePlus Nord N30 5G (larry)
# ninjamagicOS device configuration

# Architecture
TARGET_ARCH := arm64
TARGET_ARCH_VARIANT := armv8-2a
TARGET_CPU_ABI := arm64-v8a
TARGET_CPU_VARIANT := kryo660

TARGET_2ND_ARCH := arm
TARGET_2ND_ARCH_VARIANT := armv8-a
TARGET_2ND_CPU_ABI := armeabi-v7a
TARGET_2ND_CPU_VARIANT := cortex-a55

# SoC
TARGET_BOARD_PLATFORM := sm6375
TARGET_SOC := snapdragon_695

# Bootloader
TARGET_BOOTLOADER_BOARD_NAME := larry
TARGET_NO_BOOTLOADER := false

# Kernel
BOARD_KERNEL_BASE := 0x00000000
BOARD_KERNEL_PAGESIZE := 4096
BOARD_KERNEL_IMAGE_NAME := Image
TARGET_KERNEL_SOURCE := kernel/oneplus/sm6375
TARGET_KERNEL_CONFIG := ninjamagic_larry_defconfig
BOARD_KERNEL_CMDLINE := \
    console=ttyMSM0,115200n8 \
    androidboot.hardware=qcom \
    androidboot.memcg=1 \
    lpm_levels.sleep_disabled=1 \
    msm_rtb.filter=0x237 \
    service_locator.enable=1 \
    androidboot.usbcontroller=4e00000.dwc3

# Partitions
BOARD_BOOTIMAGE_PARTITION_SIZE := 100663296      # 96MB
BOARD_DTBOIMG_PARTITION_SIZE := 25165824         # 24MB
BOARD_SUPER_PARTITION_SIZE := 10200547328
BOARD_SUPER_PARTITION_GROUPS := oneplus_dynamic_partitions
BOARD_ONEPLUS_DYNAMIC_PARTITIONS_PARTITION_LIST := system system_ext product vendor odm
BOARD_ONEPLUS_DYNAMIC_PARTITIONS_SIZE := 10196353024

# Filesystem
TARGET_USERIMAGES_USE_EXT4 := true
TARGET_USERIMAGES_USE_F2FS := true
BOARD_USERDATAIMAGE_FILE_SYSTEM_TYPE := f2fs

# Display
TARGET_SCREEN_DENSITY := 391
TARGET_SCREEN_WIDTH := 1080
TARGET_SCREEN_HEIGHT := 2400

# GPU
BOARD_GPU_TYPE := adreno-619

# NPU / ML Accelerator
BOARD_HAS_NPU := false
BOARD_HAS_DSP := true
BOARD_DSP_TYPE := hexagon_686

# Radio / Modem
BOARD_MODEM_TYPE := snapdragon_x51
ENABLE_VENDOR_RIL_SERVICE := true

# Security
BOARD_AVB_ENABLE := true
BOARD_USES_QCOM_SPU := true

# MSI (ninjamagicOS cognitive runtime)
BOARD_MSI_KERNEL_MODULE := true
BOARD_MSI_LANES_MAX := 512
BOARD_MSI_EVENTS_MAX_TOPICS := 32768
BOARD_MSI_ASSOC_DSP_ACCEL := true

# SELinux
BOARD_SEPOLICY_DIRS += device/oneplus/larry/sepolicy

# WiFi
BOARD_WLAN_DEVICE := qcwcn
BOARD_WPA_SUPPLICANT_DRIVER := NL80211

# Bluetooth
BOARD_HAVE_BLUETOOTH := true
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/oneplus/larry/bluetooth

# SD Card
BOARD_HAS_SDCARD := true
BOARD_SDCARD_MAX_SIZE := 1099511627776  # 1TB
