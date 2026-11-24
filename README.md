<!-- SPDX-License-Identifier: GPL-2.0-or-later -->

# EDK2 UEFI Firmware For Tegra 2 based platforms
simple EDK2 Port for Tegra 2 platforms, currently only the Toshiba AC100.

heavily based on HtcHd2Pkg, which is based on Hikey960Pkg and many more, idk man just check the credit list

## Loading
Currently expected to be loaded from U-Boot, load to $loadaddr and go to it. 

## OS support
  - ReactOS: Boots to desktop!
  - Windows: eta one day™

## Support status 

### SoC 
| Function      	| status |
|-----------------------|-------|
| Timer			|   ✅	 |
| Interrupt Controller  |   ✅	 |
| Serial		|   ✅   |
| I2C			|   🟨   |
| MMC			|   ✅   |
| USB			|   ❌   |

Notes:
  - i2c is 🟨 because i only bothered with i2c slave support for nvec for now

### Device
| Function      | Notes                            | status |
|---------------|----------------------------------|--------|
| Screen	| its just simplefb lol		   |   ✅   |
| Keyboard	| Keyboard is done by (NV)EC	   |   🟨  |
| eMMC          | didnt care enough yet            |   ❌ |
| SD Card	| primary boot media		   |   ✅  |
| USB device	| god no(t yet)			   |   ❌  |

## Build notes

 - builds against edk2 stable/202508 
 - edk2-platforms at a5e5644d0e574da64ed908c7232d1cf5f6720c1f

## Credits
 - imbushuo for creating [PrimeG2Pkg](https://github.com/imbushuo/PrimeG2Pkg)
 - ivoszbg for [Msm8916Pkg](https://github.com/ivoszbg/edk2-msm8916/tree/master)
 - winocm for the [iPhone4Pkg](https://github.com/idaviden/kaikidan/tree/linaro-release)
 - sonic011gamer for [msm8909Pkg](https://github.com/sonic011gamer/msm8909Pkg)
 - HTC-Leo-Revival-Project for [HtcHd2Pkg](https://github.com/HTC-Leo-Revival-Project/HtcHd2Pkg)
 - libre-tegra team

## License

Original Tegra2Pkg code is licensed under GPL-2.0-or-later. See `LICENSE`.

Files derived from HtcHd2Pkg, EDK2, ArmPkg and other upstream projects retain
their original BSD licenses. See the SPDX headers in the source files,
`LICENSE.BSD-2-Clause`, `LICENSE.BSD-2-Clause-Patent` and `NOTICE`.
