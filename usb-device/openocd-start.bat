rem Download https://github.com/xpack-dev-tools/openocd-xpack/releases/download/v0.10.0-14/xpack-openocd-0.10.0-14-win32-x64.zip
rem Unzip into ""C:\Program Files\GNU MCU Eclipse\OpenOCD\"
rem The prebuilt binaries for OpenOCD on Windows have moved around a bit and this directory name might not be entirely
rem appropriate but I had a previous version installed here.
rem Connect NUCLEO_F767ZI in the /normal/ way using the USB and the on-board ST-LINK/V2

"C:\Program Files\GNU MCU Eclipse\OpenOCD\xpack-openocd-0.10.0-14\bin\openocd.exe" --file "C:\Program Files\GNU MCU Eclipse\OpenOCD\xpack-openocd-0.10.0-14\scripts\interface\stlink.cfg" --file "C:\Program Files\GNU MCU Eclipse\OpenOCD\xpack-openocd-0.10.0-14\scripts\board\st_nucleo_f7.cfg"
