# Python Version of usb-host

I've got a vague plan to attempt to visualise the USB data on a graph.  I'm wondering if it will be easier to do this using [Python](https://www.python.org/) rather than C++.

I also think that attempting to create a GUI application using [mingw-w64](http://mingw-w64.org/doku.php), i.e. what I've used for my first usb-host, will be *swimming against the tide*.

Hence I've started this project.

## Procedure

1. Install Python 3.8
1. Install [PyUSB](https://github.com/pyusb/pyusb) i.e. `pip install pyusb`
1. Install libusb for Windows
    1. Download latest Windows binaries e.g. https://github.com/libusb/libusb/releases/download/v1.0.23/libusb-1.0.23.7z
    1. Unzip somewhere appropriate
    1. Ensure Windows 64-bit dll is on the path, e.g. `set PATH = %PATH%;libusb\MS64\dll`

## `OSError: exception:`

I was seeing an exception and didn't know if it was my fault given that this is my first time with PyUSB...

    Exception ignored in: <function _AutoFinalizedObjectBase.__del__ at 0x000001C428951280>
    Traceback (most recent call last):
      File "C:\Users\matthewb\AppData\Local\Programs\Python\Python38\lib\site-packages\usb\_objfinalizer.py", line 84, in __del__
        self.finalize()
      File "C:\Users\matthewb\AppData\Local\Programs\Python\Python38\lib\site-packages\usb\_objfinalizer.py", line 144, in finalize
        self._finalizer()
      File "C:\Users\matthewb\AppData\Local\Programs\Python\Python38\lib\weakref.py", line 566, in __call__
        return info.func(*info.args, **(info.kwargs or {}))
      File "C:\Users\matthewb\AppData\Local\Programs\Python\Python38\lib\site-packages\usb\_objfinalizer.py", line 104, in _do_finalize_object_ref
        obj._do_finalize_object()
      File "C:\Users\matthewb\AppData\Local\Programs\Python\Python38\lib\site-packages\usb\_objfinalizer.py", line 71, in _do_finalize_object
        self._finalize_object()
      File "C:\Users\matthewb\AppData\Local\Programs\Python\Python38\lib\site-packages\usb\backend\libusb1.py", line 604, in _finalize_object
        _lib.libusb_unref_device(self.devid)
    OSError: exception: access violation writing 0x0000000000000024

It turns out PyUSB 1.0.2, as installed by `pip`, has a bug see [OSError with wxPython #203](https://github.com/pyusb/pyusb/issues/203) and [OSError: exception: access violation writing 0x00000014 in libusb1.py #288](https://github.com/pyusb/pyusb/issues/288).

Hence I installed the git version using `pip install git+https://github.com/pyusb/pyusb`. Once PyUSB is released this should no longer be necessary.