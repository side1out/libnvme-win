#pragma once
/*
 * portable_ifaddrs.h
 *  - POSIX: just includes <ifaddrs.h>
 *  - Windows: provides compatible getifaddrs()/freeifaddrs() using GetAdaptersAddresses
 *
 * Populates:
 *   ifa_name      : adapter name (UTF-8)
 *   ifa_flags     : IFF_UP-ish bit (1) when adapter is up; 0 otherwise (minimal)
 *   ifa_addr      : sockaddr* (AF_INET / AF_INET6)
 *   ifa_netmask   : sockaddr* (computed from prefix length)
 *   ifa_broadaddr : sockaddr* (IPv4 broadcast if unicast + non-point-to-point)
 *   ifa_next      : forward-linked list
 *
 * Limitations:
 *   - Only basic ifa_flags are set (UP bit).
 *   - ifa_data is left NULL.
 *   - IPv6 "broadcast" is not defined; we leave ifa_broadaddr NULL for IPv6.
 */

#ifdef _WIN32

/* ===================== Windows implementation ===================== */
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600  /* Vista+ for GetAdaptersAddresses is fine */
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Minimal flag bit: mark interface "up" with bit 1 */
#ifndef IFF_UP
#define IFF_UP 0x1
#endif

/* POSIX-compatible struct ifaddrs */
struct ifaddrs {
    struct ifaddrs *ifa_next;
    char           *ifa_name;      /* UTF-8 */
    unsigned int    ifa_flags;     /* minimal: IFF_UP if up */
    struct sockaddr *ifa_addr;     /* primary address */
    struct sockaddr *ifa_netmask;  /* netmask from prefix length */
    struct sockaddr *ifa_broadaddr;/* IPv4 broadcast (if applicable) */
    void           *ifa_data;      /* unused */
};

/* --- Helpers --- */

static inline char* _pi_utf16_to_utf8(const wchar_t* w)
{
    if (!w) return NULL;
    int need = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
    if (need <= 0) return NULL;
    char* s = (char*)malloc((size_t)need);
    if (!s) return NULL;
    if (WideCharToMultiByte(CP_UTF8, 0, w, -1, s, need, NULL, NULL) <= 0) {
        free(s); return NULL;
    }
    return s;
}

static inline struct sockaddr* _pi_dup_sockaddr(const SOCKADDR* sa, int len)
{
    if (!sa || len <= 0) return NULL;
    struct sockaddr* p = (struct sockaddr*)malloc((size_t)len);
    if (!p) return NULL;
    memcpy(p, sa, (size_t)len);
    return p;
}

/* Build IPv4 netmask from prefix length */
static inline struct sockaddr* _pi_make_ipv4_netmask(ULONG prefixLen)
{
    struct sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    if (prefixLen > 32) prefixLen = 32;
    uint32_t mask = (prefixLen == 0) ? 0 : htonl(~((1u << (32 - prefixLen)) - 1u));
    sin.sin_addr.s_addr = mask;
    struct sockaddr* out = (struct sockaddr*)malloc(sizeof(sin));
    if (!out) return NULL;
    memcpy(out, &sin, sizeof(sin));
    return out;
}

/* Build IPv6 netmask from prefix length */
static inline struct sockaddr* _pi_make_ipv6_netmask(ULONG prefixLen)
{
    struct sockaddr_in6 sin6 = {0};
    sin6.sin6_family = AF_INET6;
    if (prefixLen > 128) prefixLen = 128;
    for (ULONG i = 0; i < 16; ++i) {
        int bit = (int)prefixLen - (int)(i * 8);
        unsigned char val = (unsigned char)(bit >= 8 ? 0xFF : (bit <= 0 ? 0x00 : (0xFF << (8 - bit))));
        sin6.sin6_addr.s6_addr[i] = val;
    }
    struct sockaddr* out = (struct sockaddr*)malloc(sizeof(sin6));
    if (!out) return NULL;
    memcpy(out, &sin6, sizeof(sin6));
    return out;
}

