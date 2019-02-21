#include <endian.h>
#include <stdlib.h>
#include <string.h>
#include "mtp1descriptors.h"

struct mtp1_descriptors_s* mtp1descriptors_ptr = NULL;
struct mtp1strings_s* mtp1strings_ptr = NULL;
struct usb_functionfs_descs_head_incompatible* mtp1descriptors_header_incompatible_ptr = NULL;

const struct mtp1_descriptors_s* mtp1descriptors()
{
    if (!mtp1descriptors_ptr) {
        const struct mtp1_descriptors_s mtp1descriptors = {
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
                 .bInterval = 16, // bInterval frames * 1 ms/frame
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
                 .bInterval = 5, // 2**(bInterval-1) franes * 0.125 ms/frame
              },
           },
        };
        mtp1descriptors_ptr = malloc(sizeof(struct mtp1_descriptors_s));
        memcpy(mtp1descriptors_ptr, &mtp1descriptors, sizeof(struct mtp1_descriptors_s));
    }
    return mtp1descriptors_ptr;
}

const struct mtp1strings_s* mtp1strings()
{
    if (!mtp1strings_ptr) {
        const struct mtp1strings_s mtp1strings = {
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
        mtp1strings_ptr = malloc(sizeof(struct mtp1strings_s));
        memcpy(mtp1strings_ptr, &mtp1strings, sizeof(struct mtp1strings_s));
    }
    return mtp1strings_ptr;
}

const struct usb_functionfs_descs_head_incompatible* mtp1descriptors_header_incompatible()
{
    if (!mtp1descriptors_header_incompatible_ptr) {
        const struct usb_functionfs_descs_head_incompatible mtp1descriptors_header_incompatible = {
          .magic = cpu_to_le32(FUNCTIONFS_DESCRIPTORS_MAGIC),
          .length = cpu_to_le32(sizeof(struct mtp1_descriptors_s_incompatible)),
          .fs_count = 4,
          .hs_count = 4,
          .ss_count = 0
        };
        mtp1descriptors_header_incompatible_ptr = malloc(sizeof(struct usb_functionfs_descs_head_incompatible));
        memcpy(mtp1descriptors_header_incompatible_ptr, &mtp1descriptors_header_incompatible, sizeof(struct usb_functionfs_descs_head_incompatible));
    }
    return mtp1descriptors_header_incompatible_ptr;
}
