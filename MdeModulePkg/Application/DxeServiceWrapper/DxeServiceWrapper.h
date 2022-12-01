#ifndef __EFI_WRAPPER_PROTOCOL_H__
#define __EFI_WRAPPER_PROTOCOL_H__

#define EFI_WRAPPER_PROTOCOL_GUID \
    {0x71d10478, 0x324e, 0x4cb3, {0x90, 0xcc, 0x6b, 0xbb, 0x7f, 0x03, 0x8d, 0xcf}}

typedef struct _EFI_WRAPPER_PROTOCOL EFI_WRAPPER_PROTOCOL;


typedef
EFI_STATUS
(EFIAPI *EFI_WRAPPER_PROTOCOL_PROCESS_FIRMWARE)(
  IN VOID *Image,
  IN UINTN ImageSize,
  OUT EFI_HANDLE *Handle 
  );


struct _EFI_WRAPPER_PROTOCOL {
    EFI_WRAPPER_PROTOCOL_PROCESS_FIRMWARE CallDxeServiceProtocol;
};

extern EFI_GUID gEfiWrapperProtocolGuid;

#endif