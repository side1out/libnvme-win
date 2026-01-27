// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libnvme and retains its licenses.
 * Copyright (c) 2025.
 *
 * Author: James Huey <james_huey@Kingston.com>
 */
#include "ioctl.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <nvme.h>
#include <time.h>

#include <winsock2.h>
#include <windows.h>
#include <inttypes.h>
#include <winioctl.h>
//#include "ntddstor.h"
#include "ntddstor_shim.h"
//#include "../ioctl.h"



int nvme_scan_topology_win()//, void *f_args)
{
    // just printing...
    list_nvme_drives_win();
    return 0;
}

int ioctl(int fd, unsigned long req, ...) {
    va_list args;
    va_start(args, req); // 'count' is the last fixed argument

    unsigned long ioctl_cmd = req;
    void * cmd = va_arg(args, void *);
    va_end(args);

    printf("libnvme-win v1.9\n");
    int err = win_ioctl((HANDLE)fd, ioctl_cmd, cmd);
    if (err != 0) {
        printf("win_ioctl failed with error: %d\n", err);
        return -1;
    }
}

int win_ioctl(HANDLE fd, unsigned long ioctl_cmd, void * cmd)
{
    switch (ioctl_cmd) {
        case NVME_IOCTL_ADMIN_CMD:
            return nvme_submit_admin_passthru_win(fd, (struct nvme_passthru_cmd *)cmd);
        case NVME_IOCTL_ID:
            return nvme_get_nsid_win(fd);
        default:
            printf("Unsupported ioctl command: %lu\n", ioctl_cmd);
            return -1;
    }
}
uint32_t nvme_get_nsid_win(HANDLE fd)
{
    return 1; // fix later - prob identify namespace
}
int nvme_submit_admin_passthru_win(HANDLE fd, struct nvme_passthru_cmd *cmd)
{
    // Implement the Windows-specific logic for submitting an admin passthrough command
    // This is a placeholder implementation
    //printf("Submitting admin passthrough command on Windows cmdp = %llu cmd = %d, nsid = 0x%x\n", cmd, cmd->opcode, cmd->nsid);
    switch (cmd->opcode) {
        case NVME_ADMIN_COMMAND_GET_LOG_PAGE: // Example opcode for Get Log
            printf("Get Log command being submitted %08x\n", cmd->cdw10 & 0xff);
#if 0                
            printf("Get Log using passthrough\n");
            return NVMEPassthrough(fd, (unsigned char*)cmd->addr, cmd->data_len, cmd->data_len, cmd->nsid, cmd->opcode, cmd->cdw10, cmd->cdw11, cmd->cdw12, cmd->cdw13, cmd->flags, cmd->timeout_ms);
#else
            return nvme_submit_admin_passthru_win_get_log(fd, cmd);
#endif
            break;
        case NVME_ADMIN_COMMAND_IDENTIFY: // Example opcode for Identify
            printf("Identify command submitted\n");
            return nvme_submit_admin_passthru_win_identify(fd, cmd);
            break;
        case NVME_ADMIN_COMMAND_SET_FEATURES: // 0x09
            printf("Set features command submitted\n");
            return nvme_submit_admin_passthru_win_set_feature(fd, cmd);
        case NVME_ADMIN_COMMAND_GET_FEATURES: // 0x0A
            printf("Get features command submitted\n");
            return nvme_submit_admin_passthru_win_get_feature(fd, cmd);
        case NVME_ADMIN_COMMAND_FIRMWARE_COMMIT:        // 0x10 "Firmware Activate" command has been renamed to "Firmware Commit" command in spec v1.2
            return FirmwareUpdate(fd, cmd);
        case NVME_ADMIN_COMMAND_FIRMWARE_IMAGE_DOWNLOAD: // 0x11,
            printf("Firmware action submitted\n");
            return FirmwareDownload(fd, cmd);
        default:
            printf("Opcode not mapped to a windows API, attempting NVMe Passthrough: %u\n", cmd->opcode);
            return NVMEPassthrough(fd, (unsigned char*)cmd->addr, cmd->data_len, cmd->data_len, cmd->nsid, cmd->opcode, cmd->cdw10, cmd->cdw11, cmd->cdw12, cmd->cdw13, cmd->flags, cmd->timeout_ms);
    }
    return -1; 
}
int nvme_submit_admin_passthru_win_identify(HANDLE fd, struct nvme_passthru_cmd *cmd)
{
    uint32_t cns = cmd->cdw10;
    STORAGE_PROTOCOL_NVME_DATA_TYPE dt = NVMeDataTypeIdentify;
    ULONG len = cmd->data_len;
    unsigned char* buffer = (unsigned char*)cmd->addr;    
    int retlen = 0;
    int res = GetNVMEProperty(fd, dt, cmd->nsid, cns, cmd->cdw11, cmd->cdw12, cmd->cdw13, cmd->cdw14, cmd->cdw15, len, buffer, &retlen);
    if (res != 0) {
         printf("Failed to get NVMe Identify property, error: %d\n", res);
        return -1;
    }
    printf("NVMe Identify property retrieved successfully, length: %d\n", retlen);
    return 0;
}
int nvme_submit_admin_passthru_win_get_log(HANDLE fd, struct nvme_passthru_cmd *cmd)
{
    STORAGE_PROTOCOL_NVME_DATA_TYPE dt = NVMeDataTypeLogPage;
    ULONG len = cmd->data_len;
    unsigned char* buffer = (unsigned char*)cmd->addr;    
    int retlen = 0;
    int res = GetNVMEProperty(fd, dt, cmd->nsid, cmd->cdw10, cmd->cdw11, cmd->cdw12, cmd->cdw13, cmd->cdw14, cmd->cdw15, len, buffer, &retlen);
    if (res != 0) {
         printf("Failed to get NVMe Get Log, error: %d\n", res);
        return -1;
    }
    printf("NVMe Get Log retrieved successfully, length: %d\n", retlen);
    return 0;
}
int nvme_submit_admin_passthru_win_get_feature(HANDLE fd, struct nvme_passthru_cmd *cmd)
{
    STORAGE_PROTOCOL_NVME_DATA_TYPE dt = NVMeDataTypeFeature;
    ULONG len = cmd->data_len;
    unsigned char* buffer = (unsigned char*)cmd->addr;    
    int retlen = 0;
    int res = GetNVMEProperty(fd, dt, cmd->nsid, cmd->cdw10, cmd->cdw11, cmd->cdw12, cmd->cdw13, cmd->cdw14, cmd->cdw15, len, buffer, &retlen);
    if (res != 0) {
        printf("Failed to get NVMe Get Feature, error: %d\n", res);
        return -1;
    }
    printf("NVMe Get Feature retrieved successfully, length: %d\n", retlen);
    return 0;
}
int nvme_submit_admin_passthru_win_set_feature(HANDLE fd, struct nvme_passthru_cmd *cmd)
{
    STORAGE_PROTOCOL_NVME_DATA_TYPE dt = NVMeDataTypeFeature;
    ULONG len = cmd->data_len;
    unsigned char* buffer = (unsigned char*)cmd->addr;    
    int retlen = 0;

    int res = SetNVMEProperty(fd, dt, cmd->nsid, cmd->cdw10, cmd->cdw11, cmd->cdw12, cmd->cdw13, cmd->cdw14, cmd->cdw15, len, buffer, &retlen,0,0);
    if (res != 0) {
        printf("Failed to set NVMe Feature, error: %d\n", res);
        //return -1;
    }
    res = SetNVMEProperty(fd, dt, cmd->nsid, cmd->cdw10, cmd->cdw11, cmd->cdw12, cmd->cdw13, cmd->cdw14, cmd->cdw15, len, buffer, &retlen,0,1);
    if (res != 0) {
        printf("Failed to set NVMe Feature, error: %d\n", res);
        return res;
    }
    printf("NVMe Set Feature was successfully, length: %d\n", retlen);
    return 0;
}

