/** @file
  NVIDIA NVEC transport driver

  Copyright (c) 2026, Leandro Friedrich.
  SPDX-License-Identifier: GPL-2.0-or-later
**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/I2cSlave.h>
#include <Protocol/Nvec.h>

#include <Tegra2/Tegra2Hw.h>

#define TEGRA2_GPIO_BANK_STRIDE  0x80
#define TEGRA2_GPIO_CONFIG       0x00
#define TEGRA2_GPIO_DIR_OUT      0x10
#define TEGRA2_GPIO_OUT          0x20

#define TEGRA2_GPIO_BANK(Gpio)  ((Gpio) >> 5)
#define TEGRA2_GPIO_PORT(Gpio)  (((Gpio) >> 3) & 0x03)
#define TEGRA2_GPIO_BIT(Gpio)   (1U << ((Gpio) & 0x07))

#define TEGRA2_GPIO_REG(Gpio, Reg) \
  (TEGRA2_GPIO_BASE + TEGRA2_GPIO_BANK (Gpio) * TEGRA2_GPIO_BANK_STRIDE + \
   (Reg) + TEGRA2_GPIO_PORT (Gpio) * sizeof (UINT32))

// PV2 is the request line to the EC.
#define NVEC_REQUEST_GPIO  170
#define NVEC_REQUEST_CFG   TEGRA2_GPIO_REG (NVEC_REQUEST_GPIO, TEGRA2_GPIO_CONFIG)
#define NVEC_REQUEST_OE    TEGRA2_GPIO_REG (NVEC_REQUEST_GPIO, TEGRA2_GPIO_DIR_OUT)
#define NVEC_REQUEST_OUT   TEGRA2_GPIO_REG (NVEC_REQUEST_GPIO, TEGRA2_GPIO_OUT)
#define NVEC_REQUEST_BIT   TEGRA2_GPIO_BIT (NVEC_REQUEST_GPIO)

// SL_STATUS reports the 8-bit address including the R/W bit
#define NVEC_ADDRESS  0x8A

#define NVEC_TIMEOUT_MIN      20
#define NVEC_TIMEOUT_MAX      600
#define NVEC_ATTEMPTS_MAX     10
#define NVEC_READ_EVENTS_MAX  8

typedef enum {
  NvecStateBegin = 0,
  NvecStateCmd,
  NvecStateSubCmd,
  NvecStateRead,
  NvecStateWrite
} NVEC_STATE;

typedef enum {
  NvecIoError = -1,
  NvecIoTimeout,
  NvecIoReadOk,
  NvecIoWriteOk,
  NvecIoNotReady,
  NvecIoRetry
} NVEC_IO_RESULT;

typedef struct {
  NVEC_PROTOCOL        Protocol;
  I2C_SLAVE_PROTOCOL   *I2c;
  NVEC_EVENT           EventHandler;
  VOID                        *EventHandlerContext;

  NVEC_STATE           State;
  UINT8                       RxBuffer[NVEC_MSG_SIZE];
  UINTN                       RxPos;
  CONST UINT8                 *TxBuffer;
  UINTN                       TxPos;
  UINTN                       TxSize;
  BOOLEAN                     ResponseDone;
} NVEC_DEV;

#define NVEC_FROM_THIS(a)  BASE_CR (a, NVEC_DEV, Protocol)

STATIC CONST UINT8  mNvecNoop[] = { 0x02, 0x07, 0x02 };

STATIC
VOID
NvecSetRequest (
  IN BOOLEAN  Idle
  )
{
  UINT32  Value;

  Value = MmioRead32 (NVEC_REQUEST_OUT);
  if (Idle) {
    Value |= NVEC_REQUEST_BIT;
  } else {
    Value &= ~NVEC_REQUEST_BIT;
  }

  MmioWrite32 (NVEC_REQUEST_OUT, Value);
}

STATIC
VOID
NvecInitRequestGpio (
  VOID
  )
{
  MmioOr32 (NVEC_REQUEST_CFG, NVEC_REQUEST_BIT);
  MmioOr32 (NVEC_REQUEST_OUT, NVEC_REQUEST_BIT);
  MmioOr32 (NVEC_REQUEST_OE, NVEC_REQUEST_BIT);
}

STATIC
VOID
NvecProcessMessage (
  IN NVEC_DEV  *Dev
  )
{
  UINTN  Expected;

  if (NVEC_MSG_IS_EVENT (Dev->RxBuffer)) {
    if (Dev->EventHandler != NULL) {
      Dev->EventHandler (Dev->RxBuffer, Dev->EventHandlerContext);
    }

    return;
  }

  // The EC sometimes sends a short reply first, wait for the full one.
  Expected = Dev->RxBuffer[1] + 2;
  if (Dev->RxPos < Expected) {
    return;
  }

  Dev->ResponseDone = TRUE;
}

STATIC
NVEC_IO_RESULT
NvecDoIo (
  IN NVEC_DEV  *Dev,
  IN BOOLEAN          WaitForEc
  )
{
  UINTN   Timeout;
  UINT32  Status;
  UINT32  Received;
  UINT32  ToSend;
  BOOLEAN First;

  Timeout  = 0;
  Received = 0;
  ToSend   = 0;
  First    = TRUE;

  if (WaitForEc) {
    gBS->Stall (NVEC_TIMEOUT_MIN * 1000);
  }

  while (TRUE) {
    Status = Dev->I2c->Status (Dev->I2c);
    if ((Status & I2C_SL_IRQ) == 0) {
      if (First && !WaitForEc) {
        return NvecIoNotReady;
      }

      if (Timeout > NVEC_TIMEOUT_MAX * 1000) {
        // Abandoned transfer: realign so the next address byte starts clean.
        Dev->State = NvecStateBegin;
        Dev->RxPos = 0;
        Dev->TxPos = 0;
        return NvecIoTimeout;
      }

      First = FALSE;
      gBS->Stall (100);
      Timeout += 100;
      continue;
    }

    First = FALSE;

    if ((Status & I2C_SL_RNW) == 0) {
      Received = Dev->I2c->ReadData (Dev->I2c);
    }

    // The EC starts a new transfer, restart the state machine.
    if (Status == (I2C_SL_IRQ | I2C_SL_RCVD)) {
      Dev->State = NvecStateBegin;
      Dev->RxPos  = 0;
      Dev->TxPos  = 0;
    }

    switch (Dev->State) {
      case NvecStateBegin:
        Dev->RxPos = 0;
        Dev->TxPos = 0;
        if (Received != NVEC_ADDRESS) {
          DEBUG ((DEBUG_WARN, "Nvec: unexpected address 0x%02x\n", Received));
          return NvecIoError;
        }

        Dev->State = NvecStateCmd;
        break;

      case NvecStateCmd:
        if (Dev->RxPos < NVEC_MSG_SIZE) {
          Dev->RxBuffer[Dev->RxPos++] = (UINT8)Received;
        }

        Dev->State = NvecStateSubCmd;
        break;

      case NvecStateSubCmd:
        if (Status == (I2C_SL_IRQ | I2C_SL_RNW | I2C_SL_RCVD)) {
          if (Dev->RxBuffer[0] != 0x01) {
            DEBUG ((DEBUG_WARN, "Nvec: read without read command\n"));
            Dev->State = NvecStateBegin;
            return NvecIoError;
          }

          if (Dev->TxBuffer == NULL) {
            Dev->TxBuffer = mNvecNoop;
            Dev->TxSize   = sizeof (mNvecNoop);
            Dev->TxPos    = 0;
          }

          // send length and drop request line
          Dev->State = NvecStateWrite;
          ToSend     = (UINT32)Dev->TxSize;
          Dev->I2c->WriteData (Dev->I2c, ToSend);
          NvecSetRequest (TRUE);
        } else {
          Dev->State = NvecStateRead;
          if (Dev->RxPos < NVEC_MSG_SIZE) {
            Dev->RxBuffer[Dev->RxPos++] = (UINT8)Received;
          }

          // The transfer can end on the scan byte, don't drop it.
          if ((Status & I2C_SL_END_TRANS) != 0) {
            NvecProcessMessage (Dev);
            Dev->RxPos = 0;
            return NvecIoReadOk;
          }
        }

        break;

      case NvecStateRead:
        if (Dev->RxPos >= NVEC_MSG_SIZE) {
          DEBUG ((DEBUG_WARN, "Nvec: receive buffer is full\n"));
          break;
        }

        Dev->RxBuffer[Dev->RxPos++] = (UINT8)Received;
        if ((Status & I2C_SL_END_TRANS) != 0) {
          NvecProcessMessage (Dev);
          Dev->RxPos = 0;
          return NvecIoReadOk;
        }

        break;

      case NvecStateWrite:
        if (Dev->TxPos >= Dev->TxSize) {
          if ((Status & I2C_SL_END_TRANS) != 0) {
            return NvecIoWriteOk;
          }

          DEBUG ((DEBUG_WARN, "Nvec: no data to write\n"));
          return NvecIoError;
        }

        ToSend = Dev->TxBuffer[Dev->TxPos++];
        Dev->I2c->WriteData (Dev->I2c, ToSend);
        if ((Status & I2C_SL_END_TRANS) != 0) {
          Dev->TxPos    = 0;
          Dev->TxBuffer = NULL;
          return NvecIoWriteOk;
        }

        break;

      default:
        DEBUG ((DEBUG_WARN, "Nvec: unknown state %d\n", Dev->State));
        Dev->State = NvecStateBegin;
        return NvecIoError;
    }

    if ((Status & I2C_SL_END_TRANS) != 0) {
      return NvecIoRetry;
    }
  }
}

STATIC
EFI_STATUS
EFIAPI
NvecRequest (
  IN NVEC_PROTOCOL  *This,
  IN CONST UINT8           *Buffer,
  IN UINTN                 Size
  )
{
  NVEC_DEV       *Dev;
  NVEC_IO_RESULT Result;
  UINTN                 Attempt;
  UINTN                 Index;

  Dev = NVEC_FROM_THIS (This);

  Dev->TxBuffer = Buffer;
  Dev->TxSize   = Size;

  for (Attempt = 0; Attempt < NVEC_ATTEMPTS_MAX; Attempt++) {
    Dev->TxPos        = 0;
    Dev->ResponseDone = FALSE;

    // Pull the request line low so the EC knows we have a command.
    NvecSetRequest (FALSE);
    Result = NvecDoIo (Dev, TRUE);
    if (Result != NvecIoWriteOk) {
      DEBUG ((DEBUG_WARN, "Nvec: failed to send request (%d)\n", Result));
      NvecSetRequest (TRUE);
      continue;
    }

    for (Index = 0; Index < NVEC_READ_EVENTS_MAX; Index++) {
      Result = NvecDoIo (Dev, TRUE);
      if (Result != NvecIoReadOk) {
        break;
      }

      if (Dev->ResponseDone) {
        break;
      }
    }

    NvecSetRequest (TRUE);

    if (Dev->ResponseDone) {
      Dev->TxBuffer = NULL;
      Dev->TxSize   = 0;
      Dev->TxPos    = 0;
      return EFI_SUCCESS;
    }
  }

  DEBUG ((DEBUG_WARN, "Nvec: request failed (last io result %d)\n", Result));
  Dev->TxBuffer = NULL;
  Dev->TxSize   = 0;
  Dev->TxPos    = 0;

  return EFI_DEVICE_ERROR;
}

STATIC
VOID
EFIAPI
NvecPoll (
  IN NVEC_PROTOCOL  *This
  )
{
  NVEC_DEV  *Dev;

  Dev = NVEC_FROM_THIS (This);

  NvecDoIo (Dev, FALSE);
}

STATIC
VOID
EFIAPI
NvecSetEventHandler (
  IN NVEC_PROTOCOL  *This,
  IN NVEC_EVENT     EventHandler,
  IN VOID                  *Context
  )
{
  NVEC_DEV  *Dev;

  Dev                     = NVEC_FROM_THIS (This);
  Dev->EventHandler       = EventHandler;
  Dev->EventHandlerContext = Context;
}

EFI_STATUS
EFIAPI
NvecInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS       Status;
  NVEC_DEV  *Dev;

  Dev = AllocateZeroPool (sizeof (*Dev));
  if (Dev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->LocateProtocol (&gI2cSlaveProtocolGuid, NULL, (VOID **)&Dev->I2c);
  if (EFI_ERROR (Status)) {
    FreePool (Dev);
    return Status;
  }

  Dev->Protocol.Request          = NvecRequest;
  Dev->Protocol.Poll             = NvecPoll;
  Dev->Protocol.SetEventHandler  = NvecSetEventHandler;

  NvecInitRequestGpio ();
  NvecSetRequest (TRUE);
  gBS->Stall (100 * 1000);

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gNvecProtocolGuid,
                  &Dev->Protocol,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Dev);
  }

  return Status;
}
