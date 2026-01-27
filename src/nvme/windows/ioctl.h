// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Windows-specific NVMe implementation
 *
 * Copyright (c) 2026, James Huey <side1out@yahoo.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#ifndef _WIN_IOCTL_H
#define _WIN_IOCTL_H

#define _WIN32_WINNT 0x0A00          // Win10 APIs
#include <winapifamily.h>

#ifndef WINAPI_FAMILY
#define WINAPI_FAMILY WINAPI_FAMILY_DESKTOP_APP
#endif

#if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM)
#error "This code must be built for desktop or system partition"
#endif

#include "libnvmeapi.h"
#include "../ioctl.h"
#include "../types.h"

#define ENOTBLK 15

#include <winsock2.h>
#include <windows.h>
#include <winioctl.h>
//#include "ntddstor.h"
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

//#include "../tree.h"
//#include "../private.h"

/* ----- Linux-style ioctl encoding (ABI baked into lots of code) ----- */
#define _IOC_NRBITS     8
#define _IOC_TYPEBITS   8
#define _IOC_SIZEBITS   14
#define _IOC_DIRBITS    2

#define _IOC_NRSHIFT    0
#define _IOC_TYPESHIFT  (_IOC_NRSHIFT  + _IOC_NRBITS)
#define _IOC_SIZESHIFT  (_IOC_TYPESHIFT + _IOC_TYPEBITS)
#define _IOC_DIRSHIFT   (_IOC_SIZESHIFT + _IOC_SIZEBITS)

#define _IOC_NONE   0U
#define _IOC_WRITE  1U
#define _IOC_READ   2U

#define _IOC(dir, type, nr, size) \
    (((uint32_t)(dir)  << _IOC_DIRSHIFT)  | \
     ((uint32_t)(type) << _IOC_TYPESHIFT) | \
     ((uint32_t)(nr)   << _IOC_NRSHIFT)   | \
     ((uint32_t)(size) << _IOC_SIZESHIFT))

//#define _IO(type, nr)           _IOC(_IOC_NONE, (type), (nr), 0)
//#define _IOR(type, nr, T)       _IOC(_IOC_READ, (type), (nr), (uint32_t)sizeof(T))
//#define _IOW(type, nr, T)       _IOC(_IOC_WRITE, (type), (nr), (uint32_t)sizeof(T))
#define _IOWR(type, nr, T)      _IOC(_IOC_READ|_IOC_WRITE, (type), (nr), (uint32_t)sizeof(T))

/* Decode helpers */
#define _IOC_DIR(nr)   (((nr) >> _IOC_DIRSHIFT)  & ((1U<<_IOC_DIRBITS)-1))
#define _IOC_TYPE(nr)  (((nr) >> _IOC_TYPESHIFT) & ((1U<<_IOC_TYPEBITS)-1))
#define _IOC_NR(nr)    (((nr) >> _IOC_NRSHIFT)   & ((1U<<_IOC_NRBITS)-1))
#define _IOC_SIZE(nr)  (((nr) >> _IOC_SIZESHIFT) & ((1U<<_IOC_SIZEBITS)-1))

#define MAX_PHYSICAL_DRIVES 64

static int dumpmem(const char * label, unsigned char * buf,int len,int perline, FILE * out) {
    int i=0;
    int uperline=perline;
    if (uperline<1)
        uperline=32;    //default
    printf("\n%s:",label);
    if (out)
        fprintf(out,"\n%s:",label);
    for (i=0;i<len;i++) {
            if ((i%uperline)==0) {
                    printf("\n0x%08x: ",i);
                    if (out)
                        fprintf(out,"\n0x%08x: ",i);
            }
            printf("%02x ",buf[i]);
            if (out)
                fprintf(out,"%02x ",buf[i]);
    }
    printf("\n");
    if (out)
        fprintf(out,"\n");
    return 0;
}


static uint64_t round_up_4096(uint64_t x) {
    return (x + 4095) & ~((uint64_t)4095);
}


