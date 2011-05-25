#ifndef MTP1DESCRIPTORS_H
#define MTP1DESCRIPTORS_H

#include <linux/usb/functionfs.h>
#include "ptp.h"

#define cpu_to_le16(x)  htole16(x)
#define cpu_to_le32(x)  htole32(x)
#define le32_to_cpu(x)  le32toh(x)
#define le16_to_cpu(x)  le16toh(x)

#define MTP_STRING_DESCRIPTOR "MTP"
#define ENGLISH_US 0x0409

static const char* control_file = "/dev/mtp/ep0";
static const char* in_file = "/dev/mtp/ep1";
static const char* out_file = "/dev/mtp/ep2";
static const char* interrupt_file = "/dev/mtp/ep3";

struct mtp1_descriptors_s {
   struct usb_functionfs_descs_head header;
   struct {
      struct usb_interface_descriptor intf;
      struct usb_endpoint_descriptor_no_audio mtp_ep_in;
      struct usb_endpoint_descriptor_no_audio mtp_ep_out;
      struct usb_endpoint_descriptor_no_audio mtp_ep_int;
   } __attribute__((packed)) fs_descs, hs_descs;
} __attribute__((packed));

extern const struct mtp1_descriptors_s mtp1descriptors;

struct mtp1strings_s {
   struct usb_functionfs_strings_head header;
   struct {
      __le16 code;
      const char str1[sizeof MTP_STRING_DESCRIPTOR];
   } __attribute__((packed)) lang0;
} __attribute__((packed));

extern const struct mtp1strings_s mtp1strings;

#endif
