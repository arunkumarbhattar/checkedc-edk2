/** @file
  This a DXE Services Wrapper to allow applications to access
  DXE Services.

**/
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Guid/DxeServices.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/DebugLib.h>
#include "DxeServiceWrapper.h"


EFI_HANDLE NewHandle = NULL;
EFI_WRAPPER_PROTOCOL mWrapperProtocol = {
  CallDxeServiceProtocol
};

// This is simple a wrapper protocol function that will
// take in the inputs for ProcessFirmwareVolume on gDS
// and then call it and return the output.

EFI_STATUS
EFIAPI
CallDxeServiceProtocol (
  IN VOID *Image,
  IN UINTN ImageSize,
  OUT EFI_HANDLE *Handle
)
{
  EFI_STATUS Status;
  Status = gDS->ProcessFirmwareVolume(Image, ImageSize, Handle);

  return Status;
}

/**
  The user Entry Point for DXE Driver. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
WrapperEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS Status;

  Status = gBS->InstallMultipleProtocolInterfaces(
                &NewHandle, 
                &gEfiWrapperProtocolGuid, 
                &mWrapperProtocol, 
                NULL
                );
  ASSERT_EFI_ERROR(Status);
  return EFI_SUCCESS;
}
