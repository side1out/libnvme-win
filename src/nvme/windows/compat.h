

#ifndef _WIN_COMPAT_H
#define _WIN_COMPAT_H

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <io.h>
#include <stdint.h>
#include <time.h>
#include <dirent.h>
#include <stdarg.h>
#include <string.h>


#include "net_compat.h"

#define fsync(fd) _commit(fd)
#define sleep(x) Sleep((x) * 1000)
#define close _close
#define malloc_usable_size(p) _msize(p)

// added for libnvme 

#ifndef EREMOTEIO
#define EREMOTEIO EIO
#endif
#ifndef EDQUOT
#define EDQUOT EFBIG
#endif
#ifndef ERESTART
#define ERESTART EINTR
#endif
#ifndef ENOKEY
#define ENOKEY ENOENT  /* reasonable fallback: treat as “missing” */
#endif
#ifndef EKEYEXPIRED
#define EKEYEXPIRED EACCES
#endif
#ifndef EKEYREJECTED
#define EKEYREJECTED EACCES
#endif

#ifndef DT_UNKNOWN
#define DT_UNKNOWN  0
#endif
#ifndef DT_FIFO
#define DT_FIFO     1
#endif
#ifndef DT_CHR
#define DT_CHR      2
#endif
#ifndef DT_DIR
#define DT_DIR      4
#endif
#ifndef DT_BLK
#define DT_BLK      6
#endif
#ifndef DT_REG
#define DT_REG      8
#endif
#ifndef DT_LNK
#define DT_LNK     10
#endif
#ifndef DT_SOCK
#define DT_SOCK    12
#endif

#ifndef HAVE_D_TYPE
#define HAVE_D_TYPE 1
#endif

/* Portable macro: get entry type, fallback to stat() if d_type not available */
static inline unsigned char portable_d_type(const char *dirpath, const struct dirent *ent) {
    /* Windows dirent may not have d_type; use attributes instead */
    char fullpath[MAX_PATH];
    snprintf(fullpath, sizeof(fullpath), "%s\\%s", dirpath, ent->d_name);
    DWORD attr = GetFileAttributesA(fullpath);
    if (attr == INVALID_FILE_ATTRIBUTES)
        return DT_UNKNOWN;
    if (attr & FILE_ATTRIBUTE_DIRECTORY)
        return DT_DIR;
    return DT_REG;
}

#include <process.h>
#ifndef getpid
#define getpid _getpid
#endif

#define _cleanup_(x)
#define _cleanup_dir_
#else
#if defined(__GNUC__) || defined(__clang__)
#define _cleanup_(x) __attribute__((cleanup(x)))
#else
#define _cleanup_(x)
#endif
static inline void closedirp(DIR **d) { if (d && *d) { closedir(*d); *d = NULL; } }
#define _cleanup_dir_ _cleanup_(closedirp)


#define MIN min

#include <malloc.h>   // for _aligned_malloc / _aligned_free

static inline void *reallocarray(void *ptr, size_t nmemb, size_t size)
{
    /* Overflow check: nmemb * size > SIZE_MAX */
    if (nmemb && size > SIZE_MAX / nmemb) {
        errno = ENOMEM;
        return NULL;
    }
    return realloc(ptr, nmemb * size);
}

static inline int posix_memalign(void **memptr, size_t alignment, size_t size)
{
    if (!memptr) return EINVAL;
    // alignment must be power of two and multiple of sizeof(void*)
    if ((alignment & (alignment - 1)) || (alignment % sizeof(void*) != 0))
        return EINVAL;

    void *p = _aligned_malloc(size, alignment);
    if (!p)
        return ENOMEM;

    *memptr = p;
    return 0;
}

static inline int gettimeofday_win(struct timeval *tv, void *tz)
{
    FILETIME ft;
    ULARGE_INTEGER uli;
    const unsigned __int64 EPOCH_DIFF = 116444736000000000ULL;
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart  = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    unsigned __int64 t = uli.QuadPart - EPOCH_DIFF;
    tv->tv_sec  = (long)(t / 10000000ULL);
    tv->tv_usec = (long)((t % 10000000ULL) / 10);
    return 0;
}
#ifndef gettimeofday
#define gettimeofday(tv, tz) gettimeofday_win(tv, tz)
#endif

/*
static char * basename(const char *path) {
    static char fname[_MAX_FNAME];
    static char ext[_MAX_EXT];
    _splitpath(path, NULL, NULL, fname, ext);
    static char result[_MAX_FNAME + _MAX_EXT];
    snprintf(result, sizeof(result), "%s%s", fname, ext);
    return result;
}
*/    

static long getpagesize(void)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (long)si.dwPageSize;
}

ssize_t getline(char **lineptr, size_t *n, FILE *stream);

