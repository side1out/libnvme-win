#ifndef _LINUX_TYPES_H
#define _LINUX_TYPES_H

#include "windows.h"
#include <stdint.h>
typedef uint8_t __u8;
typedef uint16_t __u16;
typedef uint32_t __u32;
typedef uint64_t __u64;

typedef uint8_t __le8;
typedef uint16_t __le16;
typedef uint32_t __le32;
typedef uint64_t __le64;

#if 0

//
// Command completion status
// The "Phase Tag" field and "Status Field" are separated in spec. We define them in the same data structure to ease the memory access from software.
//
typedef union {

    struct {
        USHORT  P           : 1;        // Phase Tag (P)

        USHORT  SC          : 8;        // Status Code (SC)
        USHORT  SCT         : 3;        // Status Code Type (SCT)
        USHORT  Reserved    : 2;
        USHORT  M           : 1;        // More (M)
        USHORT  DNR         : 1;        // Do Not Retry (DNR)
    } DUMMYSTRUCTNAME;

    USHORT AsUshort;

} NVME_COMMAND_STATUS, *PNVME_COMMAND_STATUS;


//
// Information of log: NVME_LOG_PAGE_ERROR_INFO. Size: 64 bytes
//
typedef struct {

    ULONGLONG           ErrorCount;
    USHORT              SQID;           // Submission Queue ID
    USHORT              CMDID;          // Command ID
    NVME_COMMAND_STATUS Status;         // Status Field: This field indicates the Status Field for the command  that completed.  The Status Field is located in bits 15:01, bit 00 corresponds to the Phase Tag posted for the command.

    struct {
        USHORT  Byte        : 8;        // Byte in command that contained the error.
        USHORT  Bit         : 3;        // Bit in command that contained the error.
        USHORT  Reserved    : 5;
    } ParameterErrorLocation;

    ULONGLONG           Lba;            // LBA: This field indicates the first LBA that experienced the error condition, if applicable.
    ULONG               NameSpace;      // Namespace: This field indicates the namespace that the error is associated with, if applicable.

    UCHAR               VendorInfoAvailable;    // Vendor Specific Information Available

    UCHAR               Reserved0[3];

    ULONGLONG           CommandSpecificInfo;    // This field contains command specific information. If used, the command definition specifies the information returned.

    UCHAR               Reserved1[24];

} NVME_ERROR_INFO_LOG, *PNVME_ERROR_INFO_LOG;

