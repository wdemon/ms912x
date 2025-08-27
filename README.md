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

## Testing

Check kernel logs and connected outputs:

```
dmesg | grep ms912x
modetest
cat /sys/class/drm/*/status
```