void print_last_error(const char *context)
{
    DWORD errorMessageID = GetLastError();
    if (errorMessageID == 0)
        return;  // No error

    LPSTR messageBuffer = NULL;

    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorMessageID,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer,
        0,
        NULL
    );

    if (size && messageBuffer)
    {
        if (context)
            fprintf(stderr, "%s: (%lu) %s\n", context, errorMessageID, messageBuffer);
        else
            fprintf(stderr, "Windows Error (%lu): %s\n", errorMessageID, messageBuffer);
    }
    else
    {
        fprintf(stderr, "Windows Error (%lu)\n", errorMessageID);
    }
    LocalFree(messageBuffer);
}
void print_win_error(const char *ctx)
{
    DWORD e = GetLastError();
    LPSTR msg = NULL;

    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, e, 0, (LPSTR)&msg, 0, NULL);

    fprintf(stderr, "%s failed: (%lu) %s\n", ctx, e, msg ? msg : "");
    LocalFree(msg);
}
int GetIdentifyDeviceOrAdapter(int value)
{
    switch (value) {
        case NVME_IDENTIFY_CNS_CTRL:                    // 1
        case NVME_IDENTIFY_CNS_NVMSET_LIST:             // 4
        case NVME_IDENTIFY_CNS_CSI_CTRL:                // 6
        case NVME_IDENTIFY_CNS_NS_USER_DATA_FORMAT:     // 9
        case NVME_IDENTIFY_CNS_CSI_NS_USER_DATA_FORMAT: // 0x0A
        case NVME_IDENTIFY_CNS_CTRL_LIST:               // 0x13
        case NVME_IDENTIFY_CNS_PRIMARY_CTRL_CAP:		// 0x14
	    case NVME_IDENTIFY_CNS_SECONDARY_CTRL_LIST:		// 0x15
	    case NVME_IDENTIFY_CNS_NS_GRANULARITY:			// 0x16
	    case NVME_IDENTIFY_CNS_UUID_LIST:				// 0x17
	    case NVME_IDENTIFY_CNS_DOMAIN_LIST:				// 0x18
	    case NVME_IDENTIFY_CNS_ENDURANCE_GROUP_ID:      // 0x19
        case NVME_IDENTIFY_CNS_COMMAND_SET_STRUCTURE:   // 0x1C
        case NVME_IDENTIFY_CNS_PORTS_LIST:              // 0x1E
        case NVME_IDENTIFY_CNS_SUPPORTED_CTRL_STATE_FORMATS: // 0x20
            return (STORAGE_PROPERTY_ID)StorageAdapterProtocolSpecificProperty;
        default:
            return (STORAGE_PROPERTY_ID)StorageDeviceProtocolSpecificProperty;
    }
}
int GetNVMEIdentify(HANDLE fd, int cns, unsigned char* buffer, ULONG len)
{
    /*
    char path[64];
    snprintf(path, sizeof(path), "\\\\.\\PhysicalDrive%d", drive);

    // Open a handle to the NVMe device
    HANDLE fd = CreateFileA(
        path,
        GENERIC_READ,                         // read is enough for info
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (fd == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
            printf("%s not found!\n", path);
        }
        printf("%s open failed!\n", path);
        return -1;
    }
    */
    int retlen;

    int res = GetNVMEProperty(fd, NVMeDataTypeIdentify, 0xffffffff, cns, 0, 0, 0, 0, 0, len, buffer, &retlen);
    if (res != 0) {
         printf("Failed to get NVMe Identify property, error: %d\n", res);
        return -1;
    }
    //printf("NVMe Identify property retrieved successfully, length: %d\n", retlen);
    return 0;
}

int GetNVMEProperty(HANDLE fd, STORAGE_PROTOCOL_NVME_DATA_TYPE dt, uint32_t nsid, uint32_t value, uint32_t subvalue, uint32_t subvalue2, uint32_t subvalue3, uint32_t subvalue4, uint32_t subvalue5, ULONG len, unsigned char* buffer, int* retlen) {

    if (len > 4096)
        return 112;

    const DWORD IN_ARRAY_SIZE = sizeof(STORAGE_PROPERTY_QUERY) + sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA) + 4096;
    UCHAR* propQuery[IN_ARRAY_SIZE];// = (UCHAR *)malloc(IN_ARRAY_SIZE);
    memset(propQuery, 0xbb, IN_ARRAY_SIZE);
    memset(propQuery, 0, sizeof(STORAGE_PROPERTY_QUERY) + sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA));
    PSTORAGE_PROPERTY_QUERY pQueryDataDesc = (PSTORAGE_PROPERTY_QUERY)(propQuery);
    PSTORAGE_PROTOCOL_SPECIFIC_DATA pNvmeExtraData = (PSTORAGE_PROTOCOL_SPECIFIC_DATA)(pQueryDataDesc->AdditionalParameters);
    unsigned char* dbuf = (unsigned char*)propQuery;

    pNvmeExtraData->ProtocolType = ProtocolTypeNvme;
    pNvmeExtraData->DataType = dt;
    pNvmeExtraData->ProtocolDataRequestValue = value;
    pNvmeExtraData->ProtocolDataRequestSubValue = subvalue;
    pNvmeExtraData->ProtocolDataRequestSubValue2 = subvalue2;
    pNvmeExtraData->ProtocolDataRequestSubValue3 = subvalue3;
    pNvmeExtraData->ProtocolDataRequestSubValue4 = subvalue4;
    //pNvmeExtraData->ProtocolDataRequestSubValue5 = subvalue5;
    
    pNvmeExtraData->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);  // The offset of data buffer is from beginning of this data structure.
    pNvmeExtraData->ProtocolDataLength = len;
    if (dt == NVMeDataTypeLogPage) {
        pQueryDataDesc->PropertyId = (STORAGE_PROPERTY_ID)StorageDeviceProtocolSpecificProperty;
        pNvmeExtraData->ProtocolDataRequestSubValue3 = subvalue;   // dw11 - pretty silly but true
        pNvmeExtraData->ProtocolDataRequestSubValue = subvalue2;    // dw12 - offset low
        pNvmeExtraData->ProtocolDataRequestSubValue2 = subvalue3;   // dw13 - offset high
        pNvmeExtraData->ProtocolDataRequestSubValue4 = 0;           // This will map to STORAGE_PROTOCOL_DATA_SUBVALUE_GET_LOG_PAGE definition, then user can pass Retain Asynchronous Event, Log Specific Field.
        //pNvmeExtraData->ProtocolDataRequestSubValue54 = ?;        // Maybe this is dw14 (CSI) in newer sdk
    }
    else if (dt == NVMeDataTypeIdentify) {
            pQueryDataDesc->PropertyId = (STORAGE_PROPERTY_ID)GetIdentifyDeviceOrAdapter(value); 
    }
    else if (dt == NVMeDataTypeFeature) {
        pNvmeExtraData->ProtocolDataOffset = 0;
        pNvmeExtraData->ProtocolDataLength = len;
        if (len > 0)
            pNvmeExtraData->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA); 
        if ((nsid == 0xffffffff) || (nsid == 0))
            pQueryDataDesc->PropertyId = (STORAGE_PROPERTY_ID)StorageAdapterProtocolSpecificProperty; 
        else
            pQueryDataDesc->PropertyId = (STORAGE_PROPERTY_ID)StorageDeviceProtocolSpecificProperty; // msft example uses device
    }
    else {
        pQueryDataDesc->PropertyId = (STORAGE_PROPERTY_ID)StorageAdapterProtocolSpecificProperty;
    }
    pQueryDataDesc->QueryType = PropertyStandardQuery;
    DWORD bytesRet = 0;

    int doff = sizeof(STORAGE_PROPERTY_QUERY) + sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA) - sizeof(ULONG);

    int error = 1;
    memset(buffer, 0xdd, len);
    if (retlen)
        *retlen = -1;

    if (DeviceIoControl(fd,
        IOCTL_STORAGE_QUERY_PROPERTY,
        propQuery,
        sizeof(propQuery),
        propQuery,
        sizeof(propQuery),
        &bytesRet,
        NULL)) {
        memcpy(buffer, &dbuf[doff], len);
        error = 0;
        if (retlen)
            *retlen = pNvmeExtraData->ProtocolDataLength;
        //printf("NVMe property retrieved for %d:%u:%u\n", (int)dt, value, len);
    }
    else {
        printf("NVMe property failed for %d:%u:%u\n", (int)dt, value, len);
        error = GetLastError(); // the ioctl failed
        print_last_error("NVMEPassthrough DeviceIoControl failed");
    }
    return error;
}

