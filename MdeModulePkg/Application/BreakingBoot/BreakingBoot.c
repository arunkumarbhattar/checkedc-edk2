/** @file
  This sample application bases on BreakingBoot PCD setting
  to run the Breaking Boot fuzzer to the UEFI Console.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include "../Universal/DisplayEngineDxe/FormDisplay.h"
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <PiDxe.h>
#include <Guid/DxeServices.h>
#include <Protocol/VariableWrite.h>
#include <Protocol/FaultTolerantWrite.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/Variable.h>
#include <Protocol/VariableLock.h>
#include <Protocol/VarCheck.h>
#include <Library/HobLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/AuthVariableLib.h>
#include <Library/VarCheckLib.h>
#include <Library/VariableFlashInfoLib.h>
#include <Library/SafeIntLib.h>
#include <Guid/GlobalVariable.h>
#include <Guid/EventGroup.h>
#include <Guid/VariableFormat.h>
#include <Guid/SystemNvDataGuid.h>
#include <Guid/FaultTolerantWrite.h>
#include <Guid/VarErrorFlag.h>
#define CHAR_DASH 0x002D
#define CHAR_F 0x0046
#define CHAR_f 0x0066
#define CHAR_N 0x004E
#define CHAR_n 0x006E
#define CHAR_S 0x0053
#define CHAR_s 0x0073

//
// String token ID of help message text.
// Shell supports to find help message in the resource section of an application image if
// .MAN file is not found. This global variable is added to make build tool recognizes
// that the help string is consumed by user and then build tool will add the string into
// the resource section. Thus the application can use '-?' option to show help message in
// Shell.
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_STRING_ID  mStringHelpTokenId = STRING_TOKEN (STR_BREAKING_BOOT_HELP_INFORMATION);

/**
  Wait for a key to be pressed by user.

  @param Key         The key which is pressed by user.

  @retval EFI_SUCCESS The function always completed successfully.

**/
EFI_STATUS
WaitForKeyStroke (
  OUT  EFI_INPUT_KEY  *Key
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  while (TRUE) {
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, Key);
    if (!EFI_ERROR (Status)) {
      break;
    }

    if (Status != EFI_NOT_READY) {
      continue;
    }

    gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &Index);
  }

  return Status;
}

/**
  Count the storage space of a Unicode string.

  This function handles the Unicode string with NARROW_CHAR
  and WIDE_CHAR control characters. NARROW_HCAR and WIDE_CHAR
  does not count in the resultant output. If a WIDE_CHAR is
  hit, then 2 Unicode character will consume an output storage
  space with size of CHAR16 till a NARROW_CHAR is hit.

  If String is NULL, then ASSERT ().

  @param String          The input string to be counted.

  @return Storage space for the input string.

**/

UINTN
GetStringWidth (
  IN CHAR16  *String
  )
{
  UINTN  Index;
  UINTN  Count;
  UINTN  IncrementValue;

  ASSERT (String != NULL);
  if (String == NULL) {
    return 0;
  }

  Index          = 0;
  Count          = 0;
  IncrementValue = 1;

  do {
    //
    // Advance to the null-terminator or to the first width directive
    //
    for ( ;
          (String[Index] != NARROW_CHAR) && (String[Index] != WIDE_CHAR) && (String[Index] != 0);
          Index++, Count = Count + IncrementValue
          )
    {
    }

    //
    // We hit the null-terminator, we now have a count
    //
    if (String[Index] == 0) {
      break;
    }

    //
    // We encountered a narrow directive - strip it from the size calculation since it doesn't get printed
    // and also set the flag that determines what we increment by.(if narrow, increment by 1, if wide increment by 2)
    //
    if (String[Index] == NARROW_CHAR) {
      //
      // Skip to the next character
      //
      Index++;
      IncrementValue = 1;
    } else {
      //
      // Skip to the next character
      //
      Index++;
      IncrementValue = 2;
    }
  } while (String[Index] != 0);

  //
  // Increment by one to include the null-terminator in the size
  //
  Count++;

  return Count * sizeof (CHAR16);
}


