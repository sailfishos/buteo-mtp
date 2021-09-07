#ifndef MTP1DESCRIPTORS_H
#define MTP1DESCRIPTORS_H

#include <linux/usb/functionfs.h>
#include "ptp.h"

#define MTP_STRING_DESCRIPTOR "MTP"
#define ENGLISH_US 0x0409

#define MTP_EP_PATH_CONTROL    "/dev/mtp/ep0"
#define MTP_EP_PATH_IN         "/dev/mtp/ep1"
#define MTP_EP_PATH_OUT        "/dev/mtp/ep2"
#define MTP_EP_PATH_INTERRUPT  "/dev/mtp/ep3"

/* Suppress warnings about deprecated kernel structures.
 *
 * This code needs to support devices with old kernels
 * that use structures/types that have been marked as
 * deprecated in later ones - which produces diagnostic
 * noise when building for devices with recent enough
 * kernels.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

struct mtp1_descs_s {
    struct usb_interface_descriptor intf;
    struct usb_endpoint_descriptor_no_audio mtp_ep_in;
    struct usb_endpoint_descriptor_no_audio mtp_ep_out;
    struct usb_endpoint_descriptor_no_audio mtp_ep_int;
} __attribute__((packed));

struct mtp1_descriptors_s {
    struct usb_functionfs_descs_head header;
    struct mtp1_descs_s fs_descs;
    struct mtp1_descs_s hs_descs;
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


struct usb_functionfs_descs_head_incompatible {
    __le32 magic;
    __le32 length;
    __le32 fs_count;
    __le32 hs_count;
    // The following field is added to the header in some
    // android kernels, which breaks compatibility.
    __le32 ss_count;
} __attribute__((packed));

struct mtp1_descriptors_s_incompatible {
    struct usb_functionfs_descs_head_incompatible header;
    struct mtp1_descs_s fs_descs;
    struct mtp1_descs_s hs_descs;
} __attribute__((packed));

extern const struct usb_functionfs_descs_head_incompatible mtp1descriptors_header_incompatible;

#pragma GCC diagnostic pop

#endif
