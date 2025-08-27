#include <linux/usb.h>
#include <linux/jiffies.h>

#include "ms912x.h"

/**
 * ms912x_transfer_framebuffer - send framebuffer to device over USB
 * @ms912x: device handle
 * @vaddr:   CPU mapped framebuffer address
 * @size:    number of bytes to transfer
 *
 * This helper pushes a complete framebuffer to the adapter using a synchronous
 * bulk transfer.  It is intentionally minimal so the driver can operate on
 * systems lacking the shadow plane helpers.
 */
int ms912x_transfer_framebuffer(struct ms912x_device *ms912x,
                                const void *vaddr, size_t size)
{
    struct usb_device *udev = interface_to_usbdev(ms912x->intf);
    unsigned int actual = 0;
    int ret;

    if (!vaddr || !size)
        return -EINVAL;

    ret = usb_bulk_msg(udev, usb_sndbulkpipe(udev, 0x04),
                       (void *)vaddr, size, &actual,
                       msecs_to_jiffies(5000));
    if (ret)
        dev_err(&udev->dev, "bulk transfer failed: %d\n", ret);
    else
        dev_dbg(&udev->dev, "framebuffer transferred: %u bytes\n", actual);

    return ret;
}
