# MSI Kernel Module Makefile
# ninjamagicOS Minimal Substrate Interface

obj-$(CONFIG_MSI_SUBSTRATE) += msi.o

msi-y := msi_module.o \
         msi_domain.o \
         msi_lane.o \
         msi_event.o \
         msi_state.o \
         msi_ioctl.o

msi-$(CONFIG_MSI_NPU_ACCEL) += msi_npu.o

ccflags-y += -DMSI_MAX_LANES=$(CONFIG_MSI_MAX_LANES)
ccflags-y += -DMSI_MAX_DOMAINS=$(CONFIG_MSI_MAX_DOMAINS)
ccflags-y += -DMSI_MAX_TOPICS=$(CONFIG_MSI_MAX_TOPICS)
ccflags-$(CONFIG_MSI_DEBUG) += -DMSI_DEBUG