/* Format bytes as "  1.00  TB" / "240.06  GB" roughly like nvme-cli. */
static void format_bytes(double bytes, char *buf, size_t buflen)
{
    const double TB = 1e12;
    const double GB = 1e9;
    const double MB = 1e6;

    if (bytes >= TB) {
        snprintf(buf, buflen, "%6.2f  TB", bytes / TB);
    } else if (bytes >= GB) {
        snprintf(buf, buflen, "%6.2f  GB", bytes / GB);
    } else if (bytes >= MB) {
        snprintf(buf, buflen, "%6.2f  MB", bytes / MB);
    } else {
        snprintf(buf, buflen, "%6.0f  B ", bytes);
    }
}

/* nvme-cli prints "xxx  B +  y B" (logical + metadata). 
   Here we only know logical block size, so we show "+ 0 B". */
static void format_lba(uint32_t lba_size, uint32_t meta_size,
                       char *buf, size_t buflen)
{
    snprintf(buf, buflen, "%4u   B + %2u B", lba_size, meta_size);
}

/* Extract a string from STORAGE_DEVICE_DESCRIPTOR using an offset. */
static const char *get_desc_string(PSTORAGE_DEVICE_DESCRIPTOR desc, DWORD offset)
{
    if (offset == 0 || offset == (DWORD)-1)
        return "";
    return (const char *)((const uint8_t *)desc + offset);
}

/* ----------------- Core listing logic ----------------- */



void list_nvme_drives_win(void);

/* Map a POSIX fd to a Windows HANDLE. You decide how to create it. */
HANDLE fd_to_handle(int fd);

/* errno ↔ GetLastError helpers (minimal) */
int win_set_errno_from_last_error(void);

int nvme_scan_topology_win(void);//, void *f_args);

/* ioctl-compatible signature */
NVME_API int ioctl(int fd, unsigned long req, ...);
int win_ioctl(HANDLE fd, unsigned long ioctl_cmd, void  * cmd);

uint32_t nvme_get_nsid_win(HANDLE fd);
int nvme_submit_admin_passthru_win(HANDLE fd, struct nvme_passthru_cmd *cmd);

int nvme_submit_admin_passthru_win_identify(HANDLE fd, struct nvme_passthru_cmd *cmd);
int nvme_submit_admin_passthru_win_get_log(HANDLE fd, struct nvme_passthru_cmd *cmd);
int nvme_submit_admin_passthru_win_get_feature(HANDLE fd, struct nvme_passthru_cmd *cmd);
int nvme_submit_admin_passthru_win_set_feature(HANDLE fd, struct nvme_passthru_cmd *cmd);

void print_last_error(const char *context);
void print_win_error(const char *ctx);


int GetIdentifyDeviceOrAdapter(int value);
int GetNVMEIdentify(HANDLE fd, int cns, unsigned char* buffer, ULONG len);
int GetNVMEProperty(HANDLE fd, STORAGE_PROTOCOL_NVME_DATA_TYPE dt, uint32_t nsid, uint32_t value, uint32_t subvalue, uint32_t subvalue2, uint32_t subvalue3, uint32_t subvalue4, uint32_t subvalue5, ULONG len, unsigned char* buffer, int* retlen);
int SetNVMEProperty(HANDLE fd, STORAGE_PROTOCOL_NVME_DATA_TYPE dt, uint32_t nsid, uint32_t value, uint32_t subvalue, uint32_t subvalue2, uint32_t subvalue3, uint32_t subvalue4, uint32_t subvalue5, ULONG len, unsigned char* buffer, int* retlen, int v, int l);
int SetNVMEProperty2(HANDLE fd, STORAGE_PROTOCOL_NVME_DATA_TYPE dt, uint32_t nsid, uint32_t value, uint32_t subvalue, uint32_t subvalue2, uint32_t subvalue3, uint32_t subvalue4, uint32_t subvalue5, ULONG len, unsigned char* buffer, int* retlen, int v, int l);
int SetNvmeHostControlledThermal(HANDLE hDevice);

int NVMEPassthrough(HANDLE fd, unsigned char* buffer, int bufferLength, int datalen, uint32_t nsid, uint32_t opcode, uint32_t dw10, uint32_t dw11, uint32_t dw12, uint32_t dw13, uint32_t flags, int timeout_ms);

int FirmwareUpdate(HANDLE hDev, const struct nvme_passthru_cmd *cmd);
int FirmwareDownload(HANDLE fd, const struct nvme_passthru_cmd *cmd);
//int DeviceFirmwareUpgrade(HANDLE handle, const char * FileName);

#ifdef __cplusplus
}
#endif

#endif