/* Portable replacement for open_memstream().
 *
 * On platforms with open_memstream():
 *   - open_memstream_compat == open_memstream
 *   - msync_memstream() just fflush() (POSIX updates *bufp *sizep)
 *   - close_memstream() == fclose()
 *
 * On Windows/MinGW (no open_memstream):
 *   - returns a FILE* to a temporary binary file ("wb+")
 *   - write with fprintf/fwrite as usual
 *   - call msync_memstream() to read current contents into *bufp *sizep
 *   - call close_memstream() at the end to get final buffer and close
 *   - free(*bufp) when finished
 */

/* Open a memory-like stream.
 *   bufp/sizep are output holders that will be set by msync/close.
 *   Returns FILE* or NULL on error.
 */
FILE* open_memstream(char **bufp, size_t *sizep);

/* Refresh *bufp *sizep to the current contents of the stream.
 *   Returns 0 on success, -1 on error (errno may be set).
 *   On POSIX (real open_memstream), this is equivalent to fflush(fp).
 */
int msync_memstream(FILE *fp, char **bufp, size_t *sizep);

/* Close the stream and return the final buffer.
 *   On success: returns 0, sets *bufp *sizep (NUL-terminated), closes fp.
 *   On failure: returns -1.
 */
int close_memstream(FILE *fp, char **bufp, size_t *sizep);



#ifndef MAP_FAILED
#define MAP_FAILED ((void*)-1)
#endif

/* Define POSIX-ish flags if missing */
#ifndef PROT_NONE
#define PROT_NONE  0x00
#endif
#ifndef PROT_READ
#define PROT_READ  0x01
#endif
#ifndef PROT_WRITE
#define PROT_WRITE 0x02
#endif
#ifndef PROT_EXEC
#define PROT_EXEC  0x04
#endif

#ifndef MAP_SHARED
#define MAP_SHARED   0x01
#endif
#ifndef MAP_PRIVATE
#define MAP_PRIVATE  0x02
#endif
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS 0x20
#endif
#ifndef MAP_FIXED
#define MAP_FIXED    0x10  /* ignored on Windows */
#endif
#ifndef MAP_HUGETLB
#define MAP_HUGETLB  0x40000
#endif

/* Convert POSIX prot -> Windows PAGE_* protect */
static inline DWORD _mmap_page_prot(int prot)
{
    if (prot == PROT_NONE) return PAGE_NOACCESS;

    const int rw = (prot & PROT_WRITE) != 0;
    const int ex = (prot & PROT_EXEC)  != 0;
    if (ex) {
        return rw ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
    } else {
        return rw ? PAGE_READWRITE : PAGE_READONLY;
    }
}

/* Desired access for MapViewOfFile */
static inline DWORD _mmap_view_access(int prot, int flags)
{
    DWORD acc = 0;
    if (flags & MAP_PRIVATE) {
        acc |= FILE_MAP_COPY;                /* copy-on-write */
    } else {
        if (prot & PROT_READ)  acc |= FILE_MAP_READ;
        if (prot & PROT_WRITE) acc |= FILE_MAP_WRITE;
    }
#ifdef FILE_MAP_EXECUTE
    if (prot & PROT_EXEC) acc |= FILE_MAP_EXECUTE;
#endif
    return acc ? acc : FILE_MAP_READ;
}

static inline void _set_errno_from_winerr(DWORD werr)
{
    switch (werr) {
    case ERROR_NOT_ENOUGH_MEMORY:
    case ERROR_COMMITMENT_LIMIT:
    case ERROR_OUTOFMEMORY: errno = ENOMEM; break;
    case ERROR_PRIVILEGE_NOT_HELD:          errno = EPERM;  break;
    case ERROR_INVALID_PARAMETER:           errno = EINVAL; break;
    default:                                errno = EINVAL; break;
    }
}

/* Anonymous mapping: VirtualAlloc (optionally with large pages) */
static inline void* _mmap_anon(void* addr, size_t length, int prot, int flags)
{
    DWORD protect = _mmap_page_prot(prot);
    DWORD alloc   = MEM_RESERVE | MEM_COMMIT;
    if (flags & MAP_HUGETLB) {
        SIZE_T lp = GetLargePageMinimum();
        if (lp == 0 || (length % lp) != 0) { errno = EINVAL; return MAP_FAILED; }
        alloc |= MEM_LARGE_PAGES;
        /* Caller must have SeLockMemoryPrivilege enabled; otherwise VirtualAlloc fails. */
    }
    void* p = VirtualAlloc(addr, length, alloc, protect);
    if (!p) { _set_errno_from_winerr(GetLastError()); return MAP_FAILED; }
    return p;
}

