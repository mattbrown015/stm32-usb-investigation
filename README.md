# STM32 USB Investigation
I'm interested in how to create a USB device based on a STM32 MCU.

The [STM32f723IE](https://www.st.com/content/st_com/en/products/microcontrollers-microprocessors/stm32-32-bit-arm-cortex-mcus/stm32-high-performance-mcus/stm32f7-series/stm32f7x3/stm32f723ie.html) has an integrated HS PHY and hence looks like a good choice for the basis of a USB HS device where size and power are not the primary concerns.

The [Discovery kit with STM32F723IE MCU](https://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-mpu-eval-tools/stm32-mcu-mpu-eval-tools/stm32-discovery-kits/32f723ediscovery.html) is cheap and easy to use.

It seems likely that a USB device will be sophisticated enough to justify an operating system.  I've used [Mbed OS](https://os.mbed.com/), it's free, open-source and includes official support for STM devices (and I've used it before).

## Mbed OS

Mbed OS 6.1 includes support for many STM development boards including the [NUCLEO-F767ZI](https://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-mpu-eval-tools/stm32-mcu-mpu-eval-tools/stm32-nucleo-boards/nucleo-f767zi.html).  The USB examples, see [USBCDC](https://os.mbed.com/docs/mbed-os/v6.2/apis/usbcdc.html) for an example, *just* build and work on the [NUCLEO-F767ZI](https://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-mpu-eval-tools/stm32-mcu-mpu-eval-tools/stm32-nucleo-boards/nucleo-f767zi.html).  It's unfortunate that the [NUCLEO-F767ZI](https://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-mpu-eval-tools/stm32-mcu-mpu-eval-tools/stm32-nucleo-boards/nucleo-f767zi.html) does not include a HS PHY.

The [32F723EDISCOVERY](https://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-mpu-eval-tools/stm32-mcu-mpu-eval-tools/stm32-discovery-kits/32f723ediscovery.html) is not target supported by Mbed OS 6.1, which is a shame.

Creating an Mbed OS custom target is straightforward, see (Adding and configuring targets)[https://os.mbed.com/docs/mbed-os/v6.2/program-setup/adding-and-configuring-targets.html].  It didn't take long to add a custom target for the [32F723EDISCOVERY](https://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-mpu-eval-tools/stm32-mcu-mpu-eval-tools/stm32-discovery-kits/32f723ediscovery.html).  The USB examples *just* worked on [32F723EDISCOVERY](https://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-mpu-eval-tools/stm32-mcu-mpu-eval-tools/stm32-discovery-kits/32f723ediscovery.html) USB FS interface.

(I think) As a consequence of [32F723EDISCOVERY](https://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-mpu-eval-tools/stm32-mcu-mpu-eval-tools/stm32-discovery-kits/32f723ediscovery.html) Mbed OS 6.1 does not support USB HS for STM targets.  [USBPhy_STM32.cpp](https://github.com/ARMmbed/mbed-os/blob/master/targets/TARGET_STM/USBPhy_STM32.cpp) includes some code that implies it might support USB HS, see [USBPhy_STM32.cpp#L197](https://github.com/ARMmbed/mbed-os/blob/master/targets/TARGET_STM/USBPhy_STM32.cpp#L197) for example, but, AFAICT, it is incomplete.

## STM32CubeF7 HAL

MBed OS does not enforce the use of the Mbed OS abstractions of target peripherals.  In other words, it is possible to use the STM32CubeF7 USB HAL rather than the [USB APIs](https://os.mbed.com/docs/mbed-os/v6.2/apis/usb-apis.html).  Using the HAL means the project is not portable to other targets but this is not a problem for this investigation.

Mbed OS does not include the *middleware components* that are include in the STM distribution of [STM32CubeF7](https://www.st.com/content/st_com/en/products/embedded-software/mcu-mpu-embedded-software/stm32-embedded-software/stm32cube-mcu-mpu-packages/stm32cubef7.html).  I decided it would make a better learning opportunity to use the USB HAL and not attempt to combine the STM32CubeF7 USB *middleware component*.  I did use the STM32CubeF7 USB *middleware component* as a reference.
