#ifndef _FRAME_BUFFER_CONFIG_LIB_H_
#define _FRAME_BUFFER_CONFIG_LIB_H_

#include <Uefi.h>

typedef struct {
  UINT32  BitsPerPixel;
  UINT32  Width;
  UINT32  Height;
} FRAME_BUFFER_CONFIGURATION;

EFI_STATUS
EFIAPI
FrameBufferConfigLibConfigurePixelFormat (
  VOID
  );

#endif