/* Compute IPv4 broadcast: addr | ~mask (for typical Ethernet, non-ptp) */
static inline struct sockaddr* _pi_ipv4_broadcast(const struct sockaddr_in* addr4,
                                                  const struct sockaddr_in* mask4)
{
    if (!addr4 || !mask4) return NULL;
    struct sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    uint32_t a = ntohl(addr4->sin_addr.s_addr);
    uint32_t m = ntohl(mask4->sin_addr.s_addr);
    sin.sin_addr.s_addr = htonl(a | (~m));
    struct sockaddr* out = (struct sockaddr*)malloc(sizeof(sin));
    if (!out) return NULL;
    memcpy(out, &sin, sizeof(sin));
    return out;
}

/* --- Public API: getifaddrs/freeifaddrs --- */

static inline int getifaddrs(struct ifaddrs **ifap)
{
    if (!ifap) return -1;

    ULONG flags = GAA_FLAG_INCLUDE_PREFIX |
                  GAA_FLAG_SKIP_ANYCAST   |
                  GAA_FLAG_SKIP_MULTICAST |
                  GAA_FLAG_SKIP_DNS_SERVER;

    ULONG sz = 0;
    DWORD rc = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, NULL, &sz);
    if (rc != ERROR_BUFFER_OVERFLOW) return -1;

    IP_ADAPTER_ADDRESSES *aa = (IP_ADAPTER_ADDRESSES*)malloc(sz);
    if (!aa) return -1;

    rc = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, aa, &sz);
    if (rc != NO_ERROR) { free(aa); return -1; }

    struct ifaddrs *head = NULL, *tail = NULL;

    for (IP_ADAPTER_ADDRESSES *a = aa; a; a = a->Next) {
        /* Choose a UTF-8 name: FriendlyName if possible, else AdapterName */
        char *name_utf8 = _pi_utf16_to_utf8(a->FriendlyName);
        if (!name_utf8) name_utf8 = _strdup(a->AdapterName ? a->AdapterName : "unknown");

        unsigned int upflag = (a->OperStatus == IfOperStatusUp) ? IFF_UP : 0;

        for (IP_ADAPTER_UNICAST_ADDRESS *u = a->FirstUnicastAddress; u; u = u->Next) {
            int fam = u->Address.lpSockaddr ? u->Address.lpSockaddr->sa_family : AF_UNSPEC;
            int slen = (int)u->Address.iSockaddrLength;
            if (!(fam == AF_INET || fam == AF_INET6) || slen <= 0) continue;

            struct ifaddrs *ifa = (struct ifaddrs*)calloc(1, sizeof(*ifa));
            if (!ifa) continue;

            /* Link into list */
            if (tail) tail->ifa_next = ifa; else head = ifa;
            tail = ifa;

            ifa->ifa_name = _strdup(name_utf8);
            ifa->ifa_flags = upflag;
            ifa->ifa_addr  = _pi_dup_sockaddr(u->Address.lpSockaddr, slen);

            /* Netmask from OnLinkPrefixLength */
            if (fam == AF_INET) {
                ifa->ifa_netmask = _pi_make_ipv4_netmask(u->OnLinkPrefixLength);
                /* Broadcast (best-effort): only for IPv4 and if we have an address/netmask */
                if (ifa->ifa_addr && ifa->ifa_netmask) {
                    const struct sockaddr_in* a4 = (const struct sockaddr_in*)ifa->ifa_addr;
                    const struct sockaddr_in* m4 = (const struct sockaddr_in*)ifa->ifa_netmask;
                    ifa->ifa_broadaddr = _pi_ipv4_broadcast(a4, m4);
                }
            } else if (fam == AF_INET6) {
                ifa->ifa_netmask = _pi_make_ipv6_netmask(u->OnLinkPrefixLength);
                /* No broadcast for IPv6 */
            }
        }

        free(name_utf8);
    }

    free(aa);
    *ifap = head;
    return 0;
}

static inline void freeifaddrs(struct ifaddrs *ifa)
{
    while (ifa) {
        struct ifaddrs *next = ifa->ifa_next;
        free(ifa->ifa_name);
        free(ifa->ifa_addr);
        free(ifa->ifa_netmask);
        free(ifa->ifa_broadaddr);
        free(ifa);
        ifa = next;
    }
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#else  /* ===================== POSIX path ===================== */

#include <ifaddrs.h>

#endif /* _WIN32 */
