# Start gdb using the following so that this .gdbinit will be run:
#  "C:\Program Files (x86)\GNU Arm Embedded Toolchain\9 2020-q2-update\bin\arm-none-eabi-gdb.exe" -iex "add-auto-load-safe-path .gdbinit" BUILD\DISCO_F723IE\GCC_ARM-DEBUG\usb-device.elf

target remote localhost:3333
