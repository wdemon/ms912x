# ms912x driver for Linux

Linux kernel driver for MacroSilicon USB to VGA/HDMI adapter.

There are two variants:
 - VID/PID is 534d:6021. Device is USB 2
 - VID/PID is 345f:9132. Device is USB 3

For kernel 6.1 checkout branch kernel-6.1

TODOs:

- Detect connector type (VGA, HDMI, etc...)
- More resolutions
- Error handling
- Is RGB to YUV conversion needed?

## Development 

Driver is written by analyzing wireshark captures of the device.

## Build

Run `make` to build the module against the currently running kernel.

## DKMS

To build via DKMS:

```
sudo dkms add .
sudo dkms build ms912x/0.1
sudo dkms install ms912x/0.1
```

## Loading

You can load the module manually:

```
sudo insmod ms912x.ko
```

## Tray utility and automatic start

A small PyQt6 tray helper (`ms912x_tray.py`) provides a red status icon and a
context menu entry to unload the driver.  The utility starts automatically when
an adapter is plugged in.

To enable the automation copy the files to the appropriate system locations:

```
sudo install -m644 99-ms912x.rules /etc/udev/rules.d/
sudo install -m755 ms912x_tray.py /usr/bin/
sudo install -m644 ms912x_tray.service /usr/lib/systemd/user/
systemctl --user daemon-reload
```

The udev rule triggers the `ms912x_tray.service` user unit which launches the
tray application.  Right‑click the icon and choose *Выйти* to unload the module
and close the utility.

## Testing

Check kernel logs and connected outputs:

```
dmesg | grep ms912x
modetest
cat /sys/class/drm/*/status
```