int NVMEPassthrough(HANDLE fd, unsigned char* buffer, int bufferLength, int datalen, uint32_t nsid, uint32_t opcode, uint32_t dw10, uint32_t dw11, uint32_t dw12, uint32_t dw13, uint32_t flags, int timeout_ms)
{
    int cmdbuflen = FIELD_OFFSET(STORAGE_PROTOCOL_COMMAND, Command) + STORAGE_PROTOCOL_COMMAND_LENGTH_NVME + datalen + sizeof(NVME_ERROR_INFO_LOG);
    unsigned char* winbuffer = (unsigned char*)malloc(cmdbuflen);
    unsigned char* databuffer = &winbuffer[FIELD_OFFSET(STORAGE_PROTOCOL_COMMAND, Command) + STORAGE_PROTOCOL_COMMAND_LENGTH_NVME];
    ZeroMemory(winbuffer, cmdbuflen);
    PSTORAGE_PROTOCOL_COMMAND protocolCommand = (PSTORAGE_PROTOCOL_COMMAND)winbuffer;
    protocolCommand->Version = STORAGE_PROTOCOL_STRUCTURE_VERSION;
    protocolCommand->Length = sizeof(STORAGE_PROTOCOL_COMMAND);
    protocolCommand->ProtocolType = ProtocolTypeNvme;
    protocolCommand->Flags = flags;
    protocolCommand->CommandLength = STORAGE_PROTOCOL_COMMAND_LENGTH_NVME;
    protocolCommand->ErrorInfoLength = sizeof(NVME_ERROR_INFO_LOG);
    protocolCommand->TimeOutValue = 10;
    protocolCommand->ErrorInfoOffset = FIELD_OFFSET(STORAGE_PROTOCOL_COMMAND, Command) +
        STORAGE_PROTOCOL_COMMAND_LENGTH_NVME;
    if ((opcode & 3) == 1) {// write command
        protocolCommand->DataToDeviceTransferLength = datalen;
        protocolCommand->DataToDeviceBufferOffset = protocolCommand->ErrorInfoOffset + protocolCommand->ErrorInfoLength;
    }
    else if ((opcode & 3) == 2) {// read command
        protocolCommand->DataFromDeviceTransferLength = datalen;
        protocolCommand->DataFromDeviceBufferOffset = protocolCommand->ErrorInfoOffset + protocolCommand->ErrorInfoLength;
    }
    protocolCommand->CommandSpecific = STORAGE_PROTOCOL_SPECIFIC_NVME_ADMIN_COMMAND;
    PNVME_COMMAND command = (PNVME_COMMAND)protocolCommand->Command;

    command->CDW0.OPC = opcode & 0xff;
    command->u.GENERAL.CDW10 = dw10;
    command->u.GENERAL.CDW11 = dw11;
    command->u.GENERAL.CDW12 = dw12;
    command->u.GENERAL.CDW13 = dw13;
    command->NSID = nsid;

    if ((opcode & 3) == 1) // write command
        memcpy(&winbuffer[protocolCommand->DataToDeviceBufferOffset], buffer, datalen);

    //
    // Send request down.
    //
    DWORD returnedLength;
    BOOL result = DeviceIoControl(fd,
        IOCTL_STORAGE_PROTOCOL_COMMAND,
        winbuffer,
        cmdbuflen,
        winbuffer,
        cmdbuflen,
        &returnedLength,
        NULL
    );

    if (result) {
        if ((opcode & 3) == 2) // read command
            memcpy(buffer, &winbuffer[protocolCommand->DataFromDeviceBufferOffset], datalen);
        return 0;
    }
    else {
        int err = GetLastError();
        print_last_error("NVMEPassthrough DeviceIoControl failed");
        return err;
    }
}

int SetNvmeHostControlledThermal(HANDLE hDevice)
{
    BOOL  ok;
    DWORD returned = 0;

    BYTE* buffer = NULL;
    ULONG bufferLength = 0;

    PSTORAGE_PROPERTY_SET               setProp = NULL;
    PSTORAGE_PROTOCOL_SPECIFIC_DATA_EXT proto   = NULL;

    // 1) Allocate buffer: STORAGE_PROPERTY_SET + STORAGE_PROTOCOL_SPECIFIC_DATA_EXT (+ optional data)
    bufferLength  = FIELD_OFFSET(STORAGE_PROPERTY_SET, AdditionalParameters);
    bufferLength += sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA_EXT);
    bufferLength += NVME_MAX_LOG_SIZE;   // often 0 for this feature, but this matches MS pattern

    buffer = (BYTE*)calloc(1, bufferLength);
    if (!buffer) {
        fprintf(stderr, "alloc failed\n");
        return -1;
    }

    // 2) Fill STORAGE_PROPERTY_SET
    setProp = (PSTORAGE_PROPERTY_SET)buffer;
    setProp->PropertyId = StorageAdapterProtocolSpecificProperty; // or StorageDeviceProtocolSpecificProperty
    setProp->SetType    = PropertyStandardSet;

    // 3) Fill STORAGE_PROTOCOL_SPECIFIC_DATA_EXT located at AdditionalParameters
    proto = (PSTORAGE_PROTOCOL_SPECIFIC_DATA_EXT) setProp->AdditionalParameters;

    proto->ProtocolType = ProtocolTypeNvme;
    proto->DataType     = NVMeDataTypeFeature;  // “Set Features” for NVMe

    // Feature ID (FID) for Host Controlled Thermal Management
    proto->ProtocolDataValue   = NVME_FEATURE_HOST_CONTROLLED_THERMAL_MANAGEMENT;

    // These sub-values map to CDW11–CDW15 of the Set Features command.
    proto->ProtocolDataSubValue  = 0;  // CDW11
    proto->ProtocolDataSubValue2 = 0;  // CDW12
    proto->ProtocolDataSubValue3 = 0;  // CDW13
    proto->ProtocolDataSubValue4 = 0;  // CDW14
    proto->ProtocolDataSubValue5 = 0;  // CDW15

    // If you don’t have a data buffer, offset/length are zero.
    proto->ProtocolDataOffset  = 0;
    proto->ProtocolDataLength  = 0;

    // 4) Call DeviceIoControl
    ok = DeviceIoControl(
        hDevice,
        IOCTL_STORAGE_SET_PROPERTY,
        buffer,
        bufferLength,
        buffer,
        bufferLength,
        &returned,
        NULL
    );

    if (!ok) {
        DWORD err = GetLastError();
        fprintf(stderr, "IOCTL_STORAGE_SET_PROPERTY failed: %lu\n", err);
        free(buffer);
        print_last_error("NVMEPassthrough DeviceIoControl failed");
        return err;
    }

    free(buffer);
    return 0;
}
int SetNVMEProperty(HANDLE fd, STORAGE_PROTOCOL_NVME_DATA_TYPE dt, uint32_t nsid, uint32_t value, uint32_t subvalue, uint32_t subvalue2, uint32_t subvalue3, uint32_t subvalue4, uint32_t subvalue5, ULONG inlen, unsigned char* data_buffer, int* retlen, int v, int l) {

    BOOL  ok;
    DWORD returned = 0;

    int len = inlen;

    BYTE* buffer = NULL;
    ULONG bufferLength = 0;

    PSTORAGE_PROPERTY_SET setProp = NULL;
    PSTORAGE_PROTOCOL_SPECIFIC_DATA_EXT proto = NULL;

     // 1) Allocate buffer: STORAGE_PROPERTY_SET + STORAGE_PROTOCOL_SPECIFIC_DATA_EXT (+ optional data)
    bufferLength  = FIELD_OFFSET(STORAGE_PROPERTY_SET, AdditionalParameters);
    bufferLength += sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA_EXT);
    bufferLength += NVME_MAX_LOG_SIZE;   // often 0 for this feature, but this matches MS pattern

    buffer = (BYTE*)calloc(1, bufferLength);
    if (!buffer) {
        fprintf(stderr, "alloc failed\n");
        return -1;
    }

    // 2) Fill STORAGE_PROPERTY_SET
    setProp = (PSTORAGE_PROPERTY_SET)buffer;
    if (l == 0)
        setProp->PropertyId = StorageAdapterProtocolSpecificProperty;
    else 
        setProp->PropertyId = StorageDeviceProtocolSpecificProperty;

    setProp->SetType    = PropertyStandardSet;

   // 3) Fill STORAGE_PROTOCOL_SPECIFIC_DATA_EXT located at AdditionalParameters
    proto = (PSTORAGE_PROTOCOL_SPECIFIC_DATA_EXT) setProp->AdditionalParameters;

    proto->ProtocolType = ProtocolTypeNvme;
    proto->DataType     = NVMeDataTypeFeature;  // “Set Features” for NVMe

    proto->ProtocolDataValue = value;
    proto->ProtocolDataSubValue = subvalue;
    proto->ProtocolDataSubValue2 = subvalue2;
    proto->ProtocolDataSubValue3 = subvalue3;
    proto->ProtocolDataSubValue4 = subvalue4;
    proto->ProtocolDataSubValue5 = subvalue5;
    proto->ProtocolDataOffset = 0; //sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);  // The offset of data buffer is from beginning of this data structure.
    if (len > 0)
        proto->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA_EXT);
    proto->ProtocolDataLength = len;  

    DWORD bytesRet = 0;

    int error = 1;
    if (retlen)
        *retlen = -1;

    if (DeviceIoControl(fd,
        IOCTL_STORAGE_SET_PROPERTY,
        buffer,
        bufferLength,
        buffer,
        bufferLength,
        &returned,
        NULL)) {
        // good    
        error = 0;
        if (retlen)
            *retlen = proto->ProtocolDataLength;
        printf("NVMe property set for %d:%u:%u\n", (int)dt, value, len);
    }
    else {
        printf("NVMe property set failed for %d:%u:%u\n", (int)dt, value, len);
        error = GetLastError(); // the ioctl failed
        print_last_error("NVMEPassthrough DeviceIoControl failed");
    }
    return error;
}

