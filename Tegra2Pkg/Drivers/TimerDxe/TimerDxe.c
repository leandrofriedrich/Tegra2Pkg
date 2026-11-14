/** @file
  Tegra20 timer DXE driver.

  Copyright (c) 2026, Leandro Friedrich.
  SPDX-License-Identifier: GPL-2.0-or-later
**/

#include <PiDxe.h>

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/HardwareInterrupt.h>
#include <Protocol/Timer.h>

#include <Tegra2/Tegra2Hw.h>

#define TEGRA2_TIMER_DEFAULT_PERIOD  100000

STATIC EFI_TIMER_NOTIFY                  mTimerNotifyFunction = NULL;
STATIC UINT64                            mTimerPeriod        = 0;
STATIC EFI_HARDWARE_INTERRUPT_PROTOCOL  *mInterrupt          = NULL;
STATIC EFI_EVENT                         mExitBootServicesEvent;

STATIC
EFI_STATUS
EFIAPI
TimerDriverRegisterHandler (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  IN EFI_TIMER_NOTIFY         NotifyFunction
  )
{
  if ((NotifyFunction == NULL) && (mTimerNotifyFunction == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((NotifyFunction != NULL) && (mTimerNotifyFunction != NULL)) {
    return EFI_ALREADY_STARTED;
  }

  mTimerNotifyFunction = NotifyFunction;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
TimerDriverSetTimerPeriod (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  IN UINT64                   TimerPeriod
  )
{
  UINT32      Ticks;
  EFI_STATUS  Status;

  if (TimerPeriod == 0) {
    MmioWrite32 (TEGRA2_TMR1_PTV, 0);
    mTimerPeriod = 0;
    return mInterrupt->DisableInterruptSource (mInterrupt, TEGRA2_TMR1_IRQ);
  }

  Ticks = (UINT32)(TimerPeriod / 10);
  if (Ticks == 0) {
    Ticks = 1;
  }
  if (Ticks > TEGRA2_TMR1_PTV_MAX_COUNT) {
    Ticks = TEGRA2_TMR1_PTV_MAX_COUNT;
  }

  mTimerPeriod = TimerPeriod;

  MmioWrite32 (TEGRA2_TMR1_PTV, 0);
  MmioWrite32 (TEGRA2_TMR1_PCR, TEGRA2_TMR1_PCR_INTR_CLR);
  MmioWrite32 (TEGRA2_TMR1_PTV, TEGRA2_TMR1_PTV_EN | TEGRA2_TMR1_PTV_PER | (Ticks - 1));

  Status = mInterrupt->EnableInterruptSource (mInterrupt, TEGRA2_TMR1_IRQ);
  return Status;
}

STATIC
EFI_STATUS
EFIAPI
TimerDriverGetTimerPeriod (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  OUT UINT64                  *TimerPeriod
  )
{
  *TimerPeriod = mTimerPeriod;
  return EFI_SUCCESS;
}

STATIC
VOID
EFIAPI
TimerInterruptHandler (
  IN  HARDWARE_INTERRUPT_SOURCE  Source,
  IN  EFI_SYSTEM_CONTEXT         SystemContext
  )
{
  EFI_TPL  OriginalTPL;

  OriginalTPL = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  MmioWrite32 (TEGRA2_TMR1_PCR, TEGRA2_TMR1_PCR_INTR_CLR);
  mInterrupt->EndOfInterrupt (mInterrupt, Source);

  if (mTimerNotifyFunction) {
    mTimerNotifyFunction (mTimerPeriod);
  }

  gBS->RestoreTPL (OriginalTPL);
}

STATIC
EFI_STATUS
EFIAPI
TimerDriverGenerateSoftInterrupt (
  IN EFI_TIMER_ARCH_PROTOCOL  *This
  )
{
  return EFI_UNSUPPORTED;
}

STATIC EFI_TIMER_ARCH_PROTOCOL  gTimer = {
  TimerDriverRegisterHandler,
  TimerDriverSetTimerPeriod,
  TimerDriverGetTimerPeriod,
  TimerDriverGenerateSoftInterrupt
};

STATIC
VOID
EFIAPI
ExitBootServicesEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  Status = TimerDriverSetTimerPeriod (&gTimer, 0);
  ASSERT_EFI_ERROR (Status);
}

EFI_STATUS
EFIAPI
TimerInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_HANDLE  Handle = NULL;
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (&gHardwareInterruptProtocolGuid, NULL, (VOID **)&mInterrupt);
  ASSERT_EFI_ERROR (Status);

  Status = TimerDriverSetTimerPeriod (&gTimer, 0);
  ASSERT_EFI_ERROR (Status);

  Status = mInterrupt->RegisterInterruptSource (mInterrupt, TEGRA2_TMR1_IRQ, TimerInterruptHandler);
  ASSERT_EFI_ERROR (Status);

  Status = TimerDriverSetTimerPeriod (&gTimer, TEGRA2_TIMER_DEFAULT_PERIOD);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiTimerArchProtocolGuid,
                  &gTimer,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_NOTIFY,
                  ExitBootServicesEvent,
                  NULL,
                  &mExitBootServicesEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
