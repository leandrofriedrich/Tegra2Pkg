/** @file
  Tegra 2 hardware register definitions

  Copyright (c) 2026, Leandro Friedrich.
  SPDX-License-Identifier: GPL-2.0-or-later
**/

#ifndef _TEGRA2_HW_H_
#define _TEGRA2_HW_H_

#define TEGRA2_GIC_CPU_BASE        0x50040100
#define TEGRA2_GIC_DIST_BASE       0x50041000
#define TEGRA2_TWD_TIMER_BASE      0x50040600
#define TEGRA2_L2CC_BASE           0x50043000

#define TEGRA2_TMR_BASE            0x60005000
#define TEGRA2_TMRUS_CNTR          0x60005010

#define TEGRA2_TMR1_PTV            (TEGRA2_TMR_BASE + 0x0)
#define TEGRA2_TMR1_PCR            (TEGRA2_TMR_BASE + 0x4)
#define TEGRA2_TMR1_PTV_EN         (1U << 31)
#define TEGRA2_TMR1_PTV_PER        (1U << 30)
#define TEGRA2_TMR1_PCR_INTR_CLR   (1U << 30)
#define TEGRA2_TMR1_PTV_MAX_COUNT  0x20000000
#define TEGRA2_TMR1_IRQ            32
#define TEGRA2_CAR_BASE            0x60006000
#define TEGRA2_GPIO_BASE           0x6000D000

#define TEGRA2_PINMUX_TRISTATE     0x70000014
#define TEGRA2_PINMUX_MUX          0x70000080
#define TEGRA2_I2C3_BASE           0x7000C500
#define TEGRA2_PMC_BASE            0x7000E400
#define TEGRA2_MC_BASE             0x7000F000
#define TEGRA2_EMC_BASE            0x7000F400

#define TEGRA2_UARTA_BASE           0x70006000

#define TEGRA2_USB0_BASE            0xC5000000
#define TEGRA2_USB1_BASE            0xC5004000
#define TEGRA2_USB2_BASE            0xC5008000
#define TEGRA2_SDMMC_SD_BASE        0xC8000000
#define TEGRA2_SDMMC_EMMC_BASE      0xC8000600

#define TEGRA2_DC1_BASE             0x54200000
#define TEGRA2_DC_CMD_STATE_CONTROL             (TEGRA2_DC1_BASE + 0x041 * 4)
#define TEGRA2_DC_CMD_DISPLAY_WINDOW_HEADER     (TEGRA2_DC1_BASE + 0x042 * 4)

/* Window A registers (window register bank starts at index 0x700). */
#define TEGRA2_DC_WIN_A_OPTIONS                 (TEGRA2_DC1_BASE + 0x700 * 4)
#define TEGRA2_DC_WIN_A_BYTE_SWAP               (TEGRA2_DC1_BASE + 0x701 * 4)
#define TEGRA2_DC_WIN_A_COLOR_DEPTH             (TEGRA2_DC1_BASE + 0x703 * 4)
#define TEGRA2_DC_WIN_A_POSITION                (TEGRA2_DC1_BASE + 0x704 * 4)
#define TEGRA2_DC_WIN_A_SIZE                    (TEGRA2_DC1_BASE + 0x705 * 4)
#define TEGRA2_DC_WIN_A_PRESCALED_SIZE           (TEGRA2_DC1_BASE + 0x706 * 4)
#define TEGRA2_DC_WIN_A_H_INITIAL_DDA             (TEGRA2_DC1_BASE + 0x707 * 4)
#define TEGRA2_DC_WIN_A_V_INITIAL_DDA             (TEGRA2_DC1_BASE + 0x708 * 4)
#define TEGRA2_DC_WIN_A_DDA_INC                   (TEGRA2_DC1_BASE + 0x709 * 4)
#define TEGRA2_DC_WIN_A_LINE_STRIDE               (TEGRA2_DC1_BASE + 0x70A * 4)

/* Tegra20 DC window fields. */
#define TEGRA2_DC_WIN_POSITION_H(x)              (((x) & 0x1FFFU) << 0)
#define TEGRA2_DC_WIN_POSITION_V(y)              (((y) & 0x1FFFU) << 16)
#define TEGRA2_DC_WIN_SIZE_H(w)                  (((w) & 0x1FFFU) << 0)
#define TEGRA2_DC_WIN_SIZE_V(h)                  (((h) & 0x1FFFU) << 16)
#define TEGRA2_DC_WIN_PRESCALED_SIZE_H(bytes)     (((bytes) & 0x7FFFU) << 0)
#define TEGRA2_DC_WIN_PRESCALED_SIZE_V(lines)     (((lines) & 0x1FFFU) << 16)
#define TEGRA2_DC_WIN_DDA_INC_H(x)               (((x) & 0xFFFFU) << 0)
#define TEGRA2_DC_WIN_DDA_INC_V(y)               (((y) & 0xFFFFU) << 16)

#define TEGRA2_DC_WIN_A_ENABLE                   (1U << 30)
#define TEGRA2_DC_WIN_A_H_DIRECTION_INCREMENT    (0U << 0)
#define TEGRA2_DC_WIN_A_V_DIRECTION_INCREMENT    (0U << 2)

#define TEGRA2_DC_WIN_A_UPDATE                   (1U << 9)
#define TEGRA2_DC_WIN_A_ACT_REQ                  (1U << 1)
#define TEGRA2_DC_WINDOW_A_SELECT                (1U << 4)

#endif
