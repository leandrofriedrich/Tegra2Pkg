/** @file
  Secondary System Description Table

  Copyright (c) 2026, Leandro Friedrich.
  SPDX-License-Identifier: GPL-2.0-or-later
**/

DefinitionBlock ("DSDT.aml", "DSDT", 2, "COMPAL", "PAZ00   ", 1)
{
  Scope (\_SB)
  {
    Device (URTA)
    {
      Name (_UID, 0)
      Name (_STA, 0x0F)

      Name (_CRS, ResourceTemplate ()
      {
        Memory32Fixed (ReadWrite, 0x70006000, 0x00000040)
        Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive)
        {
          0x44
        }
      })
    }

    Device (NVEC)
    {
      Name (_HID, "NVEC0001")
      Name (_UID, 0)
      Name (_STA, 0x0F)
      Name (_STR, Unicode("Tegra2 NVEC Keyboard and Touchpad"))

      Name (_CRS, ResourceTemplate ()
      {
        Memory32Fixed (ReadWrite, 0x7000C500, 0x00000100)
        Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive)
        {
          0x7C
        }
      })
    }
  }
}
