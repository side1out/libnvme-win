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

#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

static bool addr_equals(const SOCKADDR *sa, const char *addr)
{
    char buf[INET6_ADDRSTRLEN];
    void *src = NULL;
    int family = sa->sa_family;
    if (family == AF_INET) {
        src = &((const struct sockaddr_in *)sa)->sin_addr;
    } else if (family == AF_INET6) {
        src = &((const struct sockaddr_in6 *)sa)->sin6_addr;
    } else return false;

    if (!InetNtopA(family, src, buf, sizeof(buf))) return false;
    return _stricmp(buf, addr) == 0;
}

bool nvme_iface_primary_addr_matches(const void *unused,
                                     const char *iface, const char *addr)
{
    (void)unused;
    ULONG sz = 0;
    GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST|GAA_FLAG_SKIP_MULTICAST|
                                   GAA_FLAG_SKIP_DNS_SERVER, NULL, NULL, &sz);
    IP_ADAPTER_ADDRESSES *aa = (IP_ADAPTER_ADDRESSES*)malloc(sz);
    if (!aa) return false;
    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST|GAA_FLAG_SKIP_MULTICAST|
                                        GAA_FLAG_SKIP_DNS_SERVER, NULL, aa, &sz) != NO_ERROR) {
        free(aa); return false;
    }

    bool match = false;
    for (IP_ADAPTER_ADDRESSES *a = aa; a; a = a->Next) {
        /* match by FriendlyName or AdapterName */
        if (iface &&
            _wcsicmp(a->FriendlyName, (const wchar_t*)_bstr_t(iface)) != 0 &&
            _stricmp(a->AdapterName, iface) != 0)
            continue;

        for (IP_ADAPTER_UNICAST_ADDRESS *u = a->FirstUnicastAddress; u; u = u->Next) {
            if (addr_equals(u->Address.lpSockaddr, addr)) { match = true; break; }
        }
        if (match) break;
    }
    free(aa);
    return match;
}