EFI_STATUS
EFIAPI
HelpMenu (
  IN UINTN Option
)
{
  switch(Option)
  {
    case 0:
      Print(L"This shell application has two operations: \n \
        1. Enter 'F' or 'f' then <Enter> to run the fuzzer. \n \
        2. Enter 'N' or 'n' then <Enter> to run normal operations.\n \
        3. Enter 'S' or 's' then <Enter> to see the support drivers. \n \
        4. Hit <ESC> to exit the application.\n");
      break;
    case 1:
      Print(L"Support Drivers: \n \
        -GetVariable \n");
  }
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
CustomReadString (
  OUT CHAR16 *StringPtr
)
{
  EFI_STATUS Status;
  EFI_INPUT_KEY Key;
  CHAR16 *TempString;
  UINTN Index;
  CHAR16 KeyPad[2];
  UINTN Count = 0;
  UINTN Maximum = 11;
  UINTN MaxLen = Maximum + 1;
  TempString = AllocateZeroPool(MaxLen * sizeof(CHAR16));
  ASSERT(TempString);

  do {
    Status = WaitForKeyStroke (&Key);
    ASSERT_EFI_ERROR (Status);

    switch (Key.UnicodeChar) {
      case CHAR_NULL:
        switch (Key.ScanCode) {
          /*
          case SCAN_LEFT:
            if (CurrentCursor > 0) {
              CurrentCursor--;
            }

            break;

          case SCAN_RIGHT:
            if (CurrentCursor < (GetStringWidth (StringPtr) / 2 - 1)) {
              CurrentCursor++;
            }

            break;
          */
          case SCAN_ESC:
            FreePool (TempString);
            return EFI_DEVICE_ERROR;
          default:
            break;
        }

        break;

      case CHAR_CARRIAGE_RETURN:
        FreePool (TempString);
        return EFI_SUCCESS;


      case CHAR_BACKSPACE:
        if ((StringPtr[0] != CHAR_NULL)) {
          for (Index = 0; Index < Count; Index++) {
            TempString[Index] = StringPtr[Index];
          }

          TempString[Index] = CHAR_NULL;

          //
          // Effectively truncate string by 1 character
          //
          StrCpyS (StringPtr, MaxLen, TempString);
          Print(L"%c", Key.UnicodeChar);
          Count--;
        }

      default:
        //
        // If it is the beginning of the string, don't worry about checking maximum limits
        //
        
        if ((StringPtr[0] == CHAR_NULL) && (Key.UnicodeChar != CHAR_BACKSPACE)) {
          StrnCpyS (StringPtr, MaxLen, &Key.UnicodeChar, 1);
        } else if ((GetStringWidth (StringPtr) < ((Maximum + 1) * sizeof (CHAR16))) && (Key.UnicodeChar != CHAR_BACKSPACE)) {
          KeyPad[0] = Key.UnicodeChar;
          KeyPad[1] = CHAR_NULL;
          /*
          Count     = GetStringWidth (StringPtr) / 2 - 1;
          if (CurrentCursor < Count) {
            for (Index = 0; Index < CurrentCursor; Index++) {
              TempString[Index] = StringPtr[Index];
            }

            TempString[Index] = CHAR_NULL;
            StrCatS (TempString, MaxLen, KeyPad);
            StrCpyS (StringPtr, MaxLen, TempString);
          } else {
            */
            StrCatS (StringPtr, MaxLen, KeyPad);
        }
        Count++;
        Print(L"%c", Key.UnicodeChar);
        break;
    }
  } while (TRUE);

}

/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINTN EventIndex;
  EFI_STATUS Status;
  EFI_INPUT_KEY Key;
  EFI_PEI_HOB_POINTERS FvHob;
  EFI_HANDLE FvProtocolHandle;
  EFI_FIRMWARE_VOLUME_HEADER *FwHeader;

  //NVRAM Variables are stored in MdePkg/Include/Guid/GlobalVariable.h

  // Enable PCD in order to run the code
  if (FeaturePcdGet (PcdBreakingBootPrintEnable)) {
    Print ((CHAR16 *)PcdGetPtr (PcdBreakingBootPrintString));

    HelpMenu(0);

    do {
      gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex);
      gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
      Print(L"%c\n",Key.UnicodeChar);
      switch (Key.UnicodeChar)
      {
        case CHAR_F:
        case CHAR_f:
          FvHob.Raw = GetHobList();
          FvHob.Raw = GetNextHob(EFI_HOB_TYPE_FV, FvHob.Raw);
          FwHeader = (EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) ((EFI_PHYSICAL_ADDRESS)(UINTN)FvHob.FirmwareVolume->BaseAddress);
          Status = gDS->ProcessFirmwareVolume((VOID *) FwHeader, (MAX_UINT32), &FvProtocolHandle);
          Print(L"ProcessFirmwareVolume Status - %r\n", Status);
          DEBUG((DEBUG_INFO, "ProcessFirmwareVolume - %r\n", Status));

          break;

        case CHAR_S:
        case CHAR_s:
          Print(L"Entered Support\n");
          HelpMenu(0);
          break;

        case CHAR_N:
        case CHAR_n:
          Print(L"Entered Normal\n");
          FvHob.Raw = GetHobList();
          FvHob.Raw = GetNextHob(EFI_HOB_TYPE_FV, FvHob.Raw);
          FwHeader = (EFI_FIRMWARE_VOLUME_HEADER *) (UINTN) ((EFI_PHYSICAL_ADDRESS)(UINTN)FvHob.FirmwareVolume->BaseAddress);
          Status = gDS->ProcessFirmwareVolume((VOID *) FwHeader, (FwHeader->FvLength), &FvProtocolHandle);
          DEBUG((DEBUG_INFO, "ProcessFirmwareVolume - %r\n", Status));
          Print(L"ProcessFirmwareVolume Status - %r\n", Status);
          break;
      }
      /*
      if(Key.UnicodeChar == CHAR_CARRIAGE_RETURN)
      {
        Print(L"\n");
        if(FLAG_F)
        {
          //Run Fuzzing operations on driver
          Print(L"Running Breaking Boot Fuzzer on <APP>\n");
          CHAR16 *DriverStr = AllocateZeroPool(MaxLen * sizeof(CHAR16));
          CustomReadString(DriverStr);
          Print(L"\nRunning a fuzzing operation on %s\n", DriverStr);
          Status   = gRT->GetVariable (
                            L"OsIndications",
                            &gEfiGlobalVariableGuid,
                            NULL,
                            &DataSize,
                            &OsIndication
                            );
          Print(L"GetVariable is checking to see the Capsule Update Status.\n");
          Print(L"Results: GetVariable(OsIndications) = %d\n", OsIndication);
          if(EFI_ERROR(Status))
          {
            Print(L"**ERROR MSG**: Failure Running GetVariable.\n");
          }
          if((OsIndication & EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED) != 0)
          {
            Print(L"LOG MSG: Capsule Status = completeted.\n");
            OsIndication &= ~((UINT64)EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED);
          }
          else
          {
            Print(L"LOG MSG: Capsule Status = not completeted.\n");
            OsIndication |= ((UINT64)EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED);
          }
          Print(L"LOG MSG: Try setting the variable to the opposite value.\n");
 
          Status = gRT->SetVariable (
                          L"OsIndications",
                          &gEfiGlobalVariableGuid,
                          EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                          sizeof (UINT64),
                          &OsIndication
                          );
          if(EFI_ERROR(Status))
          {
            Print(L"**ERROR MSG**: Failure Running SetVariable(OsIndications).\n");
          }
          Status = gRT->GetVariable (
                        L"OsIndications",
                        &gEfiGlobalVariableGuid,
                        NULL,
                        &DataSize,
                        &OsIndication
                        );
          if(EFI_ERROR(Status))
          {
            Print(L"**ERROR MSG**: Failure Running GetVariable(OsIndications).\n");
          }
          Status   = gRT->GetVariable (
                            L"OsIndicationsSupported",
                            &gEfiGlobalVariableGuid,
                            NULL,
                            &DataSize,
                            &OsIndicationsSupported
                            );
          Print(L"\tUpdated Reselt: GetVariable(OsIndication) = %d\n", OsIndication);
          if(EFI_ERROR(Status))
          {
            Print(L"**ERROR MSG**: Failure Running GetVariable(OsIndicationsSupported).\n");
          }
          if((OsIndicationsSupported & EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED) != 0)
          {
            Print(L"LOG MSG: Capsule Update = Supported.\n");
          }
          else
          {
            Print(L"LOG MSG: Capsule Update = not Supported.\n");
          }
          if((OsIndication & EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED) != 0)
          {
            Print(L"LOG MSG: Capsule Status = completeted.\n");
          }
          else
          {
            Print(L"LOG MSG: Capsule Status = not complete.\n");
          }
          FreePool(DriverStr);
          FLAG_F = FALSE;
        }
        else if(FLAG_N)
        {
          //Run normal driver operations
          Print(L"Running a Normal Operations on <APP>\n");
          CHAR16 *DriverStr = AllocateZeroPool(MaxLen * sizeof(CHAR16));
          CustomReadString(DriverStr);
          Print(L"\nRunning a normal operation on %s\n", DriverStr);
          //Status = gRT->GetVariable (L"Boot0001", &ExampleGUID, &Attributes, &DataSize, &Data);
  
          Status   = gRT->GetVariable (
                            L"OsIndicationsSupported",
                            &gEfiGlobalVariableGuid,
                            NULL,
                            &DataSize,
                            &OsIndicationsSupported
                            );
          Print(L"GetVariable is to check if Capsule Update is supported on the disk.\n");
          Print(L"Results: GetVariable(OsIndicationsSupport) = %d\n", OsIndicationsSupported);
          if(EFI_ERROR(Status))
          {
            Print(L"**ERROR MSG:** Failure Running GetVariable.\n");
          }
          if((OsIndicationsSupported & EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED) != 0)
          {
            Print(L"LOG MSG: Capsule is supported on the Disk.\n");
          }
          else
          {
            Print(L"LOG MSG: Capsule is not supported on the Disk.\n");
          }
          FreePool(DriverStr);
          FLAG_N = FALSE;
        }
        else if(FLAG_S)
        {
          HelpMenu(1);
          FLAG_S = FALSE;
        }
        else
        {
          Print(L"***Invalid Operation***\n");
          HelpMenu(0);
        }
      }
      else if((Key.UnicodeChar == CHAR_F) || (Key.UnicodeChar == CHAR_f))
      {
        //Set the Fuzzer flag
        FLAG_F = TRUE;
      }
      else if((Key.UnicodeChar == CHAR_N) || (Key.UnicodeChar == CHAR_n))
      {
        //Set tje normal driver operation flag
        FLAG_N = TRUE;
      }
      else if((Key.UnicodeChar == CHAR_S) || (Key.UnicodeChar == CHAR_s))
      {
        FLAG_S = TRUE;
      }
      else
      {
        FLAG_N = FALSE;
        FLAG_F = FALSE;
        FLAG_S = FALSE;
      }
      */
    } while(!(Key.ScanCode == SCAN_ESC));

  }
  else
  {
    Print(L"Breaking Boot is currently Disabled\n");
  }

  return EFI_SUCCESS;
}
