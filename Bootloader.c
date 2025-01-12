#include "Bootloader.h"
#include "LinuxBoot.h"

/*
  * Выделить память
  * Если по какой-то причине не выходит, выдать ошибку
*/
VOID*
AllocateMemoryPool (
  IN UINTN BufferSize
  )
{
  VOID *Buffer;
  EFI_STATUS Status = gBS->AllocatePool (EfiBootServicesData, BufferSize, &Buffer);

  if (EFI_ERROR (Status))
    {
      Print (L"Failed to allocate memory: %r\n", Status);
      gBS->Exit (gImageHandle, Status, 0, NULL);
    }

  return Buffer;
}


/*
  * Запросить список логических разделов с всех устройств
*/
static EFI_STATUS
QueryPartitions (
  OUT EFI_HANDLE **Handles,
  OUT UINTN       *HandlesCount
  )
{
  return gBS->LocateHandleBuffer (ByProtocol, &gEfiBlockIoProtocolGuid,
                                               NULL, HandlesCount, Handles);
}

/*
  * Открыть логический раздел и попробовать найти в нем ядро системы
*/
static EFI_STATUS
OpenPartition (
  IN EFI_HANDLE                       Volume,
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Partition,
  OUT OS_KERNEL                      *KernelInfo
  )
{
  static EFI_GUID FileSystemInfoGuid = EFI_FILE_SYSTEM_INFO_ID;

  EFI_FILE_PROTOCOL *RootDir;

  EFI_STATUS Status = Partition->OpenVolume (Partition, &RootDir);

  if (EFI_ERROR (Status))
    return Status;

  UINTN BufferSize = sizeof(EFI_FILE_SYSTEM_INFO) + 256;
  EFI_FILE_SYSTEM_INFO *FileSystemInfo = AllocateMemoryPool (BufferSize);

  if (EFI_ERROR (Status))
    return Status;

  Status = RootDir->GetInfo (RootDir, &FileSystemInfoGuid, &BufferSize, FileSystemInfo);

  if (EFI_ERROR (Status))
    return Status;

  EFI_DEVICE_PATH_PROTOCOL *DevicePath;

  Status = gBS->HandleProtocol (Volume, &gEfiDevicePathProtocolGuid, (VOID**) &DevicePath);

  if (EFI_ERROR (Status))
    return Status;

  Print (L"\n");
  Print (L"Volume size: %d\n", FileSystemInfo->VolumeSize);
  Print (L"Block size: %d\n", FileSystemInfo->BlockSize);

  CHAR16 *DevicePathText = ConvertDevicePathToText (DevicePath, FALSE, FALSE);

  if (DevicePathText)
    {
      Print (L"Device path: %s\n", DevicePathText);
      KernelInfo->DevicePath = DevicePathText;
    }

  KernelInfo->Volume = Volume;
  KernelInfo->Partition = Partition;
  KernelInfo->RootDir = RootDir;

  LookLinux (KernelInfo);

  return EFI_SUCCESS;
}


/*
  * Входная точка загрузчика - UefiMain
*/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_HANDLE *Volumes;
  UINTN VolumesCount;

  EFI_STATUS Status = QueryPartitions (&Volumes, &VolumesCount);

  if (EFI_ERROR (Status))
    {
      Print (L"Failed to query volumes: %r\n", Status);
      return EFI_SUCCESS;
    }

  Print (L"Found %d volumes!\n", VolumesCount);

  OS_KERNEL *Kernels = AllocateMemoryPool (sizeof (OS_KERNEL) * VolumesCount);

  if (EFI_ERROR (Status))
    {
      Print (L"Failed to allocate memory: %r\n", Status);
      return EFI_SUCCESS;
    }

  for (UINTN Index = 0; Index < VolumesCount; ++Index)
    {
      EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileProtocol;

      Status = gBS->HandleProtocol (Volumes[Index], 
                          &gEfiSimpleFileSystemProtocolGuid, (VOID**) &FileProtocol);

      if (EFI_ERROR (Status))
        continue;

      Status = OpenPartition (Volumes[Index], FileProtocol, &Kernels[Index]);

      if (EFI_ERROR (Status))
        {
          Print (L"OpenPartition(): %r\n", Status);
        }
    }

  return EFI_SUCCESS;
}
