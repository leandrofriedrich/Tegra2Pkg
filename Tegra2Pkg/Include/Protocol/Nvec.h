/** @file
  NVIDIA NVEC protocol

  Copyright (c) 2026, Leandro Friedrich.
  SPDX-License-Identifier: GPL-2.0-or-later
**/

#ifndef __TEGRA2_NVEC_H__
#define __TEGRA2_NVEC_H__

#include <Uefi.h>

#define NVEC_PROTOCOL_GUID \
  { 0x69187069, 0x7252, 0x4508, { 0x9a, 0xfc, 0x51, 0x94, 0x5b, 0xcd, 0x9a, 0x64 } }

#define NVEC_MSG_SIZE  34

//
// Event types (low nibble of the header).
//
#define NVEC_EVENT_KEYBOARD  0

//
// Packet header helpers.
//
#define NVEC_MSG_IS_EVENT(Msg)    (((Msg)[0] & BIT7) != 0)
#define NVEC_MSG_EVENT_TYPE(Msg)  ((Msg)[0] & 0x0F)
#define NVEC_MSG_SIZE_CODE(Msg)   (((Msg)[0] >> 5) & 0x03)

typedef struct _NVEC_PROTOCOL NVEC_PROTOCOL;

/**
  Handle an event packet.

  @param[in] Message  Event packet. Only valid for the duration of the call.
  @param[in] Context  Handler context passed to SetEventHandler.
**/
typedef VOID (EFIAPI *NVEC_EVENT) (
  IN CONST UINT8  *Message,
  IN VOID         *Context
  );

/**
  Send a request to the EC and wait for its reply.

  @param[in] This    Protocol instance.
  @param[in] Buffer  Request packet.
  @param[in] Size    Request size in bytes.

  @retval EFI_SUCCESS       The EC answered.
  @retval EFI_DEVICE_ERROR  No valid reply was received.
**/
typedef EFI_STATUS (EFIAPI *NVEC_REQUEST) (
  IN NVEC_PROTOCOL  *This,
  IN CONST UINT8           *Buffer,
  IN UINTN                 Size
  );

/**
  Collect queued events. Event packets are delivered through the handler
  installed with SetEventHandler.
**/
typedef VOID (EFIAPI *NVEC_POLL) (
  IN NVEC_PROTOCOL  *This
  );

/**
  Install the event handler. There is a single handler.
**/
typedef VOID (EFIAPI *NVEC_SET_EVENT_HANDLER) (
  IN NVEC_PROTOCOL  *This,
  IN NVEC_EVENT     EventHandler,
  IN VOID                  *Context
  );

struct _NVEC_PROTOCOL {
  NVEC_REQUEST            Request;
  NVEC_POLL               Poll;
  NVEC_SET_EVENT_HANDLER  SetEventHandler;
};

extern EFI_GUID  gNvecProtocolGuid;

#endif
