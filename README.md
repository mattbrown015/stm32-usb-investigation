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

## Results

In all the tests the USB device was connected to a laptop host port labelled 'SS'.  The blue ports didn't work and the various 'SS' ports all seemed to give the same throughput.

### FS and Mbed OS USB

The first thing I got working used the FS port of the 32F723EDISCOVERY and was derived from `USBDevice`.

This is USB FS and hence the bulk endpoint maximum packet size (MPS) has to be 64 bytes.  The transfer size is also restricted to 64 bytes.

The throughput appeared to be around 3 Mbit/s out of a possible 12 Mbit/s.  This feels pretty average but for a first stab there's not much to say.  I know I want to investigate USB HS so there's no point trying to optimise the FS performance.

    perform 10000 bulk in transfers
    duration_us 1624392 us
    throughput MB/s 0.393994
    throughput Mbit/s 3.151949
    completed 10000 bulk in transfers

### HS and Mbed OS USB

After making a *fix* to Mbed OS, see https://github.com/mattbrown015/mbed-os, I got the test working using the USB HS port.

The bulk endpoint MPS and transfer size was still only 64 bytes because Mbed OS didn't allow anything.

The throughput didn't appear to change.

    perform 10000 bulk in transfers
    duration_us 1323553 us
    throughput MB/s 0.483547
    throughput Mbit/s 3.868376
    completed 10000 bulk in transfers

I tried switching on compiler optimisations but it didn't change the results.  The throughput is dominated by the USB not the CPU.

### HS and USB HAL

It took a while to work out how to use the USB HAL but I got there in the end.

Using the USB HAL made it possible to increase the bulk endpoint MPS to 512 bytes and perform transfers that are larger than one packet.

#### Transfer Size 512 bytes and MPS 512 bytes

With the MPS as 512 bytes and the transfer size 512 bytes, i.e. 1 packet, the throughput increased to around 30 Mbit/s out of a possible 480 Mbit/s.  This is definitely disappointing but it does confirm that it is USB HS (if the test can be trusted).

    perform 10000 bulk in transfers
    duration_us 1366002 us
    throughput MB/s 3.748164
    throughput Mbit/s 29.985315
    completed 10000 bulk in transfers

#### Transfer Size 1024 bytes and MPS 512 bytes

Increasing the transfer size to 1024 bytes makes a significant difference to the throughput, up to 50 Mbit/s from 30 Mbit/s.

I believe this is because it takes much less CPU work to prepare 1 1024 byte transfer as opposed to two 512 byte transfers.  I believe the STM puts all 1024 bytes in the TX FIFO at the same time and both packets will be sent without further CPU intervention.  I don't know how this is handled at the host/PC/libusb end but it is possible that it is all handled via DMA and/or in kernel space.

    perform 10000 bulk in transfers
    duration_us 1520971 us
    throughput MB/s 6.732541
    throughput Mbit/s 53.860330
    completed 10000 bulk in transfers

I removed the test of the bulk IN data in 'usb-host' and this increased the throughput to 60 Mbit/s.  This implies that we were wasting some bandwidth by not feeding the USB pipe.  All of the following tests will be done without the bulk IN data test.

    perform 10000 bulk in transfers
    duration_us 1349877 us
    throughput MB/s 7.585876
    throughput Mbit/s 60.687011
    completed 10000 bulk in transfers

#### Transfer Size 65536 bytes and MPS 512 bytes

Increasing the transfer size has a significant impact.  The throughput appears to be 270 Mbit/s when the transfer size is 65536 bytes.  I'd be happy if this throughput was maintained when the device also has to gather the data and the host has to do something with it.

    perform 10000 bulk in transfers
    duration_us 19334584 us
    throughput MB/s 33.895738
    throughput Mbit/s 271.165907
    completed 10000 bulk in transfers

#### Transfer Size 65536 bytes and MPS 64 bytes

Using a smaller MPS has some impact but perhaps not as much as I was expecting, 170 Mbit/s is still none too shabby!

    perform 10000 bulk in transfers
    duration_us 31220706 us
    throughput MB/s 20.991197
    throughput Mbit/s 167.929579
    completed 10000 bulk in transfers

#### Impact of EP1 Tx FIFO Size

I rearranged the data FIFO sizes to minimise the Rx FIFO and EP0 Tx FIFO, based on the recommendations, and thereby maximise the EP1 Tx FIFO.
* Rx FIFO = 0x98 words
* EP0 Tx FIFO = 0x20 words
* EP1 Tx FIFO = 0x400 - 0x98 - 0x20 - 12 = 0x33c = 828 words

828 words = 3312 bytes which allows for more than 6 packets in the Tx FIFO.

The Tx FIFO size didn't seem to have any effect on this test.

AFAICT, there is no advantage to having room for more than 2 packets in the Tx FIFO.  YMMV!

I think this is because the Tx FIFO empty level (TXFELVL) is set so that the TXFE interrupt indicates that the IN endpoint Tx FIFO is half empty.  In other words, while the USB core is sending one packet the CPU can be filling the Tx FIFO with the next packet.  If the CPU had other things to do and couldn't always respond immediately to the TXFE interrupt it might help to have more packets in the Tx FIFO.

##### Transfer Size 1024 bytes and MPS 512 bytes

    perform 10000 bulk in transfers
    duration_us 1362908 us
    throughput MB/s 7.513346
    throughput Mbit/s 60.106772
    completed 10000 bulk in transfers

##### Transfer Size 65536 bytes and MPS 512 bytes

    perform 10000 bulk in transfers
    duration_us 20095884 us
    throughput MB/s 32.611653
    throughput Mbit/s 260.893226
    completed 10000 bulk in transfers

#### Impact of DMA

The USB HS core includes a DMA engine to relieve the CPU of the onerous task of moving larger packets, e.g. 512 bytes, to/from system RAM.

As I understand it, the CPU stills takes the TXFE interrupt and has to manage the data being sent/received but the DMA handles the data copy.  In other words, the CPU has to something for every packet sent but it is much less when using DMA.

Using the DMA appears to improve the throughput slightly when the transfer is very large but it probably pulls more weight when the CPU has something else to do.

##### Transfer Size 1024 bytes, MPS 512 bytes and DMA

perform 10000 bulk in transfers
duration_us 1392012 us
throughput MB/s 7.356258
throughput Mbit/s 58.850067
completed 10000 bulk in transfers

##### Transfer Size 65536 bytes, MPS 512 bytes and DMA

perform 10000 bulk in transfers
duration_us 19222769 us
throughput MB/s 34.092903
throughput Mbit/s 272.743224
completed 10000 bulk in transfers
