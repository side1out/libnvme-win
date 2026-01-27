# libnvme Windows Port

This directory contains a junction/link to the separate **libnvme-win** repository.

## Repository Structure

- **Location**: `c:\sourcetree\libnvme-win` (actual repository)
- **Junction**: `nvme-cli-win\subprojects\libnvme` → `libnvme-win`
- **Purpose**: Allows libnvme to be developed separately while integrated into nvme-cli build

## Architecture

### Windows Compatibility Layer

The Windows port implements NVMe functionality through Windows-specific APIs:

**Key Components:**
- `src/nvme/windows/ioctl.c` - Windows IOCTL implementations
- `src/nvme/windows/ioctl.h` - IOCTL declarations
- `src/nvme/windows/compat.c` - Windows compatibility functions
- `src/nvme/windows/types.h` - Windows type definitions
- `src/nvme/windows/syslog.h` - Syslog compatibility layer

**Modified Core Files:**
- `src/nvme/ioctl.c` - Conditional compilation for Windows IOCTLs
- `src/nvme/tree.h` - WinSock2 vs netinet/in.h
- `src/nvme/private.h` - Socket header compatibility
- `src/nvme/types.h` - Windows types vs Linux types
- `src/nvme/util.c` - Windows networking compatibility
- `src/nvme/fabrics.c` - Network compatibility
- `src/nvme/log.h` - Windows syslog support
- `src/nvme/mi.h` - Endianness handling

### Preprocessor Guards

Windows-specific code is wrapped with:
```c
#ifdef WINDOWS_GCC
  // Windows implementation
#else
  // Linux implementation
#endif
```

## Current Version

- **Base**: libnvme v1.15
- **Upstream**: v1.16.1 available (not yet merged)
- **Commits behind master**: 4 commits

## Building

libnvme is built automatically as part of nvme-cli build via meson subproject.

### Standalone Build (Advanced)

```bash
cd c:\sourcetree\libnvme-win

# Configure
meson setup .build --buildtype=debug

# Build
meson compile -C .build
```

**Outputs:**
- `libnvme.dll` - Core NVMe library
- `libnvme-mi.dll` - Management Interface library

## Windows-Specific Changes

### Networking
- Uses `WinSock2.h` instead of `netinet/in.h`, `arpa/inet.h`
- Socket initialization via WSAStartup/WSACleanup
- Network address conversion via Windows APIs

### Types
- Custom type definitions in `windows/types.h`
- Avoids conflicts with Windows SDK `nvme.h` enums
- Custom endian conversion routines

### IOCTL Interface
- Uses Windows DeviceIoControl API
- NVMe passthrough via `IOCTL_STORAGE_*` commands
- Device enumeration via SetupAPI

### Excluded Features
- MCTP (Management Component Transport Protocol) - Linux-specific
- NBFT (NVMe Boot Firmware Table) - Requires platform support
- Some MI (Management Interface) features

## Development Workflow

### Making Changes

1. **Edit files** in `c:\sourcetree\libnvme-win\` (actual repo)
2. **Test** via nvme-cli build (uses junction automatically)
3. **Commit** in libnvme-win repository
4. **Keep in sync** with nvme-cli-win project

### Merging Upstream v1.16.1

When ready to merge upstream changes:

```bash
cd c:\sourcetree\libnvme-win

# Fetch latest
git fetch origin

# Option 1: Rebase windows-port onto master
git checkout windows-port
git rebase origin/master

# Option 2: Merge master into windows-port
git merge origin/master
```

**Expected Conflicts:**
- Files with `#ifdef WINDOWS_GCC` guards
- meson.build (Windows-specific configuration)
- src/nvme/types.h (enum conflicts)
- src/nvme/ioctl.c (IOCTL implementations)

After merge, rebuild nvme-cli-win to verify compatibility.

## Known Limitations (v1.15 base)

- Missing v1.16+ API improvements
- Struct differences may affect nvme-cli compatibility
- Some newer features unavailable

## Testing

Windows-specific testing focuses on:
- DLL loading and symbol resolution
- IOCTL passthrough to NVMe devices
- Memory allocation/deallocation
- Network operations (for fabrics support if enabled)

## References

- **Upstream**: https://github.com/linux-nvme/libnvme
- **Windows Port**: https://github.com/side1out/libnvme-win
- **Documentation**: https://libnvme.readthedocs.io/

---

**Note**: This is a development junction. Modifications made here affect the actual repository at `c:\sourcetree\libnvme-win`.
