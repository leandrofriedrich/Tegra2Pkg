<!-- SPDX-License-Identifier: GPL-2.0-or-later -->

# EDK2 UEFI Firmware For Tegra 2 based platforms
simple EDK2 Port for Tegra 2 platforms, currently only the Toshiba AC100.

heavily based on HtcHd2Pkg, which is based on Hikey960Pkg and many more, idk man just check the credit list

## Loading
Currently expected to be loaded from U-Boot, load to $loadaddr and go to it. 

## OS support
  - none yet

## Support status 

### SoC 
| Function      	| status |
|-----------------------|-------|
| Timer			|   ✅	 |
| Interrupt Controller  |   ✅	 |
| Serial		|   ✅   |

### Device
| Function      | Notes                            | status |
|---------------|----------------------------------|--------|
| Screen	| its just simplefb lol		   |   ✅   |

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
