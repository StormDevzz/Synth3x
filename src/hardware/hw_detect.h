#ifndef HW_DETECT_H
#define HW_DETECT_H

#include <stdint.h>

/* CPUID (assembly in hw_cpuid.S) */
char    *hw_cpuid_vendor(char buf[13]);
uint64_t hw_cpuid_features(void);
int      hw_cpuid_has_avx(void);
int      hw_cpuid_has_sse2(void);
char    *hw_cpuid_brand_string(char buf[49]);

/* USB/PCI vendor detection helpers */
const char *hw_vendor_name(uint16_t vendor_id);
const char *hw_device_class_name(uint16_t class_code);

/* Returns 1 if touchpad-like device found in /proc/bus/input/devices */
int hw_has_touchpad(void);

/* Returns 1 if Synaptics touchpad */
int hw_has_synaptics(void);

/* Returns vendor string from DMI (e.g., "LENOVO", "ACER", "DELL") */
int hw_dmi_vendor(char *buf, int max_len);

/* Returns product name from DMI */
int hw_dmi_product(char *buf, int max_len);

/* Detect if we're on a laptop (has battery / lid switch) */
int hw_is_laptop(void);

/* Detect if we're in a virtual machine */
int hw_is_vm(void);

/* Auto-configure system based on hardware detection */
void hw_auto_configure(void);

/* Returns human-readable laptop name */
const char *hw_laptop_name(void);

#endif