//
// NVMe command data structure
//
typedef struct {
    //
    // Common fields for all commands
    //
    NVME_COMMAND_DWORD0 CDW0;
    ULONG               NSID;
    ULONG               Reserved0[2];
    ULONGLONG           MPTR;
    ULONGLONG           PRP1;
    ULONGLONG           PRP2;

    //
    // Command independent fields from CDW10 to CDW15
    //
    union {

        //
        // General Command data fields
        //
        struct {
            ULONG   CDW10;
            ULONG   CDW11;
            ULONG   CDW12;
            ULONG   CDW13;
            ULONG   CDW14;
            ULONG   CDW15;
        } GENERAL;

        //
        // Admin Command: Identify
        //
        struct {
            NVME_CDW10_IDENTIFY CDW10;
            NVME_CDW11_IDENTIFY CDW11;
            ULONG   CDW12;
            ULONG   CDW13;
            ULONG   CDW14;
            ULONG   CDW15;
        } IDENTIFY;

        //
        // Admin Command: Abort
        //
        struct {
            NVME_CDW10_ABORT CDW10;
            ULONG   CDW11;
            ULONG   CDW12;
            ULONG   CDW13;
            ULONG   CDW14;
            ULONG   CDW15;
        } ABORT;

        //
        // Admin Command: Get/Set Features
        //
        struct {
            NVME_CDW10_GET_FEATURES CDW10;
            NVME_CDW11_FEATURES     CDW11;
            ULONG   CDW12;
            ULONG   CDW13;
            ULONG   CDW14;
            ULONG   CDW15;
        } GETFEATURES;

        struct {
            NVME_CDW10_SET_FEATURES CDW10;
            NVME_CDW11_FEATURES     CDW11;
            NVME_CDW12_FEATURES     CDW12;
            NVME_CDW13_FEATURES     CDW13;
            NVME_CDW14_FEATURES     CDW14;
            NVME_CDW15_FEATURES     CDW15;
        } SETFEATURES;

        //
        // Admin Command: Get Log Page
        //
        struct {
            union {
                NVME_CDW10_GET_LOG_PAGE     CDW10;
                NVME_CDW10_GET_LOG_PAGE_V13 CDW10_V13;
            };

            NVME_CDW11_GET_LOG_PAGE CDW11;
            NVME_CDW12_GET_LOG_PAGE CDW12;
            NVME_CDW13_GET_LOG_PAGE CDW13;
            NVME_CDW14_GET_LOG_PAGE CDW14;
            ULONG                   CDW15;
        } GETLOGPAGE;

        //
        // Admin Command: Create IO Completion Queue
        //
        struct {
            NVME_CDW10_CREATE_IO_QUEUE CDW10;
            NVME_CDW11_CREATE_IO_CQ    CDW11;
            ULONG   CDW12;
            ULONG   CDW13;
            ULONG   CDW14;
            ULONG   CDW15;
        } CREATEIOCQ;

        //
        // Admin Command: Create IO Submission Queue
        //
        struct {
            NVME_CDW10_CREATE_IO_QUEUE CDW10;
            NVME_CDW11_CREATE_IO_SQ    CDW11;
            ULONG   CDW12;
            ULONG   CDW13;
            ULONG   CDW14;
            ULONG   CDW15;
        } CREATEIOSQ;

        //
        // NVM Command: Dataset Management
        //
        struct {
            NVME_CDW10_DATASET_MANAGEMENT   CDW10;
            NVME_CDW11_DATASET_MANAGEMENT   CDW11;
            ULONG   CDW12;
            ULONG   CDW13;
            ULONG   CDW14;
            ULONG   CDW15;
        } DATASETMANAGEMENT;

        //
        // Admin Command: SECURITY SEND
        //
        struct {
            NVME_CDW10_SECURITY_SEND_RECEIVE    CDW10;
            NVME_CDW11_SECURITY_SEND            CDW11;
            ULONG   CDW12;
            ULONG   CDW13;
            ULONG   CDW14;
            ULONG   CDW15;
        } SECURITYSEND;

        //
        // Admin Command: SECURITY RECEIVE
        //
        struct {
            NVME_CDW10_SECURITY_SEND_RECEIVE    CDW10;
            NVME_CDW11_SECURITY_RECEIVE         CDW11;
            ULONG   CDW12;
            ULONG   CDW13;
            ULONG   CDW14;
            ULONG   CDW15;
        } SECURITYRECEIVE;

        //
        // Admin Command: FIRMWARE IMAGE DOWNLOAD
        //
        struct {
            NVME_CDW10_FIRMWARE_DOWNLOAD        CDW10;
            NVME_CDW11_FIRMWARE_DOWNLOAD        CDW11;
            ULONG   CDW12;
            ULONG   CDW13;
            ULONG   CDW14;
            ULONG   CDW15;
        } FIRMWAREDOWNLOAD;

        //
        // Admin Command: FIRMWARE ACTIVATE
        //
        struct {
            NVME_CDW10_FIRMWARE_ACTIVATE        CDW10;
            ULONG   CDW11;
            ULONG   CDW12;
            ULONG   CDW13;
            ULONG   CDW14;
            ULONG   CDW15;
        } FIRMWAREACTIVATE;

        //
        // Admin Command: FORMAT NVM
        //
        struct {
            NVME_CDW10_FORMAT_NVM               CDW10;
            ULONG   CDW11;
            ULONG   CDW12;
            ULONG   CDW13;
            ULONG   CDW14;
            ULONG   CDW15;
        } FORMATNVM;

        //
        // Admin Command: DIRECTIVE RECEIVE
        //
        struct {
            NVME_CDW10_DIRECTIVE_RECEIVE        CDW10;
            NVME_CDW11_DIRECTIVE_RECEIVE        CDW11;
            NVME_CDW12_DIRECTIVE_RECEIVE        CDW12;
            ULONG   CDW13;
            ULONG   CDW14;
            ULONG   CDW15;
        } DIRECTIVERECEIVE;

        //
        // Admin Command: DIRECTIVE SEND
        //
        struct {
            NVME_CDW10_DIRECTIVE_SEND           CDW10;
            NVME_CDW11_DIRECTIVE_SEND           CDW11;
            NVME_CDW12_DIRECTIVE_SEND           CDW12;
            ULONG   CDW13;
            ULONG   CDW14;
            ULONG   CDW15;
        } DIRECTIVESEND;

        //
        // Admin Command: SANITIZE
        //
        struct {
            NVME_CDW10_SANITIZE                 CDW10;
            NVME_CDW11_SANITIZE                 CDW11;
            ULONG   CDW12;
            ULONG   CDW13;
            ULONG   CDW14;
            ULONG   CDW15;
        } SANITIZE;
        
        //
        // NVM Command: Read/Write
        //
        struct {
            ULONG                   LBALOW;
            ULONG                   LBAHIGH;
            NVME_CDW12_READ_WRITE   CDW12;
            NVME_CDW13_READ_WRITE   CDW13;
            ULONG                   CDW14;
            NVME_CDW15_READ_WRITE   CDW15;
        } READWRITE;

        //
        // NVM Command: RESERVATION ACQUIRE
        //
        struct {
            NVME_CDW10_RESERVATION_ACQUIRE      CDW10;
            ULONG   CDW11;
            ULONG   CDW12;
            ULONG   CDW13;
            ULONG   CDW14;
            ULONG   CDW15;
        } RESERVATIONACQUIRE;

        //
        // NVM Command: RESERVATION REGISTER
        //
        struct {
            NVME_CDW10_RESERVATION_REGISTER     CDW10;
            ULONG   CDW11;
            ULONG   CDW12;
            ULONG   CDW13;
            ULONG   CDW14;
            ULONG   CDW15;
        } RESERVATIONREGISTER;

        //
        // NVM Command: RESERVATION RELEASE
        //
        struct {
            NVME_CDW10_RESERVATION_RELEASE      CDW10;
            ULONG   CDW11;
            ULONG   CDW12;
            ULONG   CDW13;
            ULONG   CDW14;
            ULONG   CDW15;
        } RESERVATIONRELEASE;

        //
        // NVM Command: RESERVATION REPORT
        //
        struct {
            NVME_CDW10_RESERVATION_REPORT       CDW10;
            NVME_CDW11_RESERVATION_REPORT       CDW11;
            ULONG   CDW12;
            ULONG   CDW13;
            ULONG   CDW14;
            ULONG   CDW15;
        } RESERVATIONREPORT;

        //
        // NVM Command: Zone Management Send
        //
        struct {
            NVME_CDW10_ZONE_MANAGEMENT_SEND CDW1011;
            ULONG                           CDW12;
            NVME_CDW13_ZONE_MANAGEMENT_SEND CDW13;
            ULONG                           CDW14;
            ULONG                           CDW15;
        } ZONEMANAGEMENTSEND;

        //
        // NVM Command: Zone Management Receive
        //
        struct {
            NVME_CDW10_ZONE_MANAGEMENT_RECEIVE  CDW1011;
            ULONG                               DWORDCOUNT;
            NVME_CDW13_ZONE_MANAGEMENT_RECEIVE  CDW13;
            ULONG                               CDW14;
            ULONG                               CDW15;
        } ZONEMANAGEMENTRECEIVE;

        //
        // NVM Command: Zone Append
        //
        struct {
            NVME_CDW10_ZONE_APPEND              CDW1011;
            NVME_CDW12_ZONE_APPEND              CDW12;
            ULONG                               CDW13;
            ULONG                               ILBRT;
            NVME_CDW15_ZONE_APPEND              CDW15;
        } ZONEAPPEND;

    } u;

} NVME_COMMAND, *PNVME_COMMAND;

C_ASSERT(sizeof(NVME_COMMAND) == 64); // NVMe commands are always 64 bytes
                                      // (defined by constant STORAGE_PROTOCOL_COMMAND_LENGTH_NVME)
#endif 

#endif // _LINUX_TYPES_H


