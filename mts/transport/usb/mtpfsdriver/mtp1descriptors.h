#ifndef MTP1DESCRIPTORS_H
#define MTP1DESCRIPTORS_H

#include <linux/usb/ptp.h>
#include <linux/usb/functionfs.h>

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

static const struct {
   struct usb_functionfs_descs_head header;
   struct {
      struct usb_interface_descriptor intf;
      struct usb_endpoint_descriptor_no_audio mtp_ep_in;
      struct usb_endpoint_descriptor_no_audio mtp_ep_out;
      struct usb_endpoint_descriptor_no_audio mtp_ep_int;
   } __attribute__((packed)) fs_descs, hs_descs;
} __attribute__((packed)) mtp1descriptors = {
   .header = {
      .magic = cpu_to_le32(FUNCTIONFS_DESCRIPTORS_MAGIC),
      .length = cpu_to_le32(sizeof mtp1descriptors),
      .fs_count = 4,
      .hs_count = 4,
   },
   .fs_descs = {
      .intf = {
         .bLength = sizeof mtp1descriptors.fs_descs.intf,
         .bDescriptorType = USB_DT_INTERFACE,
         .bAlternateSetting = 0,
         .bNumEndpoints = 3,
         .bInterfaceClass = USB_CLASS_STILL_IMAGE,
         .bInterfaceSubClass = USB_SUBCLASS_PTP,
         .bInterfaceProtocol = USB_PROTOCOL_PTP,
         .iInterface = 1,
      },
      .mtp_ep_in = {
         .bLength = sizeof mtp1descriptors.fs_descs.mtp_ep_in,
         .bDescriptorType = USB_DT_ENDPOINT,
         .bEndpointAddress = 1 | USB_DIR_IN,
         .bmAttributes = USB_ENDPOINT_XFER_BULK,
         .wMaxPacketSize = cpu_to_le16(PTP_FS_DATA_PKT_SIZE),
         .bInterval = 0,
      },
      .mtp_ep_out = {
         .bLength = sizeof mtp1descriptors.fs_descs.mtp_ep_out,
         .bDescriptorType = USB_DT_ENDPOINT,
         .bEndpointAddress = 2 | USB_DIR_OUT,
         .bmAttributes = USB_ENDPOINT_XFER_BULK,
         .wMaxPacketSize = cpu_to_le16(PTP_FS_DATA_PKT_SIZE),
         .bInterval = 0,
      },
      .mtp_ep_int = {
         .bLength = sizeof mtp1descriptors.fs_descs.mtp_ep_int,
         .bDescriptorType = USB_DT_ENDPOINT,
         .bEndpointAddress = 3 | USB_DIR_IN,
         .bmAttributes = USB_ENDPOINT_XFER_INT,
         .wMaxPacketSize = cpu_to_le16(PTP_FS_EVENT_PKT_SIZE),
         .bInterval = 255,
      },
   },
   .hs_descs = {
      .intf = {
         .bLength = sizeof mtp1descriptors.hs_descs.intf,
         .bDescriptorType = USB_DT_INTERFACE,
         .bAlternateSetting = 0,
         .bNumEndpoints = 3,
         .bInterfaceClass = USB_CLASS_STILL_IMAGE,
         .bInterfaceSubClass = USB_SUBCLASS_PTP,
         .bInterfaceProtocol = USB_PROTOCOL_PTP,
         .iInterface = 1,
      },
      .mtp_ep_in = {
         .bLength = sizeof mtp1descriptors.hs_descs.mtp_ep_in,
         .bDescriptorType = USB_DT_ENDPOINT,
         .bEndpointAddress = 1 | USB_DIR_IN,
         .bmAttributes = USB_ENDPOINT_XFER_BULK,
         .wMaxPacketSize = cpu_to_le16(PTP_HS_DATA_PKT_SIZE),
         .bInterval = 0,
      },
      .mtp_ep_out = {
         .bLength = sizeof mtp1descriptors.hs_descs.mtp_ep_out,
         .bDescriptorType = USB_DT_ENDPOINT,
         .bEndpointAddress = 2 | USB_DIR_OUT,
         .bmAttributes = USB_ENDPOINT_XFER_BULK,
         .wMaxPacketSize = cpu_to_le16(PTP_HS_DATA_PKT_SIZE),
         .bInterval = 0,
      },
      .mtp_ep_int = {
         .bLength = sizeof mtp1descriptors.hs_descs.mtp_ep_int,
         .bDescriptorType = USB_DT_ENDPOINT,
         .bEndpointAddress = 3 | USB_DIR_IN,
         .bmAttributes = USB_ENDPOINT_XFER_INT,
         .wMaxPacketSize = cpu_to_le16(PTP_HS_EVENT_PKT_SIZE),
         .bInterval = 12,
      },
   },
};

static const struct {
   struct usb_functionfs_strings_head header;
   struct {
      __le16 code;
      const char str1[sizeof MTP_STRING_DESCRIPTOR];
   } __attribute__((packed)) lang0;
} __attribute__((packed)) mtp1strings = {
   .header = {
      .magic = cpu_to_le32(FUNCTIONFS_STRINGS_MAGIC),
      .length = cpu_to_le32(sizeof mtp1strings),
      .str_count = cpu_to_le32(1),
      .lang_count = cpu_to_le32(1),
   },
   .lang0 = {
      cpu_to_le16(ENGLISH_US),
      MTP_STRING_DESCRIPTOR,
   },
};

#endif
