/** @file
  Tegra20 hardware register definitions.

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
#define TEGRA2_PMC_BASE            0x7000E400
#define TEGRA2_MC_BASE             0x7000F000
#define TEGRA2_EMC_BASE            0x7000F400

#define TEGRA2_UARTA_BASE          0x70006000

#define TEGRA2_USB0_BASE           0xC5000000
#define TEGRA2_USB1_BASE           0xC5004000
#define TEGRA2_USB2_BASE           0xC5008000
#define TEGRA2_SDMMC_SD_BASE       0xC8000000
#define TEGRA2_SDMMC_EMMC_BASE     0xC8000600

#endif