/* File-backed mapping via CreateFileMapping/MapViewOfFile */
static inline void* _mmap_file(void* addr, size_t length, int prot, int flags,
                               int fd, int64_t offset)
{
    HANDLE hFile = (HANDLE)_get_osfhandle(fd);
    if (hFile == INVALID_HANDLE_VALUE) { errno = EBADF; return MAP_FAILED; }

    /* Section protection—must be compatible with view protection later */
    DWORD secProtect = _mmap_page_prot(prot);
    if (flags & MAP_PRIVATE) {
        /* For COW, section must be created with WRITECOPY-style protection. */
        secProtect = (prot & PROT_EXEC) ? PAGE_EXECUTE_WRITECOPY : PAGE_WRITECOPY;
    }

    /* Large pages for sections (file or pagefile-backed) */
    DWORD secAttrs = 0;
#ifdef SEC_LARGE_PAGES
    if (flags & MAP_HUGETLB) {
        SIZE_T lp = GetLargePageMinimum();
        if (lp == 0 || (length % lp) != 0) { errno = EINVAL; return MAP_FAILED; }
        secAttrs |= SEC_LARGE_PAGES;
    }
#endif

    HANDLE hMap = CreateFileMapping(hFile, NULL, secProtect | secAttrs,
                                    (DWORD)((length >> 32) & 0xffffffff),
                                    (DWORD)(length & 0xffffffff), NULL);
    if (!hMap) { _set_errno_from_winerr(GetLastError()); return MAP_FAILED; }

    SYSTEM_INFO si; GetSystemInfo(&si);
    SIZE_T gran = si.dwAllocationGranularity; /* usually 64 KiB */

    uint64_t off = (uint64_t)offset;
    uint64_t alignedOff = off & ~((uint64_t)gran - 1);
    SIZE_T   delta      = (SIZE_T)(off - alignedOff);

    DWORD offHi = (DWORD)((alignedOff >> 32) & 0xffffffff);
    DWORD offLo = (DWORD)(alignedOff & 0xffffffff);
    SIZE_T mapLen = length + delta; /* ensure requested region fits in the view */

    DWORD access = _mmap_view_access(prot, flags);

    void* base = MapViewOfFile(hMap, access, offHi, offLo, mapLen);
    CloseHandle(hMap);
    if (!base) { _set_errno_from_winerr(GetLastError()); return MAP_FAILED; }

    /* Return pointer adjusted to the requested offset */
    return (void*)((char*)base + delta);
}

/* Public API: mmap/munmap/msync/mprotect */
static inline void* mmap(void* addr, size_t length, int prot, int flags, int fd, long long offset)
{
    if (length == 0) { errno = EINVAL; return MAP_FAILED; }

    if (flags & MAP_ANONYMOUS) {
        /* On Windows, anonymous mappings are pagefile-backed allocations. */
        return _mmap_anon(addr, length, prot, flags);
    } else {
        return _mmap_file(addr, length, prot, flags, fd, (int64_t)offset);
    }
}

static inline int munmap(void* addr, size_t length)
{
    (void)length; /* not needed by Win32 APIs */

    /* If this was a VirtualAlloc region: try to free it */
    if (VirtualFree(addr, 0, MEM_RELEASE)) return 0;

    /* If that failed, assume it's a file mapping view.
       If addr was returned as base+delta, align down to granularity to
       retrieve the actual base pointer MapViewOfFile returned. */
    SYSTEM_INFO si; GetSystemInfo(&si);
    SIZE_T gran = si.dwAllocationGranularity;
    uintptr_t p  = (uintptr_t)addr;
    void* base   = (void*)(p & ~((uintptr_t)gran - 1));

    if (UnmapViewOfFile(base)) return 0;

    _set_errno_from_winerr(GetLastError());
    return -1;
}

static inline int msync(void* addr, size_t length, int flags /*ignored*/)
{
    (void)flags;
    /* Try flushing as a mapped view; for VirtualAlloc there's nothing to flush */
    if (FlushViewOfFile(addr, length)) return 0;

    /* If not a file mapping, succeed (POSIX allows MS_SYNC/MS_ASYNC no-ops for anonymous) */
    DWORD err = GetLastError();
    if (err == ERROR_MAPPED_ALIGNMENT || err == ERROR_INVALID_HANDLE) return 0;

    _set_errno_from_winerr(err);
    return -1;
}

static inline int mprotect(void* addr, size_t length, int prot)
{
    DWORD newProt = _mmap_page_prot(prot);
    DWORD oldProt = 0;
    if (VirtualProtect(addr, length, newProt, &oldProt)) return 0;

    _set_errno_from_winerr(GetLastError());
    return -1;
}

#include <signal.h>

/* Define sigset_t placeholder for Windows */
#ifndef SIGSET_T_DEFINED
typedef unsigned long sigset_t;
#define SIGSET_T_DEFINED
#endif

typedef void (*sighandler_t)(int);

struct sigaction {
    sighandler_t sa_handler;
    unsigned long sa_flags;
    void (*sa_restorer)(void);
    sigset_t sa_mask;
};

