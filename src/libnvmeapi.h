#ifndef _LIBNVME_API_H
#define _LIBNVME_API_H

#ifdef WINDOWS_GCC
  #ifdef NVME_LIB
    // When building the DLL itself
    #define NVME_API __declspec(dllexport)
  #else
    // When including this header in other projects that use the DLL
    #define NVME_API __declspec(dllimport)
  #endif
#else
  // Non-Windows (Linux, macOS): visibility("default") works the same way
  #define NVME_API __attribute__((visibility("default")))
#endif

#endif