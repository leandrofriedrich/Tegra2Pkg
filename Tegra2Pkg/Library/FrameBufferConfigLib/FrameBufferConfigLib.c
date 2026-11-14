#include <Uefi.h>
#include <Library/IoLib.h>
#include <Tegra2/Tegra2Hw.h>

EFI_STATUS
EFIAPI
FrameBufferConfigLibConfigurePixelFormat (
  VOID
  )
{

  // Select Display Controller Window A
  MmioWrite32 (
    TEGRA2_DC_CMD_DISPLAY_WINDOW_HEADER,
    TEGRA2_DC_WINDOW_A_SELECT
    );

  // Set Window A to 32-bit B8G8R8A8 format and disable byte swapping
  MmioWrite32 (TEGRA2_DC_WIN_A_COLOR_DEPTH, 12U);
  MmioWrite32 (TEGRA2_DC_WIN_A_BYTE_SWAP, 0U);

  // Set Window A Position
  MmioWrite32 (
    TEGRA2_DC_WIN_A_POSITION,
    TEGRA2_DC_WIN_POSITION_H (0U) |
    TEGRA2_DC_WIN_POSITION_V (0U)
    );

  // (Re)Set destination window dimensions (otherwise we lose half of the panel)
  MmioWrite32 (
    TEGRA2_DC_WIN_A_SIZE,
    TEGRA2_DC_WIN_SIZE_H (1024U) |
    TEGRA2_DC_WIN_SIZE_V (600U)
    );

  // Set Prescaled source dimensions
  MmioWrite32 (
    TEGRA2_DC_WIN_A_PRESCALED_SIZE,
    TEGRA2_DC_WIN_PRESCALED_SIZE_H (1024U * 4U) |
    TEGRA2_DC_WIN_PRESCALED_SIZE_V (600U)
    );

  // Do not scale
  MmioWrite32 (TEGRA2_DC_WIN_A_H_INITIAL_DDA, 0U);
  MmioWrite32 (TEGRA2_DC_WIN_A_V_INITIAL_DDA, 0U);
  MmioWrite32 (
    TEGRA2_DC_WIN_A_DDA_INC,
    TEGRA2_DC_WIN_DDA_INC_H (0x1000U) |
    TEGRA2_DC_WIN_DDA_INC_V (0x1000U)
    );

  // Set line stride
  MmioWrite32 (TEGRA2_DC_WIN_A_LINE_STRIDE, 1024U * 4U);

  // Request Window A register update and activate the updated state
  MmioWrite32 (TEGRA2_DC_CMD_STATE_CONTROL, TEGRA2_DC_WIN_A_UPDATE);
  MmioWrite32 (TEGRA2_DC_CMD_STATE_CONTROL, TEGRA2_DC_WIN_A_ACT_REQ);

  return EFI_SUCCESS;
}