int FirmwareUpdate(HANDLE fd, const struct nvme_passthru_cmd *cmd)
{
    if (!cmd) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    DWORD bytesReturned = 0;
    BOOL ok;
    /* Convert cdw11 (dword offset) to byte offset */

    switch (cmd->opcode) {

    case nvme_admin_fw_download: {
        /*
         * NVMe Firmware Image Download (opcode 0x11)
         *
         * Linux: cdw10 = NUMD (dwords - 1), cdw11 = offset in dwords
         *        addr/data_len = pointer/size of this chunk
         *
         * Windows: IOCTL_STORAGE_FIRMWARE_DOWNLOAD
         *          Input buffer = STORAGE_HW_FIRMWARE_DOWNLOAD + payload
         */

        const uint8_t *fw_data = (const uint8_t *)(uintptr_t)cmd->addr;
        uint64_t fw_len        = cmd->data_len;

        if (!fw_data || fw_len == 0) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return -1;
        }

        uint64_t img_offset_bytes = (uint64_t)cmd->cdw11 * 4u;

        // Saniity check

        if (img_offset_bytes == 0) {
            STORAGE_HW_FIRMWARE_INFO_QUERY q = {0};
            q.Version = sizeof(q);
            q.Size    = sizeof(q);

            BYTE buf[4096] = {0};
            DWORD ret = 0;

            BOOL ok = DeviceIoControl(
                fd,
                IOCTL_STORAGE_FIRMWARE_GET_INFO,
                &q,
                sizeof(q),
                buf,
                sizeof(buf),
                &ret,
                NULL
            );

            if (!ok) {
                printf("GET_INFO failed: %lu\n", GetLastError());
            } else {
                PSTORAGE_HW_FIRMWARE_INFO info = (PSTORAGE_HW_FIRMWARE_INFO)buf;
                printf("SlotCount=%u, SupportUpgrade=%u, ImagePayloadAlignment=%u, ImagePayloadMaxSize=%u\n",
                    info->SlotCount,
                    info->SupportUpgrade ? 1 : 0,
                    info->ImagePayloadAlignment,
                    info->ImagePayloadMaxSize);
            }
        }

        /* Optional sanity check: NUMD vs data_len */
        uint64_t numd_bytes = ((uint64_t)cmd->cdw10 + 1u) * 4u;
        if (numd_bytes != fw_len) {
            /* You can decide to treat this as error or just log a warning. */
            fprintf(stderr,
                    "Warning: cdw10(NUMD) bytes (%llu) != data_len (%llu); using data_len\n",
                    (unsigned long long)numd_bytes,
                    (unsigned long long)fw_len);
        }

        /* Build STORAGE_HW_FIRMWARE_DOWNLOAD + payload */
        size_t headerSize   = offsetof(STORAGE_HW_FIRMWARE_DOWNLOAD, ImageBuffer);
        size_t totalSize    = headerSize + (size_t)fw_len;

        STORAGE_HW_FIRMWARE_DOWNLOAD *dl =
            (STORAGE_HW_FIRMWARE_DOWNLOAD *)calloc(1, totalSize);
        if (!dl) {
            SetLastError(ERROR_OUTOFMEMORY);
            return -1;
        }

        dl->Version    = sizeof(STORAGE_HW_FIRMWARE_DOWNLOAD);
        dl->Size       = (DWORD)totalSize;

        /*
         * Flags:
         *   - you may want to set STORAGE_HW_FIRMWARE_REQUEST_FLAG_CONTROLLER
         *     if you are targeting the controller vs a specific LUN.
         *   - set LAST_SEGMENT if this is the final chunk.
         *
         * Here we assume a single-chunk update and mark it as LAST_SEGMENT.
         */
        LONG flags = STORAGE_HW_FIRMWARE_REQUEST_FLAG_CONTROLLER;

        /* Determine first/last segment */
        BOOL isFirst = (img_offset_bytes == 0);
        BOOL isLast  = (img_offset_bytes + fw_len == cmd->fw_len);
        if (isFirst)
            flags |= STORAGE_HW_FIRMWARE_REQUEST_FLAG_FIRST_SEGMENT;
        if (isLast)
            flags |= STORAGE_HW_FIRMWARE_REQUEST_FLAG_LAST_SEGMENT;

        printf("Firmware download: %llu bytes at offset %llu bytes, flags=0x%x\n", (unsigned long long)fw_len, (unsigned long long)img_offset_bytes, dl->Flags);

        dl->Slot       = 0;  /* 0 => “let controller choose slot” or use your own */
        /* Reserved[3] already zeroed by calloc */

        dl->Offset     = img_offset_bytes;
        dl->BufferSize = fw_len;

        memcpy(dl->ImageBuffer, fw_data, (size_t)fw_len);

        ok = DeviceIoControl(
            fd,
            IOCTL_STORAGE_FIRMWARE_DOWNLOAD,
            dl,
            (DWORD)totalSize,
            dl,
            (DWORD)totalSize,
            &bytesReturned,
            NULL
        );

        if (!ok) {
            print_last_error("IOCTL_STORAGE_FIRMWARE_DOWNLOAD");
            free(dl);
            return -1;
        }
        printf("firmware download good\n");
        free(dl);
        return 0;
    }

    case nvme_admin_fw_commit: {
        /*
         * NVMe Firmware Commit (opcode 0x10)
         *
         * Linux uses:
         *   cdw10 = (action << 3) | slot [+ optional bpid in high bits]
         *
         * NVMe spec:
         *   bits [2:0]  = FS (Firmware Slot)
         *   bits [5:3]  = CA / AA (Commit / Activate Action)
         *
         * Windows: IOCTL_STORAGE_FIRMWARE_ACTIVATE
         *          Input buffer = STORAGE_HW_FIRMWARE_ACTIVATE
         */

        uint32_t cdw10 = cmd->cdw10;

        uint8_t slot   = (uint8_t)( cdw10        & 0x7u );  /* bits 2:0 */
        uint8_t action = (uint8_t)((cdw10 >> 3) & 0x7u );   /* bits 5:3 */

        /* Build STORAGE_HW_FIRMWARE_ACTIVATE */
        STORAGE_HW_FIRMWARE_ACTIVATE act = {0};

        act.Version = sizeof(STORAGE_HW_FIRMWARE_ACTIVATE);
        act.Size    = sizeof(STORAGE_HW_FIRMWARE_ACTIVATE);

        /*
         * Map NVMe action -> STORAGE_HW_FIRMWARE_ACTIVATE Flags
         *
         *   NVMe CA  Meaning (simplified)
         *     0  Downloaded image replaces slot, not activated
         *     1  Downloaded image replaces slot, activate at next reset
         *     2  Activate existing image in slot at next reset
         *     3  Activate existing image in slot immediately
         *
         * Windows flags include:
         *   STORAGE_HW_FIRMWARE_REQUEST_FLAG_CONTROLLER
         *   STORAGE_HW_FIRMWARE_REQUEST_FLAG_SWITCH_TO_EXISTING_FIRMWARE
         *   STORAGE_HW_FIRMWARE_REQUEST_FLAG_REPLACE_EXISTING_IMAGE (newer)
         *
         * For a basic mapping we’ll:
         *   - always set CONTROLLER flag
         *   - set SWITCH_TO_EXISTING_FIRMWARE for “activate existing” modes
         *   You can refine this mapping based on how you actually use CA.
         */

        act.Flags = STORAGE_HW_FIRMWARE_REQUEST_FLAG_CONTROLLER;

        if (action == 2 || action == 3) {
            /* Activate an existing slot image */
            act.Flags |= STORAGE_HW_FIRMWARE_REQUEST_FLAG_SWITCH_TO_EXISTING_FIRMWARE;
        }

        act.Slot = slot;

        ok = DeviceIoControl(
            fd,
            IOCTL_STORAGE_FIRMWARE_ACTIVATE,
            &act,
            sizeof(act),
            &act,
            sizeof(act),
            &bytesReturned,
            NULL
        );

        if (!ok) {
            print_last_error("IOCTL_STORAGE_FIRMWARE_ACTIVATE");
            return -1;
        }

        return 0;
    }

    default:
        /* Not a firmware-related opcode; caller should handle via generic path */
        SetLastError(ERROR_NOT_SUPPORTED);
        return -1;
    }
}
int FirmwareDownload(HANDLE fd, const struct nvme_passthru_cmd *cmd)
{
    if (!cmd) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    if (cmd->opcode != nvme_admin_fw_download) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    const uint8_t *fw_data = (const uint8_t *)(uintptr_t)cmd->addr;
    uint64_t       fw_len  = cmd->data_len;

    if (!fw_data || fw_len == 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    
    bool pad = false;
    int fw_len_real = fw_len;
    if (fw_len % 4096 != 0) {
        fprintf(stderr, "Padding the last chunk of size %" PRIu64 " to 4K alignment as required by Windows\n",
                (uint64_t)fw_len);
                pad = true;
                fw_len_real = fw_len;
                fw_len = round_up_4096(fw_len);
    }
    
    /* Linux: cdw11 = offset in dwords; convert to bytes */
    uint64_t img_offset_bytes = (uint64_t)cmd->cdw11 * 4u;

    /* Total firmware image length (Linux side should set this); if zero, assume single-chunk */
    uint64_t fw_total_len = cmd->fw_len ? cmd->fw_len : (img_offset_bytes + fw_len);

    /* Query firmware info to get alignment/size & slot */
    STORAGE_HW_FIRMWARE_INFO_QUERY q = {0};
    q.Version = sizeof(q);
    q.Size    = sizeof(q);

    BYTE infoBuf[4096] = {0};
    DWORD ret = 0;

    BOOL ok = DeviceIoControl(
        fd,
        IOCTL_STORAGE_FIRMWARE_GET_INFO,
        &q,
        sizeof(q),
        infoBuf,
        sizeof(infoBuf),
        &ret,
        NULL
    );

    if (!ok) {
        print_last_error("IOCTL_STORAGE_FIRMWARE_GET_INFO");
        return -1;
    }

    PSTORAGE_HW_FIRMWARE_INFO info = (PSTORAGE_HW_FIRMWARE_INFO)infoBuf;

    if (!info->SupportUpgrade) {
        fprintf(stderr, "Device does not support firmware upgrade\n");
        SetLastError(ERROR_NOT_SUPPORTED);
        return -1;
    }

    ULONG align = info->ImagePayloadAlignment ? info->ImagePayloadAlignment : 1;
    ULONG maxSz = info->ImagePayloadMaxSize   ? info->ImagePayloadMaxSize   : (ULONG)fw_len;

    /* Alignment checks: offset and chunk length must obey device constraints */
    if ((img_offset_bytes % align) != 0) {
        fprintf(stderr, "Firmware chunk offset %" PRIu64 " not aligned to %lu\n",
                (uint64_t)img_offset_bytes, (unsigned long)align);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    if ((fw_len % align) != 0 && (img_offset_bytes + fw_len) < fw_total_len) {
        /* Allow misalignment only on the final partial chunk */
        fprintf(stderr, "Firmware chunk size %" PRIu64 " not aligned to %lu (and not final)\n",
                (uint64_t)fw_len, (unsigned long)align);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    if (fw_len == 0 || fw_len > maxSz) {
        fprintf(stderr, "Firmware chunk size %" PRIu64 " exceeds max payload %lu\n",
                (uint64_t)fw_len, (unsigned long)maxSz);
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    /* Choose a slot: for now, use the first valid slot */
    UCHAR slot = 0;
    if (info->SlotCount > 0) {
        slot = info->Slot[0].SlotNumber;
        if (slot == STORAGE_HW_FIRMWARE_INVALID_SLOT)
            slot = 0;
    }

    /* Determine first/last segment */
    BOOL isFirst = (img_offset_bytes == 0);
    BOOL isLast  = (img_offset_bytes + fw_len == fw_total_len);

    /* Build STORAGE_HW_FIRMWARE_DOWNLOAD + payload */
    size_t headerSize = offsetof(STORAGE_HW_FIRMWARE_DOWNLOAD, ImageBuffer);
    size_t totalSize  = headerSize + (size_t)fw_len;

    if (pad) {
        printf(" adding firmware chunk from %" PRIu64 " to next 4096 bytes\n", (uint64_t)fw_len);
    }


    STORAGE_HW_FIRMWARE_DOWNLOAD *dl =
        (STORAGE_HW_FIRMWARE_DOWNLOAD *)calloc(1, totalSize);
    if (!dl) {
        SetLastError(ERROR_OUTOFMEMORY);
        return -1;
    }
    if (pad) {
        ZeroMemory(dl, totalSize); // be sure to zero padding
    }


    dl->Version    = sizeof(STORAGE_HW_FIRMWARE_DOWNLOAD);
    dl->Size       = (DWORD)totalSize;
    dl->Slot       = slot;
    dl->Offset     = img_offset_bytes;
    dl->BufferSize = fw_len;

    ULONG flags = STORAGE_HW_FIRMWARE_REQUEST_FLAG_CONTROLLER;

    if (isFirst)
        flags |= STORAGE_HW_FIRMWARE_REQUEST_FLAG_FIRST_SEGMENT;
    if (isLast)
        flags |= STORAGE_HW_FIRMWARE_REQUEST_FLAG_LAST_SEGMENT;

    dl->Flags = flags;

    memcpy(dl->ImageBuffer, fw_data, (size_t)fw_len_real);

    //printf("Firmware download: %" PRIu64 " bytes at offset %" PRIu64
    //       " of %" PRIu64 " bytes, flags=0x%08lx, slot=%u\n",
    //      fw_len, img_offset_bytes, cmd->fw_len, (unsigned long)dl->Flags, dl->Slot);

    DWORD bytesReturned = 0;
    ok = DeviceIoControl(
        fd,
        IOCTL_STORAGE_FIRMWARE_DOWNLOAD,
        dl,
        (DWORD)totalSize,
        dl,
        (DWORD)totalSize,
        &bytesReturned,
        NULL
    );

    if (!ok) {
        print_last_error("IOCTL_STORAGE_FIRMWARE_DOWNLOAD");
        free(dl);
        return -1;
    }

    printf("Firmware download chunk OK\n");
    free(dl);
    return 0;
}


void list_nvme_drives_win(void)
{
    printf("Node                  Mangled SN                               SN                   Model                                    Namespace  Usage                      Format           FW Rev  \n");
    printf("--------------------- ---------------------------------------- -------------------- ---------------------------------------- ---------- -------------------------- ---------------- --------\n");

    int found_any = 0;

    for (int drive = 0; drive < MAX_PHYSICAL_DRIVES; ++drive) {
        char path[64];
        snprintf(path, sizeof(path), "\\\\.\\PhysicalDrive%d", drive);

        HANDLE h = CreateFileA(
            path,
            GENERIC_READ,                         // read is enough for info
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );

        if (h == INVALID_HANDLE_VALUE) {
            DWORD err = GetLastError();
            if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
                // Once we hit a gap, keep going; Windows numbering can be sparse.
                continue;
            }
            // For other errors, just skip this drive.
            continue;
        }

        /* --- Query STORAGE_DEVICE_DESCRIPTOR to see if it's NVMe --- */

        STORAGE_PROPERTY_QUERY query;
        ZeroMemory(&query, sizeof(query));
        query.PropertyId = StorageDeviceProperty;
        query.QueryType  = PropertyStandardQuery;

        uint8_t outBuf[4096];
        ZeroMemory(outBuf, sizeof(outBuf));

        DWORD bytes = 0;
        BOOL ok = DeviceIoControl(
            h,
            IOCTL_STORAGE_QUERY_PROPERTY,
            &query,
            sizeof(query),
            outBuf,
            sizeof(outBuf),
            &bytes,
            NULL
        );

        if (!ok || bytes < sizeof(STORAGE_DEVICE_DESCRIPTOR)) {
            CloseHandle(h);
            continue;
        }

        PSTORAGE_DEVICE_DESCRIPTOR desc = (PSTORAGE_DEVICE_DESCRIPTOR)outBuf;

        if (desc->BusType != BusTypeNvme) {
            // Not an NVMe device, skip.
            CloseHandle(h);
            continue;
        }

        found_any = 1;

        unsigned char icbuffer[4096];
        ZeroMemory(icbuffer, sizeof(icbuffer));
        int ret = GetNVMEIdentify(h, NVME_IDENTIFY_CNS_CTRL, icbuffer, sizeof(icbuffer));
        if (ret != 0) {
            CloseHandle(h);
            continue;
        }
        else {
            // Successfully got Identify Controller data
            // You can parse icbuffer as needed here
            //printf("Successfully retrieved NVMe Identify Controller data for %s\n", path);
        }

        char serial[21] = {0};
        char mn[41] = {0};
        char fr[9]  = {0};

        memcpy(serial, &icbuffer[4], 20);
        memcpy(mn, &icbuffer[24], 40);
        memcpy(fr, &icbuffer[64], 8);

        //trim_ascii(sn);
        //trim_ascii(mn);
        //trim_ascii(fr);

        //printf("Serial      : %s\n", sn);
        //printf("Model       : %s\n", mn);
        //printf("FW Revision : %s\n", fr);

        const char *vendor = get_desc_string(desc, desc->VendorIdOffset);
        const char *product = get_desc_string(desc, desc->ProductIdOffset);
        const char *serialm = get_desc_string(desc, desc->SerialNumberOffset); // mangled windows serial number
        const char *fw_rev = get_desc_string(desc, desc->ProductRevisionOffset);

        const char *serial2 = "";

        /* Build a "model" string similar to Linux nvme-cli (typically ProductId). */
        char model[64];
        snprintf(model, sizeof(model), "%s", product && product[0] ? product : "NVMe Device");

        /* --- Get capacity + logical block size --- */

        DISK_GEOMETRY_EX geomEx;
        ZeroMemory(&geomEx, sizeof(geomEx));
        bytes = 0;

        double capacity_bytes = 0.0;
        uint32_t logical_block_size = 512;

        ok = DeviceIoControl(
            h,
            IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
            NULL,
            0,
            &geomEx,
            sizeof(geomEx),
            &bytes,
            NULL
        );

        if (ok) {
            capacity_bytes = (double)geomEx.DiskSize.QuadPart;
            logical_block_size = geomEx.Geometry.BytesPerSector;
        }

        CloseHandle(h);

        char usage_total[32];
        char usage_used[32];
        char format_str[32];

        format_bytes(capacity_bytes, usage_total, sizeof(usage_total));
        // We don't know "used" vs "total" for a raw physical drive, so print total / total.
        snprintf(usage_used, sizeof(usage_used), "%s", usage_total);

        format_lba(logical_block_size, 0, format_str, sizeof(format_str));

        /* For now, assume single namespace 0x1 like in your sample. */
        unsigned int nsid = 0x1;

        /*
         * Print row.  Columns:
         * - Node:    \\.\PhysicalDriveN
         * - Generic: (blank or same as Node – here just blank placeholder)
         * - SN:      serial
         * - Model:   model
         * - Namespace: nsid (hex)
         * - Usage:   <used> / <total> (here total/total)
         * - Format:  "<lba size> B + 0 B"
         * - FW Rev:  fw_rev
         */
        printf("%-21s %-21s %-20s %-40s 0x%-8X %10s / %10s  %-16s %-8s\n",
               path,                   // Node
               serialm,                     // Generic (no direct Windows analog)
               serial && serial[0] ? serial : "",
               model,
               nsid,
               usage_used,
               usage_total,
               format_str,
               fw_rev && fw_rev[0] ? fw_rev : "");
    }

    if (!found_any) {
        fprintf(stderr, "No NVMe devices found (BusTypeNvme) on this system.\n");
    }
}

static int GetDiskPerfSnapshot(const char *devicePath, DISK_PERFORMANCE *outPerf)
{
    HANDLE h = CreateFileA(
        devicePath,
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (h == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Failed to open %s (err=%lu)\n", devicePath, GetLastError());
        return -1;
    }

    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(
        h,
        IOCTL_DISK_PERFORMANCE,
        NULL,
        0,
        outPerf,
        sizeof(*outPerf),
        &bytesReturned,
        NULL
    );

    CloseHandle(h);

    if (!ok) {
        fprintf(stderr,
            "IOCTL_DISK_PERFORMANCE failed on %s (err=%lu)\n",
            devicePath, GetLastError());
        return -1;
    }

    return 0;
}

static void print_usage(void)
{
    printf("Usage:\n");
    printf("  perfmon <driveNumber> --perf-mon sample-ms=<N>\n");
    printf("\nExample:\n");
    printf("  perfmon 0 --perf-mon sample-ms=1000\n");
}

int main(int argc, char **argv)
{
    if (argc < 4) {
        print_usage();
        return 1;
    }

    int driveNum = atoi(argv[1]);

    if (strcmp(argv[2], "--perf-mon") != 0) {
        print_usage();
        return 1;
    }

    // Parse sample-ms=N
    int sampleMs = 1000;
    if (strncmp(argv[3], "sample-ms=", 10) == 0) {
        sampleMs = atoi(argv[3] + 10);
        if (sampleMs < 10) sampleMs = 10;
    } else {
        print_usage();
        return 1;
    }

    // Build device path
    char device[64];
    sprintf(device, "\\\\.\\PhysicalDrive%d", driveNum);

    printf("Monitoring %s every %d ms...\n", device, sampleMs);

    // CSV file name
    char csvFile[128];
    sprintf(csvFile, "diskperf_PhysicalDrive%d.csv", driveNum);

    FILE *fp = fopen(csvFile, "w");
    if (!fp) {
        fprintf(stderr, "Failed to open output CSV: %s\n", csvFile);
        return 1;
    }

    fprintf(fp,
        "timestamp,"
        "ReadBytes,WriteBytes,"
        "ReadCount,WriteCount,"
        "DeltaReadBytes,DeltaWriteBytes,"
        "DeltaReadCount,DeltaWriteCount,"
        "Read_MBps,Write_MBps,Read_IOPS,Write_IOPS,Total_IOPS\n");
    fflush(fp);

    DISK_PERFORMANCE prev = {0};
    DISK_PERFORMANCE cur  = {0};

    // Get initial snapshot
    if (GetDiskPerfSnapshot(device, &prev) != 0) {
        fclose(fp);
        return 1;
    }

    printf("Logging to %s ... Press Ctrl+C to stop.\n", csvFile);

    for (;;) {
        Sleep(sampleMs);

        if (GetDiskPerfSnapshot(device, &cur) != 0) {
            fprintf(stderr, "Error retrieving stats.\n");
            continue;
        }

        // Compute elapsed time in seconds
        LONGLONG dt100 = cur.QueryTime.QuadPart - prev.QueryTime.QuadPart;
        double secs = dt100 / 10000000.0;
        if (secs <= 0.0) secs = sampleMs / 1000.0;

        // Compute deltas
        ULONGLONG dReadBytes  = cur.BytesRead.QuadPart     - prev.BytesRead.QuadPart;
        ULONGLONG dWriteBytes = cur.BytesWritten.QuadPart  - prev.BytesWritten.QuadPart;
        ULONGLONG dReadOps    = cur.ReadCount              - prev.ReadCount;
        ULONGLONG dWriteOps   = cur.WriteCount             - prev.WriteCount;

        // Compute rates
        double readMBps  = (double)dReadBytes  / (1024.0 * 1024.0) / secs;
        double writeMBps = (double)dWriteBytes / (1024.0 * 1024.0) / secs;
        double readIOPS  = (double)dReadOps    / secs;
        double writeIOPS = (double)dWriteOps   / secs;

        // Timestamp
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char ts[64];
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", t);

        // Write CSV line
        fprintf(fp,
            "%s,"
            "%llu,%llu,"
            "%lu,%lu,"
            "%llu,%llu,"
            "%lu,%lu,"
            "%.3f,%.3f,%.3f,%.3f,%.3f\n",
            ts,
            (unsigned long long)cur.BytesRead.QuadPart,
            (unsigned long long)cur.BytesWritten.QuadPart,
            (unsigned long)cur.ReadCount,
            (unsigned long)cur.WriteCount,
            (unsigned long long)dReadBytes,
            (unsigned long long)dWriteBytes,
            (unsigned long)dReadOps,
            (unsigned long)dWriteOps,
            readMBps, writeMBps, readIOPS, writeIOPS, (readIOPS + writeIOPS));

        fflush(fp);

        // Move current → previous
        prev = cur;
    }

    fclose(fp);
    return 0;
}



#if 0
int DeviceFirmwareUpgrade(HANDLE handle, const char * FileName)
/*++

Routine Description:

    Performs a firmware upgrade to the NVMe controller. The an available firmware
    slot is selected, the firmware is downloaded to the controller from an image
    file, and the new firmware is activated.


Arguments:

    DeviceList    – a pointer to device array that contains disks information.
    Index         – the index of NVMe device in DeviceList array.
    FileName      – the name of the firmware upgrade image file.

Return Value:

    None

--*/
{
    int ret=1;

    BOOL                    result;
    PUCHAR                  buffer = NULL;
    ULONG                   bufferSize;
    ULONG                   firmwareStructureOffset;
    ULONG                   imageBufferLength;

    PSRB_IO_CONTROL         srbControl;
    PFIRMWARE_REQUEST_BLOCK firmwareRequest;

    PSTORAGE_FIRMWARE_INFO      firmwareInfo;
    PSTORAGE_FIRMWARE_DOWNLOAD  firmwareDownload;
    PSTORAGE_FIRMWARE_ACTIVATE  firmwareActivate;

    ULONG                   slotNumber;
    ULONG                   returnedLength;
    ULONG                   i;

    HANDLE                  fileHandle = NULL;
    ULONG                   imageOffset;
    ULONG                   readLength;
    BOOLEAN                 moreToDownload;

    //
    // The STORAGE_FIRMWARE_INFO is located after SRB_IO_CONTROL and FIRMWARE_RESQUEST_BLOCK
    //
    firmwareStructureOffset = ((sizeof(SRB_IO_CONTROL) + sizeof(FIRMWARE_REQUEST_BLOCK) - 1) / sizeof(PVOID) + 1) * sizeof(PVOID);

    //
    // The Max Transfer Length limits the part of buffer that may need to transfer to controller, not the whole buffer.
    //
    bufferSize = 4*1024;
    //if (DeviceList[Index].AdapterDescriptor.MaximumTransferLength < (2 * 1024 * 1024))
    //    bufferSize = DeviceList[Index].AdapterDescriptor.MaximumTransferLength;

    bufferSize += firmwareStructureOffset;
    bufferSize += FIELD_OFFSET(STORAGE_FIRMWARE_DOWNLOAD, ImageBuffer);

    buffer = (PUCHAR)malloc(bufferSize);
    if (buffer == NULL) {
        RecLine("FirmwareUpgrade - Allocate buffer failed: 0x%X\n", GetLastError());
        return 90002;
    }

    //
    // calculate the space available for the firmware image portion of the buffer allocation
    //
    imageBufferLength = bufferSize - firmwareStructureOffset - sizeof(STORAGE_FIRMWARE_DOWNLOAD);

    //
    // Set the request structure pointers
                //
    srbControl = (PSRB_IO_CONTROL)buffer;
    firmwareRequest = (PFIRMWARE_REQUEST_BLOCK)(srbControl + 1);
    firmwareInfo = (PSTORAGE_FIRMWARE_INFO)((PUCHAR)srbControl + firmwareRequest->DataBufferOffset);

#if 0
    RtlZeroMemory(buffer, bufferSize);

    // ---------------------------------------------------------------------------
    // ( 1 ) SELECT A SUITABLE FIRMWARE SLOT
    // ---------------------------------------------------------------------------

    //
    // Get firmware slot information data.
    //
    result = DeviceGetFirmwareInfo(handle, buffer, bufferSize, FALSE);

    if (result == FALSE) {
        RecLine("FirmwareUpgrade: Get Firmware Information Failed: 0x%X\n", GetLastError());
        goto Exit;
    }


    if (srbControl->ReturnCode != FIRMWARE_STATUS_SUCCESS) {
        RecLine("FirmwareUpgrade - get firmware info failed. srbControl->ReturnCode %d.\n", srbControl->ReturnCode);
        goto Exit;
    }

//  this hangs on win server 2012
    //
    // SelectFind the first writable slot.
    //
    slotNumber = (ULONG)-1;

    if (firmwareInfo->UpgradeSupport) {
        for (i = 0; i < firmwareInfo->SlotCount; i++) {
            if (firmwareInfo->Slot[i].ReadOnly == FALSE) {
                slotNumber = firmwareInfo->Slot[i].SlotNumber;
                break;
            }
        }
    }

    //
    // If no writable slot is found, bypass downloading and activation
    //
    if (slotNumber == (ULONG)-1) {
        RecLine("FirmwareUpgrade - No writable Firmware slot.\n");
        goto Exit;
    }

#else
    slotNumber = 1; //win 8 2012 workaround

#endif
    // ---------------------------------------------------------------------------
    // ( 2 ) DOWNLOAD THE FIRMWARE IMAGE TO THE CONTROLLER
    // ---------------------------------------------------------------------------

    //
    // initialize image length and offset
    //
    imageBufferLength = (imageBufferLength / sizeof(PVOID)) * sizeof(PVOID);
    imageOffset = 0;
    readLength = 0;
    moreToDownload = TRUE;

    //
    // Open image file and download it to controller.
    //
    if (FileName == NULL) {
        RecLine("FirmwareUpgrade - No firmware file specified.\n");
        goto Exit;
    }

    fileHandle = CreateFile(FileName,              // file to open
                            GENERIC_READ,          // open for reading
                            FILE_SHARE_READ,       // share for reading
                            NULL,                  // default security
                            OPEN_EXISTING,         // existing file only
                            FILE_ATTRIBUTE_NORMAL, // normal file
                            NULL);                 // no attr. template

    if (fileHandle == INVALID_HANDLE_VALUE) {
        RecLine("\t FirmwareUpgrade - unable to open file \"%s\" for read.", FileName);
        goto Exit;
    }

    //
    // Read and download the firmware from the image file into image buffer length portions. Send the
    // image portion to the controller.
    //
    while (moreToDownload) {

        RtlZeroMemory(buffer, bufferSize);

        //
        // Setup the SRB control with the firmware ioctl control info
        //
        srbControl->HeaderLength = sizeof(SRB_IO_CONTROL);
        srbControl->ControlCode = IOCTL_SCSI_MINIPORT_FIRMWARE;
        RtlMoveMemory(srbControl->Signature, IOCTL_MINIPORT_SIGNATURE_FIRMWARE, 8);
        srbControl->Timeout = 3;
        srbControl->Length = bufferSize - sizeof(SRB_IO_CONTROL);

        //
        // Set firmware request fields for FIRMWARE_FUNCTION_DOWNLOAD. This request is to the controller so
        // FIRMWARE_REQUEST_FLAG_CONTROLLER is set in the flags
        //
        firmwareRequest->Version = FIRMWARE_REQUEST_BLOCK_STRUCTURE_VERSION;
        firmwareRequest->Size = sizeof(FIRMWARE_REQUEST_BLOCK);
        firmwareRequest->Function = FIRMWARE_FUNCTION_DOWNLOAD;
        firmwareRequest->Flags = FIRMWARE_REQUEST_FLAG_CONTROLLER;
        firmwareRequest->DataBufferOffset = firmwareStructureOffset;
        firmwareRequest->DataBufferLength = bufferSize - firmwareStructureOffset;

        //
        // Initialize the firmware data buffer pointer to the proper position after the request structure
        //
        firmwareDownload = (PSTORAGE_FIRMWARE_DOWNLOAD)((PUCHAR)srbControl + firmwareRequest->DataBufferOffset);

        if (ReadFile(fileHandle, firmwareDownload->ImageBuffer, imageBufferLength, &readLength, NULL) == FALSE) {
            RecLine("\t FirmwareUpgrade - Read firmware file failed.\n");
            goto Exit;
        }

        if (readLength == 0) {
            moreToDownload = FALSE;
            break;
        }

        if ((readLength % sizeof(ULONG)) != 0) {
            RecLine("FirmwareUpgrade - Read firmware file failed.\n");
        }

        //
        // Set the download parameters and adjust the offset for this portion of the firmware image
        //
        firmwareDownload->Version = 1;
        firmwareDownload->Size = sizeof(STORAGE_FIRMWARE_DOWNLOAD);
        firmwareDownload->Offset = imageOffset;
        firmwareDownload->BufferSize = readLength;

        //
        // download this portion of firmware to the device
        //
        result = DeviceIoControl(handle,
                                 IOCTL_SCSI_MINIPORT,
                                 buffer,
                                 bufferSize,
                                 buffer,
                                 bufferSize,
                                 &returnedLength,
                                 NULL
                                 );

        if (result == FALSE) {
            RecLine("FirmwareUpgrade - IOCTL - firmware download failed. 0x%X.", GetLastError());
            goto Exit;
        }

        if (srbControl->ReturnCode != FIRMWARE_STATUS_SUCCESS) {
            RecLine("FirmwareUpgrade - firmware download failed. srbControl->ReturnCode %d.", srbControl->ReturnCode);
            goto Exit;
        }

        //
        // Update Image Offset for next iteration.
        //
        imageOffset += readLength;
    }

    // ---------------------------------------------------------------------------
    // ( 3 ) ACTIVATE THE FIRMWARE SLOT ASSIGNED TO THE UPGRADE
    // ---------------------------------------------------------------------------

    //
    // Activate the newly downloaded image with the assigned slot.
    //
    RtlZeroMemory(buffer, bufferSize);

    //
    // Setup the SRB control with the firmware ioctl control info
    //
    srbControl->HeaderLength = sizeof(SRB_IO_CONTROL);
    srbControl->ControlCode = IOCTL_SCSI_MINIPORT_FIRMWARE;
    RtlMoveMemory(srbControl->Signature, IOCTL_MINIPORT_SIGNATURE_FIRMWARE, 8);
    srbControl->Timeout = 10;
    srbControl->Length = bufferSize - sizeof(SRB_IO_CONTROL);

    //
    // Set firmware request fields for FIRMWARE_FUNCTION_ACTIVATE. This request is to the controller so
    // FIRMWARE_REQUEST_FLAG_CONTROLLER is set in the flags
    //
    firmwareRequest->Version = FIRMWARE_REQUEST_BLOCK_STRUCTURE_VERSION;
    firmwareRequest->Size = sizeof(FIRMWARE_REQUEST_BLOCK);
    firmwareRequest->Function = FIRMWARE_FUNCTION_ACTIVATE;
    firmwareRequest->Flags = FIRMWARE_REQUEST_FLAG_CONTROLLER;
    firmwareRequest->DataBufferOffset = firmwareStructureOffset;
    firmwareRequest->DataBufferLength = bufferSize - firmwareStructureOffset;

    //
    // Initialize the firmware activation structure pointer to the proper position after the request structure
    //
    firmwareActivate = (PSTORAGE_FIRMWARE_ACTIVATE)((PUCHAR)srbControl + firmwareRequest->DataBufferOffset);

    //
    // Set the activation parameters with the available slot selected
    //
    firmwareActivate->Version = 3;
    firmwareActivate->Size = sizeof(STORAGE_FIRMWARE_ACTIVATE);
    firmwareActivate->SlotToActivate = (UCHAR)slotNumber;

    //
    // Send the activation request
    //
    result = DeviceIoControl(handle,
                                IOCTL_SCSI_MINIPORT,
                                buffer,
                                bufferSize,
                                buffer,
                                bufferSize,
                                &returnedLength,
                                NULL
                                );


    if (result == FALSE) {
        RecLine("FirmwareUpgrade - IOCTL - firmware activate failed. 0x%X.", GetLastError());
        goto Exit;
    }

    //
    // Display status result from firmware activation
    //
    switch (srbControl->ReturnCode) {
    case FIRMWARE_STATUS_SUCCESS:
        RecLine("FirmwareUpgrade - firmware activate succeeded.");
        ret = 0;
        break;

    case FIRMWARE_STATUS_POWER_CYCLE_REQUIRED:
        RecLine("FirmwareUpgrade - firmware activate succeeded. PLEASE REBOOT COMPUTER.");
        break;

    case FIRMWARE_STATUS_ILLEGAL_REQUEST:
    case FIRMWARE_STATUS_INVALID_PARAMETER:
    case FIRMWARE_STATUS_INPUT_BUFFER_TOO_BIG:
        RecLine("FirmwareUpgrade - firmware activate parameter error. srbControl->ReturnCode %d.\n", srbControl->ReturnCode);
        break;

    case FIRMWARE_STATUS_INVALID_SLOT:
        RecLine("FirmwareUpgrade - firmware activate, slot number invalid.\n");
        break;

    case FIRMWARE_STATUS_INVALID_IMAGE:
        RecLine("FirmwareUpgrade - firmware activate, invalid firmware image.\n");
        break;

    case FIRMWARE_STATUS_ERROR:
    case FIRMWARE_STATUS_CONTROLLER_ERROR:
        RecLine("FirmwareUpgrade - firmware activate, error returned.\n");
        break;

    default:
        _tprintf(_T("\t FirmwareUpgrade - firmware activate, unexpected error. srbControl->ReturnCode %d.\n"), srbControl->ReturnCode);
        break;
   }

Exit:

    if (fileHandle != NULL) {
        CloseHandle(fileHandle);
    }

    if (buffer != NULL) {
        free(buffer);
    }

    return ret;
}
#endif
