/* mbed Microcontroller Library
 *******************************************************************************
 * Copyright (c) 2018, STMicroelectronics
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of STMicroelectronics nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 */

#include "PeripheralPins.h"
#include "mbed_toolchain.h"

//*** SERIAL ***

MBED_WEAK const PinMap PinMap_UART_TX[] = {
    {PC_6,       UART_6,  STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF8_USART6)}, // Connected to STDIO_UART_TX
    {NC, NC, 0}
};

MBED_WEAK const PinMap PinMap_UART_RX[] = {
    {PC_7,       UART_6,  STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF8_USART6)}, // Connected to STDIO_UART_RX
    {NC, NC, 0}
};

//*** USBDEVICE ***

MBED_WEAK const PinMap PinMap_USB_FS[] = {
    {PA_10,     USB_FS, STM_PIN_DATA(STM_MODE_AF_OD, GPIO_PULLUP, GPIO_AF10_OTG_FS)}, // USB_OTG_FS_ID // Connected to USB_OTGFS_ID
    {PA_11,     USB_FS, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF10_OTG_FS)}, // USB_OTG_FS_DM // Connected to USB_OTGFS_N
    {PA_12,     USB_FS, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF10_OTG_FS)}, // USB_OTG_FS_DP // Connected to USB_OTGFS_P
    {NC, NC, 0}
};

//*** USBDEVICE ***

MBED_WEAK const PinMap PinMap_USB_HS[] = {
    {PB_12,     USB_HS, STM_PIN_DATA(STM_MODE_AF_OD, GPIO_PULLUP, GPIO_AF12_OTG_HS_FS)}, // USB_OTG_HS_ID // Connected to USB_OTGHS_ID
    {PB_14,     USB_HS, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF12_OTG_HS_FS)}, // USB_OTG_HS_DM // Connected to USB_OTGHS_N
    {PB_15,     USB_HS, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_PULLUP, GPIO_AF12_OTG_HS_FS)}, // USB_OTG_HS_DP // Connected to USB_OTGHS_P
    {NC, NC, 0}
};
