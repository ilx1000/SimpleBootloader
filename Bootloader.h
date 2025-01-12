#pragma once

#include <Uefi.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>

#include <Protocol/SimpleFileSystem.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>

#include <Guid/FileInfo.h>
#include <Guid/FileSystemInfo.h>

typedef enum {
  KernelTypeNone    = 0x00,
  KernelTypeLinux   = 0x01,
  KernelTypeWindows = 0x02
} OS_KERNEL_TYPE;

struct _LINUX_BOOT_DESCRIPTOR;

struct _OS_KERNEL
{
  EFI_HANDLE         Volume;
  EFI_HANDLE         Partition;
  EFI_FILE_PROTOCOL *RootDir;
  OS_KERNEL_TYPE     KernelType;
  CHAR16            *DevicePath;

  struct _LINUX_BOOT_DESCRIPTOR *LinuxBootDescriptor;
};

typedef struct _OS_KERNEL OS_KERNEL;

VOID*
AllocateMemoryPool (
  IN UINTN BufferSize
  );