/** @file
  NVEC keyboard driver

  Copyright (c) 2026, Leandro Friedrich.
  SPDX-License-Identifier: GPL-2.0-or-later
**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/DevicePath.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/Nvec.h>

#include <Tegra2/NvecKbd.h>

#define NVEC_KBD_QUEUE_SIZE  32
#define NVEC_KBD_POLL_MS     20
#define NVEC_KBD_HOLD_MS     500
#define NVEC_KBD_KEY_COUNT   256

typedef struct {
  UINT16  ScanCode;
  CHAR16  Unicode;
  CHAR16  ShiftUnicode;
} NVEC_KEY;

typedef struct {
  BOOLEAN  Pressed;
  BOOLEAN  Pending;
  UINT32   PressMs;
} NVEC_KBD_KEY;

typedef struct {
  LIST_ENTRY                  NotifyEntry;
  EFI_KEY_DATA                KeyData;
  EFI_KEY_NOTIFY_FUNCTION     KeyNotificationFn;
} NVEC_KBD_KEY_NOTIFY;

typedef struct {
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL     ConIn;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  ConInEx;
  NVEC_PROTOCOL                      *Nvec;
  EFI_EVENT                          Timer;
  EFI_EVENT                          WaitForKey;
  EFI_EVENT                          KeyNotifyEvent;
  EFI_INPUT_KEY                      Queue[NVEC_KBD_QUEUE_SIZE];
  UINTN                              QueueHead;
  UINTN                              QueueTail;
  EFI_INPUT_KEY                      NotifyQueue[NVEC_KBD_QUEUE_SIZE];
  UINTN                              NotifyHead;
  UINTN                              NotifyTail;
  LIST_ENTRY                         NotifyList;
  EFI_KEY_TOGGLE_STATE               ToggleState;
  NVEC_KBD_KEY                       Keys[NVEC_KBD_KEY_COUNT];
  UINTN                              RepeatKey;
  UINT32                             Now;
  BOOLEAN                            RepeatPending;
  BOOLEAN                            LeftCtrl;
  BOOLEAN                            RightCtrl;
  BOOLEAN                            LeftShift;
  BOOLEAN                            RightShift;
  BOOLEAN                            CapsLock;
} NVEC_KBD;

#define NVEC_KBD_FROM_THIS(a)     BASE_CR (a, NVEC_KBD, ConIn)
#define NVEC_KBD_FROM_THIS_EX(a)  BASE_CR (a, NVEC_KBD, ConInEx)

STATIC CONST NVEC_KBD_DEVICE_PATH  mNvecKbdDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    NVEC_KBD_DEVICE_PATH_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8)(sizeof (EFI_DEVICE_PATH_PROTOCOL)),
      0
    }
  }
};

// Scan code set 1 without the E0 prefix.
STATIC CONST NVEC_KEY  mKeyMap[0x80] = {
  [0x00] = { SCAN_NULL, L'`',  L'~'  },
  [0x01] = { SCAN_ESC,  0,     0     },
  [0x02] = { SCAN_NULL, L'1',  L'!'  },
  [0x03] = { SCAN_NULL, L'2',  L'@'  },
  [0x04] = { SCAN_NULL, L'3',  L'#'  },
  [0x05] = { SCAN_NULL, L'4',  L'$'  },
  [0x06] = { SCAN_NULL, L'5',  L'%'  },
  [0x07] = { SCAN_NULL, L'6',  L'^'  },
  [0x08] = { SCAN_NULL, L'7',  L'&'  },
  [0x09] = { SCAN_NULL, L'8',  L'*'  },
  [0x0A] = { SCAN_NULL, L'9',  L'('  },
  [0x0B] = { SCAN_NULL, L'0',  L')'  },
  [0x0C] = { SCAN_NULL, L'-',  L'_'  },
  [0x0D] = { SCAN_NULL, L'=',  L'+'  },
  [0x0E] = { SCAN_NULL, 0x0008, 0x0008 },
  [0x0F] = { SCAN_NULL, 0x0009, 0x0009 },
  [0x10] = { SCAN_NULL, L'q',  L'Q'  },
  [0x11] = { SCAN_NULL, L'w',  L'W'  },
  [0x12] = { SCAN_NULL, L'e',  L'E'  },
  [0x13] = { SCAN_NULL, L'r',  L'R'  },
  [0x14] = { SCAN_NULL, L't',  L'T'  },
  [0x15] = { SCAN_NULL, L'y',  L'Y'  },
  [0x16] = { SCAN_NULL, L'u',  L'U'  },
  [0x17] = { SCAN_NULL, L'i',  L'I'  },
  [0x18] = { SCAN_NULL, L'o',  L'O'  },
  [0x19] = { SCAN_NULL, L'p',  L'P'  },
  [0x1A] = { SCAN_NULL, L'[',  L'{'  },
  [0x1B] = { SCAN_NULL, L']',  L'}'  },
  [0x1C] = { SCAN_NULL, 0x000D, 0x000D },
  [0x1E] = { SCAN_NULL, L'a',  L'A'  },
  [0x1F] = { SCAN_NULL, L's',  L'S'  },
  [0x20] = { SCAN_NULL, L'd',  L'D'  },
  [0x21] = { SCAN_NULL, L'f',  L'F'  },
  [0x22] = { SCAN_NULL, L'g',  L'G'  },
  [0x23] = { SCAN_NULL, L'h',  L'H'  },
  [0x24] = { SCAN_NULL, L'j',  L'J'  },
  [0x25] = { SCAN_NULL, L'k',  L'K'  },
  [0x26] = { SCAN_NULL, L'l',  L'L'  },
  [0x27] = { SCAN_NULL, L';',  L':'  },
  [0x28] = { SCAN_NULL, L'\'', L'"'  },
  [0x29] = { SCAN_NULL, L'`',  L'~'  },
  [0x2B] = { SCAN_NULL, L'\\', L'|'  },
  [0x2C] = { SCAN_NULL, L'z',  L'Z'  },
  [0x2D] = { SCAN_NULL, L'x',  L'X'  },
  [0x2E] = { SCAN_NULL, L'c',  L'C'  },
  [0x2F] = { SCAN_NULL, L'v',  L'V'  },
  [0x30] = { SCAN_NULL, L'b',  L'B'  },
  [0x31] = { SCAN_NULL, L'n',  L'N'  },
  [0x32] = { SCAN_NULL, L'm',  L'M'  },
  [0x33] = { SCAN_NULL, L',',  L'<'  },
  [0x34] = { SCAN_NULL, L'.',  L'>'  },
  [0x35] = { SCAN_NULL, L'/',  L'?'  },
  [0x37] = { SCAN_NULL, L'*',  L'*'  },
  [0x39] = { SCAN_NULL, L' ',  L' '  },
  [0x3B] = { SCAN_F1,   0,     0     },
  [0x3C] = { SCAN_F2,   0,     0     },
  [0x3D] = { SCAN_F3,   0,     0     },
  [0x3E] = { SCAN_F4,   0,     0     },
  [0x3F] = { SCAN_F5,   0,     0     },
  [0x40] = { SCAN_F6,   0,     0     },
  [0x41] = { SCAN_F7,   0,     0     },
  [0x42] = { SCAN_F8,   0,     0     },
  [0x43] = { SCAN_F9,   0,     0     },
  [0x44] = { SCAN_F10,  0,     0     },
  [0x47] = { SCAN_NULL, L'7',  L'7'  },
  [0x48] = { SCAN_NULL, L'8',  L'8'  },
  [0x49] = { SCAN_NULL, L'9',  L'9'  },
  [0x4A] = { SCAN_NULL, L'-',  L'-'  },
  [0x4B] = { SCAN_NULL, L'4',  L'4'  },
  [0x4C] = { SCAN_NULL, L'5',  L'5'  },
  [0x4D] = { SCAN_NULL, L'6',  L'6'  },
  [0x4E] = { SCAN_NULL, L'+',  L'+'  },
  [0x4F] = { SCAN_NULL, L'1',  L'1'  },
  [0x50] = { SCAN_NULL, L'2',  L'2'  },
  [0x51] = { SCAN_NULL, L'3',  L'3'  },
  [0x52] = { SCAN_NULL, L'0',  L'0'  },
  [0x53] = { SCAN_NULL, L'.',  L'.'  },
  [0x56] = { SCAN_NULL, L'\\', L'|'  },
  [0x57] = { SCAN_F11,  0,     0     },
  [0x58] = { SCAN_F12,  0,     0     },
  [0x73] = { SCAN_NULL, L'5',  L'5'  },
  [0x7D] = { SCAN_NULL, L'9',  L'9'  }
};

// Scan code set 1 with the E0 prefix.
STATIC CONST NVEC_KEY  mExtKeyMap[0x80] = {
  [0x35] = { SCAN_NULL,      L'/',  L'/'  },
  [0x41] = { SCAN_LEFT,      0,     0     },
  [0x47] = { SCAN_HOME,      0,     0     },
  [0x48] = { SCAN_UP,        0,     0     },
  [0x49] = { SCAN_PAGE_UP,   0,     0     },
  [0x4B] = { SCAN_LEFT,      0,     0     },
  [0x4D] = { SCAN_RIGHT,     0,     0     },
  [0x4F] = { SCAN_END,       0,     0     },
  [0x50] = { SCAN_DOWN,      0,     0     },
  [0x51] = { SCAN_PAGE_DOWN, 0,     0     },
  [0x52] = { SCAN_INSERT,    0,     0     },
  [0x53] = { SCAN_DELETE,    0,     0     },
  [0x5D] = { SCAN_ESC,       0,     0     },
  [0x5E] = { SCAN_NULL,      L'-',  L'-'  }
};

STATIC
BOOLEAN
NvecKbdIsKeyRegistered (
  IN EFI_KEY_DATA  *RegisteredData,
  IN EFI_KEY_DATA  *InputData
  )
{
  if ((RegisteredData->Key.ScanCode != InputData->Key.ScanCode) ||
      (RegisteredData->Key.UnicodeChar != InputData->Key.UnicodeChar)) {
    return FALSE;
  }

  return TRUE;
}

STATIC
VOID
NvecKbdNotifyKey (
  IN NVEC_KBD            *Kbd,
  IN CONST EFI_INPUT_KEY *Key
  )
{
  LIST_ENTRY             *Link;
  NVEC_KBD_KEY_NOTIFY    *Notify;
  EFI_KEY_DATA           KeyData;
  UINTN                  Next;

  KeyData.Key = *Key;
  KeyData.KeyState.KeyShiftState = 0;
  KeyData.KeyState.KeyToggleState = 0;

  for (Link = GetFirstNode (&Kbd->NotifyList);
       !IsNull (&Kbd->NotifyList, Link);
       Link = GetNextNode (&Kbd->NotifyList, Link)) {
    Notify = BASE_CR (Link, NVEC_KBD_KEY_NOTIFY, NotifyEntry);
    if (NvecKbdIsKeyRegistered (&Notify->KeyData, &KeyData)) {
      Next = (Kbd->NotifyTail + 1) % NVEC_KBD_QUEUE_SIZE;
      if (Next == Kbd->NotifyHead) {
        Kbd->NotifyHead = (Kbd->NotifyHead + 1) % NVEC_KBD_QUEUE_SIZE;
      }
      Kbd->NotifyQueue[Kbd->NotifyTail] = *Key;
      Kbd->NotifyTail = Next;
      gBS->SignalEvent (Kbd->KeyNotifyEvent);
      break;
    }
  }
}

STATIC
VOID
NvecKbdPush (
  IN NVEC_KBD             *Kbd,
  IN CONST EFI_INPUT_KEY  *Key
  )
{
  UINTN  Next;

  Next = (Kbd->QueueTail + 1) % NVEC_KBD_QUEUE_SIZE;
  if (Next == Kbd->QueueHead) {
    Kbd->QueueHead = (Kbd->QueueHead + 1) % NVEC_KBD_QUEUE_SIZE;
  }

  Kbd->Queue[Kbd->QueueTail] = *Key;
  Kbd->QueueTail             = Next;

  NvecKbdNotifyKey (Kbd, Key);
}

STATIC
BOOLEAN
NvecKbdIsModifier (
  IN UINTN  Index
  )
{
  UINT8    Code;
  BOOLEAN  Extended;

  Code     = (UINT8)(Index & 0x7F);
  Extended = (BOOLEAN)((Index & 0x80) != 0);

  if (Extended) {
    return (BOOLEAN)((Code == 0x1D) || (Code == 0x38) || (Code == 0x5B) || (Code == 0x5C));
  }

  return (BOOLEAN)((Code == 0x1D) || (Code == 0x2A) || (Code == 0x36) || (Code == 0x38) || (Code == 0x45));
}

STATIC
VOID
NvecKbdUpdateModifier (
  IN OUT NVEC_KBD  *Kbd,
  IN UINT8                Code,
  IN BOOLEAN              Extended,
  IN BOOLEAN              Down
  )
{
  if (Extended) {
    switch (Code) {
      case 0x1D:
        Kbd->RightCtrl = Down;
        break;
      default:
        break;
    }

    return;
  }

  switch (Code) {
    case 0x1D:
      Kbd->LeftCtrl = Down;
      break;
    case 0x2A:
      Kbd->LeftShift = Down;
      break;
    case 0x36:
      Kbd->RightShift = Down;
      break;
    default:
      break;
  }
}

STATIC
BOOLEAN
NvecKbdTranslate (
  IN NVEC_KBD  *Kbd,
  IN UINT8            Code,
  IN BOOLEAN          Extended,
  OUT EFI_INPUT_KEY   *Key
  )
{
  CONST NVEC_KEY  *Map;
  CHAR16                 Unicode;
  BOOLEAN                Shift;
  BOOLEAN                Ctrl;

  ZeroMem (Key, sizeof (*Key));

  if (!Extended) {
    switch (Code) {
      case 0x48:
        Key->ScanCode = SCAN_UP;
        return TRUE;
      case 0x4B:
        Key->ScanCode = SCAN_LEFT;
        return TRUE;
      case 0x4D:
        Key->ScanCode = SCAN_RIGHT;
        return TRUE;
      case 0x50:
        Key->ScanCode = SCAN_DOWN;
        return TRUE;
      default:
        break;
    }

    Map = &mKeyMap[Code];
  } else {
    Map = &mExtKeyMap[Code];
  }

  if ((Map->ScanCode == SCAN_NULL) && (Map->Unicode == 0)) {
    return FALSE;
  }

  Shift = (BOOLEAN)(Kbd->LeftShift || Kbd->RightShift);
  Ctrl  = (BOOLEAN)(Kbd->LeftCtrl || Kbd->RightCtrl);

  Key->ScanCode = Map->ScanCode;
  Unicode       = Shift ? Map->ShiftUnicode : Map->Unicode;

  if (Kbd->CapsLock) {
    if ((Unicode >= L'a') && (Unicode <= L'z')) {
      Unicode = (CHAR16)(Unicode - L'a' + L'A');
    } else if ((Unicode >= L'A') && (Unicode <= L'Z')) {
      Unicode = (CHAR16)(Unicode - L'A' + L'a');
    }
  }

  if (Ctrl) {
    if ((Unicode >= L'a') && (Unicode <= L'z')) {
      Unicode = (CHAR16)(Unicode - L'a' + 1);
    } else if ((Unicode >= L'A') && (Unicode <= L'Z')) {
      Unicode = (CHAR16)(Unicode - L'A' + 1);
    }
  }

  Key->UnicodeChar = Unicode;

  return TRUE;
}

STATIC
VOID
NvecKbdEmitKey (
  IN OUT NVEC_KBD  *Kbd,
  IN UINTN                Index
  )
{
  EFI_INPUT_KEY  Key;

  if (NvecKbdIsModifier (Index)) {
    return;
  }

  if (!NvecKbdTranslate (Kbd, (UINT8)(Index & 0x7F), (BOOLEAN)((Index & 0x80) != 0), &Key)) {
    return;
  }

  NvecKbdPush (Kbd, &Key);
}

STATIC
VOID
NvecKbdSweep (
  IN OUT NVEC_KBD  *Kbd
  )
{
  NVEC_KBD_KEY  *Key;
  UINTN                Index;

  // Drop keys whose repeats stopped so they cannot stay stuck
  for (Index = 0; Index < NVEC_KBD_KEY_COUNT; Index++) {
    Key = &Kbd->Keys[Index];
    if (Key->Pressed && !NvecKbdIsModifier (Index) &&
        ((UINT32)(Kbd->Now - Key->PressMs) > NVEC_KBD_HOLD_MS))
    {
      DEBUG ((DEBUG_VERBOSE, "NvecKbd: lost release for 0x%02x\n", Index));
      Key->Pressed = FALSE;
    }
  }
}

STATIC
VOID
NvecKbdSend (
  IN OUT NVEC_KBD  *Kbd
  )
{
  NVEC_KBD_KEY  *Key;
  UINTN                Index;
  BOOLEAN              Repeat;

  // Repeat only when the EC reported one for a key that was already held.
  Repeat = (BOOLEAN)(Kbd->RepeatPending && !Kbd->Keys[Kbd->RepeatKey].Pending);
  Kbd->RepeatPending = FALSE;

  for (Index = 0; Index < NVEC_KBD_KEY_COUNT; Index++) {
    Key = &Kbd->Keys[Index];
    if (!Key->Pending) {
      continue;
    }

    Key->Pending = FALSE;
    NvecKbdEmitKey (Kbd, Index);
  }

  if (Repeat && Kbd->Keys[Kbd->RepeatKey].Pressed) {
    NvecKbdEmitKey (Kbd, Kbd->RepeatKey);
  }
}

STATIC
VOID
EFIAPI
NvecKbdEventHandler (
  IN CONST UINT8  *Message,
  IN VOID         *Context
  )
{
  NVEC_KBD      *Kbd;
  NVEC_KBD_KEY  *Key;
  UINTN                Size;
  UINTN                Offset;
  UINTN                Index;
  UINT8                Code;
  BOOLEAN              Extended;
  BOOLEAN              Release;

  Kbd = Context;

  if (!NVEC_MSG_IS_EVENT (Message)) {
    return;
  }

  if (NVEC_MSG_EVENT_TYPE (Message) != NVEC_EVENT_KEYBOARD) {
    return;
  }

  Size = NVEC_MSG_SIZE_CODE (Message);
  if (Size > 1) {
    return;
  }

  Offset   = (Size == 1) ? 2 : 1;
  Code     = (UINT8)(Message[Offset] & 0x7F);
  Extended = (BOOLEAN)(Size == 1);
  Release  = (BOOLEAN)((Message[Offset] & BIT7) != 0);
  Index    = (UINTN)Code | (Extended ? 0x80 : 0);
  Key      = &Kbd->Keys[Index];

  NvecKbdUpdateModifier (Kbd, Code, Extended, (BOOLEAN)!Release);

  if (Release) {
    Key->Pressed = FALSE;
    return;
  }

  if (!Key->Pressed) {
    Key->Pending = TRUE;
    if (!Extended && (Code == 0x3A)) {
      Kbd->CapsLock = (BOOLEAN)!Kbd->CapsLock;
    }
  } else {
    Kbd->RepeatPending = TRUE;
    Kbd->RepeatKey     = Index;
  }

  Key->Pressed = TRUE;
  Key->PressMs = Kbd->Now;
}

STATIC
VOID
EFIAPI
NvecKbdWaitForKey (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  NVEC_KBD  *Kbd;

  Kbd = Context;

  if (Kbd->QueueHead != Kbd->QueueTail) {
    gBS->SignalEvent (Event);
  }
}

STATIC
VOID
EFIAPI
NvecKbdTimer (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  NVEC_KBD  *Kbd;
  EFI_TPL          OldTpl;

  Kbd = Context;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  Kbd->Now += NVEC_KBD_POLL_MS;
  Kbd->Nvec->Poll (Kbd->Nvec);
  NvecKbdSweep (Kbd);
  NvecKbdSend (Kbd);
  if (Kbd->QueueHead != Kbd->QueueTail) {
    gBS->SignalEvent (Kbd->WaitForKey);
  }

  gBS->RestoreTPL (OldTpl);
}

STATIC
EFI_STATUS
EFIAPI
NvecKbdReset (
  IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  IN BOOLEAN                         ExtendedVerification
  )
{
  NVEC_KBD  *Kbd;
  EFI_TPL          OldTpl;

  Kbd = NVEC_KBD_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  ZeroMem (Kbd->Queue, sizeof (Kbd->Queue));
  Kbd->QueueHead = 0;
  Kbd->QueueTail = 0;
  ZeroMem (Kbd->Keys, sizeof (Kbd->Keys));
  Kbd->RepeatKey     = 0;
  Kbd->RepeatPending = FALSE;
  Kbd->LeftCtrl  = FALSE;
  Kbd->RightCtrl = FALSE;
  Kbd->LeftShift = FALSE;
  Kbd->RightShift = FALSE;
  Kbd->CapsLock  = FALSE;
  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
NvecKbdReadKeyStroke (
  IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  OUT EFI_INPUT_KEY                  *Key
  )
{
  NVEC_KBD  *Kbd;
  EFI_TPL          OldTpl;

  Kbd = NVEC_KBD_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  if (Kbd->QueueHead == Kbd->QueueTail) {
    gBS->RestoreTPL (OldTpl);
    return EFI_NOT_READY;
  }

  *Key           = Kbd->Queue[Kbd->QueueHead];
  Kbd->QueueHead = (Kbd->QueueHead + 1) % NVEC_KBD_QUEUE_SIZE;
  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

STATIC
VOID
EFIAPI
NvecKbdKeyNotifyProcess (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  NVEC_KBD            *Kbd;
  LIST_ENTRY          *Link;
  NVEC_KBD_KEY_NOTIFY *Notify;
  EFI_KEY_DATA        KeyData;
  EFI_INPUT_KEY       Key;
  EFI_TPL             OldTpl;

  Kbd = Context;

  while (TRUE) {
    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    if (Kbd->NotifyHead == Kbd->NotifyTail) {
      gBS->RestoreTPL (OldTpl);
      break;
    }
    Key = Kbd->NotifyQueue[Kbd->NotifyHead];
    Kbd->NotifyHead = (Kbd->NotifyHead + 1) % NVEC_KBD_QUEUE_SIZE;
    gBS->RestoreTPL (OldTpl);

    KeyData.Key = Key;
    KeyData.KeyState.KeyShiftState = 0;
    KeyData.KeyState.KeyToggleState = 0;

    for (Link = GetFirstNode (&Kbd->NotifyList);
         !IsNull (&Kbd->NotifyList, Link);
         Link = GetNextNode (&Kbd->NotifyList, Link)) {
      Notify = BASE_CR (Link, NVEC_KBD_KEY_NOTIFY, NotifyEntry);
      if (NvecKbdIsKeyRegistered (&Notify->KeyData, &KeyData)) {
        Notify->KeyNotificationFn (&KeyData);
      }
    }
  }
}

STATIC
EFI_STATUS
EFIAPI
NvecKbdResetEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  )
{
  return NvecKbdReset (&NVEC_KBD_FROM_THIS_EX (This)->ConIn, ExtendedVerification);
}

STATIC
EFI_STATUS
EFIAPI
NvecKbdReadKeyStrokeEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  OUT EFI_KEY_DATA                      *KeyData
  )
{
  NVEC_KBD  *Kbd;
  EFI_STATUS Status;

  Kbd = NVEC_KBD_FROM_THIS_EX (This);
  Status = NvecKbdReadKeyStroke (&Kbd->ConIn, &KeyData->Key);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  KeyData->KeyState.KeyShiftState = 0;
  KeyData->KeyState.KeyToggleState = Kbd->ToggleState;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
NvecKbdSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  )
{
  NVEC_KBD  *Kbd;

  Kbd = NVEC_KBD_FROM_THIS_EX (This);
  Kbd->ToggleState = *KeyToggleState;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
NvecKbdRegisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_DATA                       *KeyData,
  IN EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT VOID                              **NotifyHandle
  )
{
  NVEC_KBD            *Kbd;
  NVEC_KBD_KEY_NOTIFY *Notify;
  LIST_ENTRY          *Link;

  if ((KeyData == NULL) || (NotifyHandle == NULL) || (KeyNotificationFunction == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Kbd = NVEC_KBD_FROM_THIS_EX (This);

  for (Link = GetFirstNode (&Kbd->NotifyList);
       !IsNull (&Kbd->NotifyList, Link);
       Link = GetNextNode (&Kbd->NotifyList, Link)) {
    Notify = BASE_CR (Link, NVEC_KBD_KEY_NOTIFY, NotifyEntry);
    if (NvecKbdIsKeyRegistered (&Notify->KeyData, KeyData) &&
        Notify->KeyNotificationFn == KeyNotificationFunction) {
      *NotifyHandle = Notify;
      return EFI_SUCCESS;
    }
  }

  Notify = AllocateZeroPool (sizeof (*Notify));
  if (Notify == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Notify->KeyNotificationFn = KeyNotificationFunction;
  CopyMem (&Notify->KeyData, KeyData, sizeof (EFI_KEY_DATA));
  InsertTailList (&Kbd->NotifyList, &Notify->NotifyEntry);
  *NotifyHandle = Notify;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
NvecKbdUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN VOID                               *NotifyHandle
  )
{
  NVEC_KBD            *Kbd;
  NVEC_KBD_KEY_NOTIFY *Notify;
  LIST_ENTRY          *Link;

  if (NotifyHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Kbd = NVEC_KBD_FROM_THIS_EX (This);

  for (Link = GetFirstNode (&Kbd->NotifyList);
       !IsNull (&Kbd->NotifyList, Link);
       Link = GetNextNode (&Kbd->NotifyList, Link)) {
    Notify = BASE_CR (Link, NVEC_KBD_KEY_NOTIFY, NotifyEntry);
    if (Notify == NotifyHandle) {
      RemoveEntryList (&Notify->NotifyEntry);
      FreePool (Notify);
      return EFI_SUCCESS;
    }
  }

  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
EFIAPI
NvecKbdInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS       Status;
  NVEC_KBD  *Kbd;
  EFI_HANDLE       Handle;
  STATIC CONST UINT8  ResetKbd[]  = { 0x06, 0x01, 0xFF, 0x03 };
  STATIC CONST UINT8  ClearLeds[] = { 0x05, 0xED, 0x00 };
  STATIC CONST UINT8  EnableKbd[] = { 0x05, 0xF4 };

  Kbd = AllocateZeroPool (sizeof (*Kbd));
  if (Kbd == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->LocateProtocol (&gNvecProtocolGuid, NULL, (VOID **)&Kbd->Nvec);
  if (EFI_ERROR (Status)) {
    FreePool (Kbd);
    return Status;
  }

  Kbd->ConIn.Reset         = NvecKbdReset;
  Kbd->ConIn.ReadKeyStroke = NvecKbdReadKeyStroke;

  Kbd->ConInEx.Reset                = NvecKbdResetEx;
  Kbd->ConInEx.ReadKeyStrokeEx      = NvecKbdReadKeyStrokeEx;
  Kbd->ConInEx.SetState             = NvecKbdSetState;
  Kbd->ConInEx.RegisterKeyNotify    = NvecKbdRegisterKeyNotify;
  Kbd->ConInEx.UnregisterKeyNotify  = NvecKbdUnregisterKeyNotify;
  InitializeListHead (&Kbd->NotifyList);

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  NvecKbdWaitForKey,
                  Kbd,
                  &Kbd->WaitForKey
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Kbd);
    return Status;
  }
  Kbd->ConIn.WaitForKey = Kbd->WaitForKey;
  Kbd->ConInEx.WaitForKeyEx = NULL;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  NvecKbdKeyNotifyProcess,
                  Kbd,
                  &Kbd->KeyNotifyEvent
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (Kbd->WaitForKey);
    FreePool (Kbd);
    return Status;
  }

  Kbd->Nvec->SetEventHandler (Kbd->Nvec, NvecKbdEventHandler, Kbd);

  Status = Kbd->Nvec->Request (Kbd->Nvec, ResetKbd, sizeof (ResetKbd));
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "NvecKbd: reset command failed\n"));
  }

  Status = Kbd->Nvec->Request (Kbd->Nvec, ClearLeds, sizeof (ClearLeds));
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "NvecKbd: clear LEDs command failed\n"));
  }

  Status = Kbd->Nvec->Request (Kbd->Nvec, EnableKbd, sizeof (EnableKbd));
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "NvecKbd: enable command failed\n"));
  }

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  NvecKbdTimer,
                  Kbd,
                  &Kbd->Timer
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (Kbd->KeyNotifyEvent);
    gBS->CloseEvent (Kbd->WaitForKey);
    FreePool (Kbd);
    return Status;
  }

  Status = gBS->SetTimer (Kbd->Timer, TimerPeriodic, EFI_TIMER_PERIOD_MILLISECONDS (NVEC_KBD_POLL_MS));
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (Kbd->Timer);
    gBS->CloseEvent (Kbd->KeyNotifyEvent);
    gBS->CloseEvent (Kbd->WaitForKey);
    FreePool (Kbd);
    return Status;
  }

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiSimpleTextInProtocolGuid,
                  &Kbd->ConIn,
                  &gEfiSimpleTextInputExProtocolGuid,
                  &Kbd->ConInEx,
                  &gEfiDevicePathProtocolGuid,
                  &mNvecKbdDevicePath,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (Kbd->Timer);
    gBS->CloseEvent (Kbd->KeyNotifyEvent);
    gBS->CloseEvent (Kbd->WaitForKey);
    FreePool (Kbd);
  }

  return Status;
}
