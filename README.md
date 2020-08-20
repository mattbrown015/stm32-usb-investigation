# STM32 USB Investigation
I'm interested in how to create a USB device based on a STM32 MCU.

The [STM32f723IE](https://www.st.com/content/st_com/en/products/microcontrollers-microprocessors/stm32-32-bit-arm-cortex-mcus/stm32-high-performance-mcus/stm32f7-series/stm32f7x3/stm32f723ie.html) has an integrated HS PHY and hence looks like a good choice for the basis of a USB HS device where size and power are not the primary concerns.

The [Discovery kit with STM32F723IE MCU](https://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-mpu-eval-tools/stm32-mcu-mpu-eval-tools/stm32-discovery-kits/32f723ediscovery.html) is cheap and easy to use.

It seems likely that a USB device will be sophisticated enough to justify an operating system.  I've used [Mbed OS](https://os.mbed.com/), it's free, open-source and includes official support for STM devices (and I've used it before).

## Mbed OS

Mbed OS 6.2 includes support for many STM development boards including the [NUCLEO-F767ZI](https://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-mpu-eval-tools/stm32-mcu-mpu-eval-tools/stm32-nucleo-boards/nucleo-f767zi.html).  The USB examples, see [USBCDC](https://os.mbed.com/docs/mbed-os/v6.2/apis/usbcdc.html) for an example, *just* build and work on the [NUCLEO-F767ZI](https://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-mpu-eval-tools/stm32-mcu-mpu-eval-tools/stm32-nucleo-boards/nucleo-f767zi.html).  It's unfortunate that the [NUCLEO-F767ZI](https://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-mpu-eval-tools/stm32-mcu-mpu-eval-tools/stm32-nucleo-boards/nucleo-f767zi.html) does not include a HS PHY.

The [32F723EDISCOVERY](https://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-mpu-eval-tools/stm32-mcu-mpu-eval-tools/stm32-discovery-kits/32f723ediscovery.html) is not target supported by Mbed OS 6.2, which is a shame.

Creating an Mbed OS custom target is straightforward, see (Adding and configuring targets)[https://os.mbed.com/docs/mbed-os/v6.2/program-setup/adding-and-configuring-targets.html].  It didn't take long to add a custom target for the [32F723EDISCOVERY](https://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-mpu-eval-tools/stm32-mcu-mpu-eval-tools/stm32-discovery-kits/32f723ediscovery.html).  The USB examples *just* worked on [32F723EDISCOVERY](https://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-mpu-eval-tools/stm32-mcu-mpu-eval-tools/stm32-discovery-kits/32f723ediscovery.html) USB FS interface.

(I think) As a consequence of [32F723EDISCOVERY](https://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-mpu-eval-tools/stm32-mcu-mpu-eval-tools/stm32-discovery-kits/32f723ediscovery.html) not being supported Mbed OS 6.2 does not support USB HS for STM targets.  [USBPhy_STM32.cpp](https://github.com/ARMmbed/mbed-os/blob/master/targets/TARGET_STM/USBPhy_STM32.cpp) includes some code that implies it might support USB HS, see [USBPhy_STM32.cpp#L197](https://github.com/ARMmbed/mbed-os/blob/master/targets/TARGET_STM/USBPhy_STM32.cpp#L197) for example, but, AFAICT, it is incomplete.

### Project Specific Changes to Mbed OS

It is likely/inevitable that any large project will need to make *special* changes to Mbed OS to solve some problem or other.  Managing these changes can be a bit of a pain.

#### Using Patch Files

In another project I used patch files to modify clones of https://github.com/ARMmbed/mbed-os.  The normal build flow was:
1. Clone https://github.com/ARMmbed/mbed-os
1. Checkout branch at specific commit
1. Apply patches
1. build

Updating the version of Mbed OS being used could require updates to the patch files.  The update flow goes something like:
1. Fetch https://github.com/ARMmbed/mbed-os
1. Checkout branch at more recent commit
1. Attempt to apply patches, if the patches still work then great
1. If the patches don't work...
1. Apply the patches manually, either:
    1. Make the changes by hand
    1. Use `git apply --3way` or `git --reject` to make the changes
1. Remake the patch files e.g. `git diff > file.patch`

This approach had the advantage of not requiring a fork of Mbed OS.

#### The Forking Workflow

The *forking workflow* makes it possible to make project specific changes to Mbed OS using a GitHub fork.  This makes the build flow simpler by avoiding the application of patches.

See [Colaborate#The forking workflow](https://os.mbed.com/docs/mbed-os/v6.2/build-tools/collaborate.html).  And (Forking Workflow)[https://www.atlassian.com/git/tutorials/comparing-workflows/forking-workflow] for a more general description.

At the start of this project I used `mbed new` which meant I was building a clone of https://github.com/ARMmbed/mbed-os.

At some point I wanted to make a change to https://github.com/ARMmbed/mbed-os so I forked it to https://github.com/mattbrown015/mbed-os and changed the path in `mbed.lib`.

This meant https://github.com/mattbrown015/mbed-os was the fetch and push origin for the clone.

After reading [The forking workflow](https://os.mbed.com/docs/mbed-os/v6.2/build-tools/collaborate.html) to update to Mbed OS 6.2.1 I did the following:

    C:...\stm32-usb-investigation\usb-device\mbed-os>git remote -v
    origin  https://github.com/mattbrown015/mbed-os (fetch)
    origin  https://github.com/mattbrown015/mbed-os (push)

    C:...\stm32-usb-investigation\usb-device\mbed-os>git remote set-url origin https://github.com/ARMmbed/mbed-os

    C:...\stm32-usb-investigation\usb-device\mbed-os>git remote -v
    origin  https://github.com/ARMmbed/mbed-os (fetch)
    origin  https://github.com/ARMmbed/mbed-os (push)

    C:...\stm32-usb-investigation\usb-device\mbed-os>git remote set-url --push origin https://github.com/mattbrown015/mbed-os

    C:...\stm32-usb-investigation\usb-device\mbed-os>git remote -v
    origin  https://github.com/ARMmbed/mbed-os (fetch)
    origin  https://github.com/mattbrown015/mbed-os (push)

    C:...\stm32-usb-investigation\usb-device\mbed-os>git fetch
    remote: Enumerating objects: 1996, done.
    remote: Counting objects: 100% (1996/1996), done.
    remote: Compressing objects: 100% (7/7), done.
    remote: Total 4699 (delta 1989), reused 1992 (delta 1989), pack-reused 2703 eceiving objects: 100% (4699/4699), 2.82 MiB | 120.00 KiB/s
    Receiving objects: 100% (4699/4699), 2.82 MiB | 96.00 KiB/s, done.
    Resolving deltas: 100% (2935/2935), completed with 980 local objects.
    From https://github.com/ARMmbed/mbed-os
     + 25cd91a72e...890f0562dc master               -> origin/master  (forced update)
     * [new branch]            dev_usb_remove_mbedh -> origin/dev_usb_remove_mbedh
     + faf32656e5...129e6970db feature-cmake        -> origin/feature-cmake  (forced update)
       3ab72c71b7..04d8eebf4e  feature-wisun        -> origin/feature-wisun
       cd3ed5129b..6a244d7adf  mbed-os-5.15         -> origin/mbed-os-5.15
     * [new tag]               mbed-os-5.15.5       -> mbed-os-5.15.5
     * [new tag]               mbed-os-5.15.5-rc1   -> mbed-os-5.15.5-rc1
     * [new tag]               mbed-os-6.2.0        -> mbed-os-6.2.0
     * [new tag]               mbed-os-6.2.1        -> mbed-os-6.2.1
     * [new tag]               mbed-os-6.2.1-rc2    -> mbed-os-6.2.1-rc2
     * [new tag]               mbed-os-6.2.1-rc1    -> mbed-os-6.2.1-rc1

    C:...\stm32-usb-investigation\usb-device\mbed-os>git merge mbed-os-6.2.1
    Removing targets/TARGET_Samsung/security_subsystem/sss_common.h
    Removing targets/TARGET_NUVOTON/TARGET_M251/PinNames.h
    Removing rtos/rtos.h
    Removing rtos/Thread.h
    ...
    Merge made by the 'recursive' strategy.
     .astyleignore                                      |    33 +-
     .travis.yml                                        |    55 +-
     DOXYGEN_FRONTPAGE.md                               |     9 +-
     LICENSE.md                                         |    14 +-
     TESTS/configs/baremetal.json                       |     7 +-
     TESTS/events/equeue/main.cpp                       |  1093 --
     TESTS/lorawan/loraradio/main.cpp                   |   291 -
     TESTS/mbed_hal/common_tickers_freq/main.cpp        |    10 +-
     TESTS/mbedmicro-rtos-mbed/basic/main.cpp           |   126 -
     TESTS/netsocket/README.md                          |  1903 ---
     TESTS/netsocket/dns/asynchronous_dns_timeouts.cpp  |    88 -
     TESTS/netsocket/nidd/main.cpp                      |   155 -
     TESTS/netsocket/tls/cert.h                         |    48 -
     TESTS/netsocket/udp/udpsocket_echotest_burst.cpp   |   247 -
     TESTS/network/emac/README.md                       |   297 -
     TESTS/network/emac/emac_TestNetworkStack.h         |   400 -
     TESTS/network/emac/emac_ctp.cpp                    |   153 -
     TESTS/network/emac/main.cpp                        |    84 -
    ...
     .../security_subsystem/drivers/sss_driver_sha2.c   |     0
     .../security_subsystem/drivers/sss_driver_sha2.h   |     0
     .../security_subsystem/drivers/sss_driver_util.c   |     0
     .../security_subsystem/drivers/sss_driver_util.h   |     0
     .../security_subsystem/drivers/sss_map.h           |     0
     .../security_subsystem/drivers/sss_oid.h           |     0
     .../security_subsystem/sss_common.h                |   176 +
     .../TARGET_Samsung/security_subsystem/sss_common.h |   176 -
     targets/targets.json                               |  2032 +--
     .../MTS_DRAGONFLY_F411RE/bootloader.bin            |   Bin 55608 -> 57908 bytes
     tools/targets/PSOC6.py                             |     1 +
     tools/test/examples/examples.json                  |    12 +-
     .../test/travis-ci/doxy-spellchecker/ignore.en.pws |     1 +
     4539 files changed, 212510 insertions(+), 128203 deletions(-)
     delete mode 100644 TESTS/events/equeue/main.cpp
    ...
    rename targets/TARGET_Samsung/{ => TARGET_SIDK_S5JS100}/security_subsystem/drivers/sss_map.h (100%)
    rename targets/TARGET_Samsung/{ => TARGET_SIDK_S5JS100}/security_subsystem/drivers/sss_oid.h (100%)
    create mode 100644 targets/TARGET_Samsung/TARGET_SIDK_S5JS100/security_subsystem/sss_common.h
    delete mode 100644 targets/TARGET_Samsung/security_subsystem/sss_common.h

    C:...\stm32-usb-investigation\usb-device\mbed-os>git status
    On branch master
    Your branch is ahead of 'origin/master' by 4 commits.
      (use "git push" to publish your local commits)

    nothing to commit, working tree clean

    C:...\stm32-usb-investigation\usb-device\mbed-os>git log
    commit 509a914f94d3f07a32b72fed44a0a578fe9f1f29 (HEAD -> master)
    Merge: 25cd91a72e 890f0562dc
    Author: Matthew Brown <matthewb@wokingham.ensilica.com>
    Date:   Thu Aug 20 16:24:23 2020 +0100

        Merge tag 'mbed-os-6.2.1' into master

        This is the mbed-os-6.2.1 release. For a full list of changes please refer to the release notes.

    commit 890f0562dc2efb7cf76a5f010b535c2b94bce849 (tag: mbed-os-6.2.1-rc2, tag: mbed-os-6.2.1, origin/master, origin/HEAD)
    Merge: 83fa12183b ace81a8be7
    Author: Martin Kojtal <martin.kojtal@arm.com>
    Date:   Tue Aug 18 10:17:53 2020 +0100

        Merge pull request #13447 from CharleyChu/topic/master_13440

        psoc64: Update TF-M release image

    C:...\stm32-usb-investigation\usb-device\mbed-os>git push
    Enumerating objects: 19, done.
    Counting objects: 100% (19/19), done.
    Delta compression using up to 4 threads
    Compressing objects: 100% (7/7), done.
    Writing objects: 100% (7/7), 753 bytes | 251.00 KiB/s, done.
    Total 7 (delta 6), reused 0 (delta 0), pack-reused 0
    remote: Resolving deltas: 100% (6/6), completed with 6 local objects.
    To https://github.com/mattbrown015/mbed-os
       ccf2f3d2e5..e98edcc3bd  master -> master

After this update `mbed.lib` to the merge commit.

#### Example

The [STM32f723IE](https://www.st.com/content/st_com/en/products/microcontrollers-microprocessors/stm32-32-bit-arm-cortex-mcus/stm32-high-performance-mcus/stm32f7-series/stm32f7x3/stm32f723ie.html) *only* has 5 SPI peripherals, it appears some/most of the other STM32F7 devices have at least 6 SPI peripherals.

Only having 5 SPI peripherals causes a problem because `SPI_6` is not defined and [spi_api.c](https://github.com/ARMmbed/mbed-os/blob/master/targets/TARGET_STM/TARGET_STM32F7/spi_api.c) doesn't build.

The *forking workflow* makes it possible to have mofifications to `spi_api.c`, see [TARGET_STM32F7: Support STM32F7 targets with only 5 SPIs.](https://github.com/mattbrown015/mbed-os/commit/4e9e09e93eb9f62f8a7e1ae68d6f1efffb52137e#diff-81a8a124f40f531af74aa6530b2d674b), and keep Mbed OS updated to its latest release.

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
