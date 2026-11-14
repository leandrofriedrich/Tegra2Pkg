/** @file
  TimerLib for Tegra20, using the 1 MHz microsecond counter (TIMERUS).

  The counter is 32 bits wide and wraps every ~71 minutes.

  Copyright (c) 2026, Leandro Friedrich.
  SPDX-License-Identifier: GPL-2.0-or-later
**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>

#include <Tegra2/Tegra2Hw.h>

#define TEGRA2_TIMER_HZ  1000000

UINT64
EFIAPI
GetPerformanceCounter (
  VOID
  )
{
  return MmioRead32(TEGRA2_TMRUS_CNTR);
}

UINT64
EFIAPI
GetPerformanceCounterProperties (
  OUT UINT64  *StartValue,  OPTIONAL
  OUT UINT64  *EndValue     OPTIONAL
  )
{
  if (StartValue != NULL) {
    *StartValue = 0;
  }
  if (EndValue != NULL) {
    *EndValue = 0xFFFFFFFFULL;
  }
  return TEGRA2_TIMER_HZ;
}

UINT64
EFIAPI
GetTimeInNanoSecond (
  IN  UINT64  Ticks
  )
{
  return MultU64x32(Ticks, 1000000000 / TEGRA2_TIMER_HZ);
}

UINTN
EFIAPI
MicroSecondDelay (
  IN  UINTN  MicroSeconds
  )
{
  UINT32  Start;

  Start = MmioRead32(TEGRA2_TMRUS_CNTR);
  while ((UINT32)(MmioRead32(TEGRA2_TMRUS_CNTR) - Start) < (UINT32)MicroSeconds) {
  }
  return MicroSeconds;
}

UINTN
EFIAPI
NanoSecondDelay (
  IN  UINTN  NanoSeconds
  )
{
  MicroSecondDelay((NanoSeconds + 999) / 1000);
  return NanoSeconds;
}