/* Flags (ignored, just for compatibility) */
#define SA_RESTART   0x10000000
#define SA_NOCLDSTOP 0x00000001
#define SA_SIGINFO   0x00000004

/* Very basic sigaction() wrapper using signal() */
static inline int sigaction(int signum,
                            const struct sigaction *act,
                            struct sigaction *oldact)
{
    if (oldact) {
        sighandler_t old = signal(signum, SIG_DFL);
        signal(signum, old);
        oldact->sa_handler = old;
        oldact->sa_flags = 0;
    }
    if (act) {
        signal(signum, act->sa_handler);
    }
    return 0;
}
/* Initialize an empty signal set (no-op) */
static inline int sigemptyset(sigset_t *set)
{
    if (set) *set = 0;
    return 0;
}

/* Add a signal number to a set (no-op, track bit for formality) */
static inline int sigaddset(sigset_t *set, int signum)
{
    if (!set) return -1;
    *set |= (1u << (signum & 31)); // fake bitmask
    return 0;
}

/* Remove a signal number (no-op) */
static inline int sigdelset(sigset_t *set, int signum)
{
    if (!set) return -1;
    *set &= ~(1u << (signum & 31));
    return 0;
}

/* Check membership (always 0/1 for fake bitmask) */
static inline int sigismember(const sigset_t *set, int signum)
{
    if (!set) return 0;
    return ((*set & (1u << (signum & 31))) != 0);
}

/* Set current signal mask (no-op) */
static inline int sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    (void)how; (void)set;
    if (oldset) *oldset = 0;
    return 0;
}
static inline struct tm *gmtime_r(const time_t *timep, struct tm *result)
{
    if (gmtime_s(result, timep) != 0)
        return NULL;
    return result;
}

// libnvme functions

static inline int dprintf(int fd, const char *fmt, ...)
{
    char buf[2048];
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (len < 0)
        return len;

    /* Ensure full write (truncate if too large) */
    if (len > (int)sizeof(buf))
        len = (int)sizeof(buf);

    return _write(fd, buf, len);
}
static inline char *strndup(const char *s, size_t n)
{
    if (!s)
        return NULL;

    size_t len = strnlen(s, n);      /* copy at most n bytes */
    char *copy = (char*)malloc(len + 1);
    if (!copy)
        return NULL;

    memcpy(copy, s, len);
    copy[len] = '\0';
    return copy;
}
static inline char *realpath_win(const char *path, char *resolved_path)
{
    if (!path)
        return NULL;

    char full[MAX_PATH];
    DWORD len = GetFullPathNameA(path, MAX_PATH, full, NULL);
    if (len == 0 || len >= MAX_PATH)
        return NULL;

    if (resolved_path)
        strcpy(resolved_path, full);
    else {
        resolved_path = (char*)malloc(strlen(full) + 1);
        if (!resolved_path)
            return NULL;
        strcpy(resolved_path, full);
    }
    return resolved_path;
}
#ifndef alphasort
static inline int alphasort(const struct dirent **a, const struct dirent **b)
{
    return _stricmp((*a)->d_name, (*b)->d_name);
}
#endif

/* scandir() implementation */
static inline int scandir(
    const char *dirname,
    struct dirent ***namelist,
    int (*filter)(const struct dirent *),
    int (*compar)(const struct dirent **, const struct dirent **))
{
    if (!dirname || !namelist) {
        errno = EINVAL;
        return -1;
    }

    DIR *dir = opendir(dirname);
    if (!dir)
        return -1;

    size_t count = 0, capacity = 16;
    struct dirent **list = (struct dirent **)malloc(capacity * sizeof(*list));
    if (!list) {
        closedir(dir);
        errno = ENOMEM;
        return -1;
    }

    struct dirent *ent;
    while ((ent = readdir(dir))) {
        if (filter && !filter(ent))
            continue;

        struct dirent *copy = (struct dirent *)malloc(sizeof(*copy));
        if (!copy) {
            closedir(dir);
            for (size_t i = 0; i < count; ++i) free(list[i]);
            free(list);
            errno = ENOMEM;
            return -1;
        }
        memcpy(copy, ent, sizeof(*copy));

        if (count == capacity) {
            capacity *= 2;
            struct dirent **tmp = (struct dirent **)realloc(list, capacity * sizeof(*tmp));
            if (!tmp) {
                closedir(dir);
                for (size_t i = 0; i < count; ++i) free(list[i]);
                free(list);
                errno = ENOMEM;
                return -1;
            }
            list = tmp;
        }
        list[count++] = copy;
    }

    closedir(dir);

    if (compar)
        qsort(list, count, sizeof(*list),
              (int (*)(const void*, const void*))compar);

    *namelist = list;
    return (int)count;
}

#endif
