#include "LinuxBoot.h"
#include "Bootloader.h"

/*
  * Получить размер файла в файловой системе
*/
static EFI_STATUS
GetFileSize (
  IN EFI_FILE_PROTOCOL *FileHandle,
  OUT UINTN *FileSize
  )
{
  static EFI_GUID Guid = EFI_FILE_INFO_ID;

  UINTN BufferSize = sizeof (EFI_FILE_INFO) + 256;
  EFI_FILE_INFO *FileInfo = AllocateMemoryPool (BufferSize);

  EFI_STATUS Status = FileHandle->GetInfo (FileHandle, &Guid, &BufferSize, FileInfo);

  if (EFI_ERROR (Status))
    {
      gBS->FreePool (FileInfo);
      return Status;
    }

  *FileSize = FileInfo->FileSize;
  gBS->FreePool (FileInfo);

  return EFI_SUCCESS;
}

/*
  * Попытаться найти ядро системы и файл initrd необходимый для загрузки
*/
EFI_STATUS
LookLinux (
  IN OS_KERNEL *KernelInfo
  )
{
  EFI_FILE_PROTOCOL *RootDir = KernelInfo->RootDir;

  EFI_FILE_PROTOCOL *KernelFile, *InitrdFile;

  EFI_STATUS Status = RootDir->Open (RootDir, &KernelFile, L"vmlinuz-linux",
                                     EFI_FILE_READ_ONLY, 0);

  if (EFI_ERROR (Status))
    {
      Print (L"Linux kernel not found\n");
      return EFI_NOT_FOUND;
    }

  Status = RootDir->Open (RootDir, &InitrdFile, L"initramfs-linux.img",
                                     EFI_FILE_READ_ONLY, 0); // TODO

  if (EFI_ERROR (Status))
    {
      Print (L"Initrd file not found\n");
      KernelFile->Close (KernelFile);

      return EFI_NOT_FOUND;
    }

  // fill boot descriptor

  KernelInfo->KernelType |= KernelTypeLinux;

  KernelInfo->LinuxBootDescriptor = AllocateMemoryPool (sizeof (LINUX_BOOT_DESCRIPTOR));
  KernelInfo->LinuxBootDescriptor->KernelFile = KernelFile;
  KernelInfo->LinuxBootDescriptor->InitrdFile = InitrdFile;

  BootLinux (KernelInfo->LinuxBootDescriptor);

  return EFI_SUCCESS;
}

typedef VOID (*LinuxHandoverEntry)(VOID *image, EFI_SYSTEM_TABLE *table, LINUX_BOOT_HEADER *setup);

VOID
BootLinux (
  IN LINUX_BOOT_DESCRIPTOR *BootDescriptor
  )
{
  UINTN KernelSize, InitrdSize;

  if (EFI_ERROR (GetFileSize (BootDescriptor->KernelFile, &KernelSize))
      || EFI_ERROR (GetFileSize (BootDescriptor->InitrdFile, &InitrdSize)))
    {
      Print (L"Failed to boot linux: unable to retrieve files info\n");
      return;
    }

  UINT8 *ImageKernel = NULL;

  EFI_STATUS Status = gBS->AllocatePool (EfiBootServicesCode, KernelSize, (VOID**)&ImageKernel);

  if (EFI_ERROR (Status))
  {
    Print (L"Failed to allocate memory for kernel: %r\n", Status);
    return;
  }

  Status = BootDescriptor->KernelFile->Read (BootDescriptor->KernelFile,
                                                        &KernelSize, ImageKernel);

  if (EFI_ERROR (Status))
    {
      Print (L"Failed to boot linux: unable to read kernel image: %r\n", Status);
      return;
    }

  LINUX_BOOT_HEADER *Header = (LINUX_BOOT_HEADER*) ((UINT8*)ImageKernel + 0x1F1);

  if (Header->magic != LINUX_BOOT_HEADER_MAGIC)
    {
      Print (L"Invalid kernel magic!\n");
      return;
    }

  UINT8 *Initrd = AllocateMemoryPool (InitrdSize);

  Status = BootDescriptor->InitrdFile->Read (BootDescriptor->InitrdFile,
                                             &InitrdSize, Initrd);

  if (EFI_ERROR (Status))
    {
      Print (L"Failed to boot linux: unable to read initrd file: %r\n", Status);
      return;
    }

  Header->code32_start = (UINT64)ImageKernel + ((Header->setup_sects + 1) * 512);
  Header->cmd_line_ptr = 0;
  Header->cmdline_size = 0;

  LINUX_BOOT_HEADER *NewHeader = AllocateMemoryPool (sizeof(LINUX_BOOT_HEADER));
  
  LinuxHandoverEntry Handover = (LinuxHandoverEntry) ((UINT64) Header->code32_start + Header->handover_offset);

  NewHeader->cmd_line_ptr = 0;
  NewHeader->cmdline_size = 0;

  Print (L"ImageKernel base address: %x\n", (UINT64) ImageKernel);
  Print (L"Code32 start: %x\n", (UINT64) Header->code32_start);
  Print (L"Handover offset: %x\n", (UINT64) Header->handover_offset);
  Print (L"Handover Code: %x\n", (UINT64) Header->code32_start + Header->handover_offset);

  Print (L"Kernel size: %d\n", KernelSize);
  Print (L"Initrd size: %d\n", InitrdSize);
  Print (L"Preffered address: %p\n", Header->pref_address);
}
