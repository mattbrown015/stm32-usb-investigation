# STM32 USB Investigation
I'm interested in how to create a USB device based on a STM32 MCU.

The [STM32f723IE](https://www.st.com/content/st_com/en/products/microcontrollers-microprocessors/stm32-32-bit-arm-cortex-mcus/stm32-high-performance-mcus/stm32f7-series/stm32f7x3/stm32f723ie.html) has an integrated HS PHY and hence looks like a good choice for the basis of a USB HS device where size and power are not the primary concerns.

The [Discovery kit with STM32F723IE MCU](https://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-mpu-eval-tools/stm32-mcu-mpu-eval-tools/stm32-discovery-kits/32f723ediscovery.html) is cheap and easy to use.

It seems likely that a USB device will be sophisticated enough to justify an operating system.  I've used [Mbed OS](https://os.mbed.com/), it's free, open-source and includes official support for STM devices.
