#pragma once

#include "Bootloader.h"

#include <Protocol/LoadedImage.h>

/*
  * Сигнатура ядра системы
*/
#define LINUX_BOOT_HEADER_MAGIC 0x53726448

/*
  * Заголовок ядра системы
  * По этой информации идет дальнейший процесс загрузки
*/
struct _LINUX_BOOT_HEADER
{
  UINT8  setup_sects;
  UINT16 root_flags;
  UINT32 syssize;
  UINT16 ram_size;
  UINT16 vid_mode;
  UINT16 root_dev;
  UINT16 boot_flag;
  UINT8  jump_opcode[2];
  UINT32 magic;
  UINT16 version;
  UINT32 realmode_switch;
  UINT16 start_sys_seg;
  UINT16 kernel_version;
  UINT8  type_of_loader;
  UINT8  loadflags;
  UINT16 setup_move_size;
  UINT32 code32_start;
  UINT32 ramdisk_image;
  UINT32 ramdisk_size;
  UINT32 bootsect_kludge;
  UINT16 heap_end_ptr;
  UINT8  ext_loader_ver;
  UINT8  ext_loader_type;
  UINT32 cmd_line_ptr;
  UINT32 initrd_addr_max;
  UINT32 kernel_alignment;
  UINT8  relocatable_kernel;
  UINT8  min_alignment;
  UINT16 xloadflags;
  UINT32 cmdline_size;
  UINT32 hardware_subarch;
  UINT8  hardware_subarch_data[8];
  UINT32 payload_offset;
  UINT32 payload_length;
  UINT8  setup_data[8];
  UINT8  pref_address[8];
  UINT32 init_size;
  UINT32 handover_offset;
  UINT32 kernel_info_offset;
} __attribute__((packed));

typedef struct _LINUX_BOOT_HEADER LINUX_BOOT_HEADER;

struct _LINUX_BOOT_DESCRIPTOR
{
  EFI_FILE_PROTOCOL *KernelFile;
  EFI_FILE_PROTOCOL *InitrdFile;
};

typedef struct _LINUX_BOOT_DESCRIPTOR LINUX_BOOT_DESCRIPTOR;

EFI_STATUS
LookLinux (
  IN OS_KERNEL *KernelInfo
  );

VOID
BootLinux (
  IN LINUX_BOOT_DESCRIPTOR *BootDescriptor
  );