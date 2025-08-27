// SPDX-License-Identifier: GPL-2.0-only

#include <linux/module.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_damage_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_helper.h> // FIX: ensure fb helper available
#include <drm/drm_file.h>
#include <drm/drm_gem_atomic_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_gem_shmem_helper.h>
#include <drm/drm_managed.h>
#include <drm/drm_ioctl.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_print.h>
#include <drm/drm_simple_kms_helper.h>
#include <drm/drm_shadow.h>

#include "ms912x.h"
#include "ms912x_compat.h" // REPLACEMENT: compatibility helpers

static int ms912x_usb_suspend(struct usb_interface *interface,
			      pm_message_t message)
{
	struct drm_device *dev = usb_get_intfdata(interface);

	return drm_mode_config_helper_suspend(dev);
}

static int ms912x_usb_resume(struct usb_interface *interface)
{
	struct drm_device *dev = usb_get_intfdata(interface);

	return drm_mode_config_helper_resume(dev);
}

/*
 * FIXME: Dma-buf sharing requires DMA support by the importing device.
 *        This function is a workaround to make USB devices work as well.
 *        See todo.rst for how to fix the issue in the dma-buf framework.
 */
static struct drm_gem_object *
ms912x_driver_gem_prime_import(struct drm_device *dev, struct dma_buf *dma_buf)
{
	struct ms912x_device *ms912x = to_ms912x(dev);

	if (!ms912x->dmadev)
		return ERR_PTR(-ENODEV);

	return drm_gem_prime_import_dev(dev, dma_buf, ms912x->dmadev);
}

DEFINE_DRM_GEM_FOPS(ms912x_driver_fops);

static const struct drm_driver driver = {
        .driver_features = DRIVER_ATOMIC | DRIVER_GEM | DRIVER_MODESET,

        /* GEM hooks */
        .fops = &ms912x_driver_fops,
        DRM_GEM_SHMEM_DRIVER_OPS,
        .gem_prime_import = ms912x_driver_gem_prime_import,

        .name = DRIVER_NAME,
        .desc = DRIVER_DESC,
        /* .date field removed in Linux 6.16+ */
        .major = DRIVER_MAJOR,
        .minor = DRIVER_MINOR,
        .patchlevel = DRIVER_PATCHLEVEL,
};

static const struct drm_mode_config_funcs ms912x_mode_config_funcs = {
	.fb_create = drm_gem_fb_create_with_dirty,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = drm_atomic_helper_commit,
};

static const struct ms912x_mode ms912x_mode_list[] = {
	/* Found in captures of the Windows driver */
	MS912X_MODE( 800,  600, 60, 0x4200, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1024,  768, 60, 0x4700, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1152,  864, 60, 0x4c00, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1280,  720, 60, 0x4f00, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1280,  800, 60, 0x5700, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1280,  960, 60, 0x5b00, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1280, 1024, 60, 0x6000, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1366,  768, 60, 0x6600, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1400, 1050, 60, 0x6700, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1440,  900, 60, 0x6b00, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1680, 1050, 60, 0x7800, MS912X_PIXFMT_UYVY),
	MS912X_MODE(1920, 1080, 60, 0x8100, MS912X_PIXFMT_UYVY),

	/* Dumped from the device */
        MS912X_MODE( 720,  480, 60, 0x0200, MS912X_PIXFMT_UYVY),
        MS912X_MODE( 720,  576, 50, 0x1100, MS912X_PIXFMT_UYVY), // FIX: correct refresh rate
        MS912X_MODE( 640,  480, 60, 0x4000, MS912X_PIXFMT_UYVY),
        MS912X_MODE(1024,  768, 75, 0x4900, MS912X_PIXFMT_UYVY), // FIX: correct refresh rate
        MS912X_MODE(1280,  600, 60, 0x4e00, MS912X_PIXFMT_UYVY),
        MS912X_MODE(1280,  768, 60, 0x5400, MS912X_PIXFMT_UYVY),
        MS912X_MODE(1280, 1024, 75, 0x6100, MS912X_PIXFMT_UYVY), // FIX: correct refresh rate
        MS912X_MODE(1360,  768, 60, 0x6400, MS912X_PIXFMT_UYVY),
        MS912X_MODE(1600, 1200, 60, 0x7300, MS912X_PIXFMT_UYVY),
        MS912X_MODE( 800,  600, 75, 0x4400, MS912X_PIXFMT_UYVY), // FIX: add mode
        MS912X_MODE(1280,  720, 50, 0x1300, MS912X_PIXFMT_UYVY), // FIX: add mode
        MS912X_MODE(1280,  768, 75, 0x5600, MS912X_PIXFMT_UYVY), // FIX: add mode
        MS912X_MODE(1920, 1080, 30, 0x2200, MS912X_PIXFMT_UYVY), // FIX: add mode
        MS912X_MODE(1920, 1080, 50, 0x1f00, MS912X_PIXFMT_UYVY), // FIX: add mode
	/* TODO: more mode numbers? */
};

static const struct ms912x_mode *
ms912x_get_mode(const struct drm_display_mode *mode)
{
        int i;
        int width = mode->hdisplay;
        int height = mode->vdisplay;
        int hz = drm_mode_vrefresh(mode);
        for (i = 0; i < ARRAY_SIZE(ms912x_mode_list); i++) {
                if (ms912x_mode_list[i].width == width &&
                    ms912x_mode_list[i].height == height &&
                    ms912x_mode_list[i].hz == hz) {
                        return &ms912x_mode_list[i];
                }
        }
        /* Unknown mode: indicate absence instead of an error pointer */
        return NULL;
}

static void ms912x_pipe_enable(struct drm_simple_display_pipe *pipe,
                               struct drm_crtc_state *crtc_state,
                               struct drm_plane_state *plane_state)
{
        struct ms912x_device *ms912x = to_ms912x(pipe->crtc.dev);
        struct drm_display_mode *mode = &crtc_state->mode;
        const struct ms912x_mode *ms_mode;
        struct drm_shadow_plane_state *shadow_state;
        struct drm_framebuffer *fb = plane_state->fb;
        struct drm_rect rect;
        int ret;

        drm_dbg(&ms912x->drm, "pipe enable\n");

        ms912x_power_on(ms912x);

        if (crtc_state->mode_changed) {
                ms_mode = ms912x_get_mode(mode);
                if (ms_mode) {
                        ms912x_set_resolution(ms912x, ms_mode);
                } else {
                        drm_err(&ms912x->drm,
                                "unsupported mode %dx%d@%d\n",
                                mode->hdisplay, mode->vdisplay,
                                drm_mode_vrefresh(mode));
                }
        }

        if (!fb)
                return;

        shadow_state = to_drm_shadow_plane_state(plane_state);
        rect.x1 = 0;
        rect.y1 = 0;
        rect.x2 = fb->width;
        rect.y2 = fb->height;

        ret = ms912x_fb_send_rect(fb, &shadow_state->data[0], &rect);
        if (ret)
                drm_err(&ms912x->drm,
                        "initial frame transfer failed: %d\n", ret);

        ms912x->update_rect = rect;
}

static void ms912x_pipe_disable(struct drm_simple_display_pipe *pipe)
{
	struct ms912x_device *ms912x = to_ms912x(pipe->crtc.dev);

	ms912x_power_off(ms912x);
}

static enum drm_mode_status
ms912x_pipe_mode_valid(struct drm_simple_display_pipe *pipe,
                       const struct drm_display_mode *mode)
{
        const struct ms912x_mode *ret = ms912x_get_mode(mode);

        if (!ret)
                return MODE_BAD;

        return MODE_OK;
}

static int ms912x_pipe_check(struct drm_simple_display_pipe *pipe,
                             struct drm_plane_state *new_plane_state,
                             struct drm_crtc_state *new_crtc_state)
{
        struct drm_device *dev = pipe->crtc.dev;
        struct drm_framebuffer *fb = new_plane_state->fb;

        drm_dbg(dev, "atomic_check\n");

        if (fb) {
                if (fb->format->format != DRM_FORMAT_XRGB8888) {
                        drm_err(dev, "unsupported format %p4cc\n",
                                &fb->format->format);
                        return -EINVAL;
                }

                if (!ms912x_get_mode(&new_crtc_state->mode)) {
                        drm_err(dev, "mode %dx%d@%d not supported\n",
                                new_crtc_state->mode.hdisplay,
                                new_crtc_state->mode.vdisplay,
                                drm_mode_vrefresh(&new_crtc_state->mode));
                        return -EINVAL;
                }
        }

        return 0;
}

static void ms912x_merge_rects(struct drm_rect *dest, struct drm_rect *r1,
			       struct drm_rect *r2)
{
	dest->x1 = min(r1->x1, r2->x1);
	dest->y1 = min(r1->y1, r2->y1);
	dest->x2 = max(r1->x2, r2->x2);
	dest->y2 = max(r1->y2, r2->y2);
}

static void ms912x_pipe_update(struct drm_simple_display_pipe *pipe,
                               struct drm_plane_state *old_state)
{
        struct drm_plane_state *state = pipe->plane.state;
        struct drm_shadow_plane_state *shadow_plane_state =
                to_drm_shadow_plane_state(state);
        struct ms912x_device *ms912x;
        struct drm_rect current_rect, rect;

        ms912x = to_ms912x(state->fb->dev);
        drm_dbg(&ms912x->drm, "pipe update\n");

        if (drm_atomic_helper_damage_merged(old_state, state, &current_rect)) {
                /* The device double buffers, so we need to send the update
                 * rects of the last two frames.
                 */
                ms912x_merge_rects(&rect, &current_rect, &ms912x->update_rect);
                if (ms912x_fb_send_rect(state->fb, &shadow_plane_state->data[0],
                                        &rect)) {
                        /* In case of error, merge the rects to update later */
                        ms912x_merge_rects(&ms912x->update_rect,
                                           &ms912x->update_rect, &rect);
                } else {
                        ms912x->update_rect = current_rect;
                }
        }
}

static int ms912x_pipe_prepare_fb(struct drm_simple_display_pipe *pipe,
                                  struct drm_plane_state *plane_state)
{
        struct drm_device *dev = pipe->crtc.dev;
        int ret;

        drm_dbg(dev, "prepare_fb\n");

        ret = drm_gem_simple_display_pipe_prepare_fb(pipe, plane_state);
        if (ret)
                drm_err(dev, "prepare_fb failed: %d\n", ret);

        return ret;
}

static void ms912x_pipe_cleanup_fb(struct drm_simple_display_pipe *pipe,
                                   struct drm_plane_state *plane_state)
{
        drm_dbg(pipe->crtc.dev, "cleanup_fb\n");
        drm_gem_simple_display_pipe_cleanup_fb(pipe, plane_state);
}

static const struct drm_simple_display_pipe_funcs ms912x_pipe_funcs = {
        .prepare_fb = ms912x_pipe_prepare_fb,
        .cleanup_fb = ms912x_pipe_cleanup_fb,
        .enable = ms912x_pipe_enable,
        .disable = ms912x_pipe_disable,
        .check = ms912x_pipe_check,
        .mode_valid = ms912x_pipe_mode_valid,
        .update = ms912x_pipe_update,
};

/* Initialization structure with the default GEM helpers so the core allocates
 * shadow plane state for us.  After drm_simple_display_pipe_init() we replace
 * the function pointer with ms912x_pipe_funcs to gain logging.
 */
static const struct drm_simple_display_pipe_funcs ms912x_pipe_init_funcs = {
        DRM_GEM_SIMPLE_DISPLAY_PIPE_SHADOW_PLANE_FUNCS,
        .enable = ms912x_pipe_enable,
        .disable = ms912x_pipe_disable,
        .check = ms912x_pipe_check,
        .mode_valid = ms912x_pipe_mode_valid,
        .update = ms912x_pipe_update,
};

static const uint32_t ms912x_pipe_formats[] = {
        DRM_FORMAT_XRGB8888,
};

static int ms912x_usb_probe(struct usb_interface *interface,
                            const struct usb_device_id *id)
{
        int ret;
        struct ms912x_device *ms912x;
        struct drm_device *dev;
        struct usb_device *udev = interface_to_usbdev(interface);
        struct usb_host_interface *alt = interface->cur_altsetting;

        /* Log basic device information */
        dev_info(&interface->dev,
                 "probe: if=%u class=%u subclass=%u id= %04x:%04x\n",
                 alt->desc.bInterfaceNumber, alt->desc.bInterfaceClass,
                 alt->desc.bInterfaceSubClass,
                 le16_to_cpu(udev->descriptor.idVendor),
                 le16_to_cpu(udev->descriptor.idProduct));
        dev_dbg(&interface->dev, "ms912x usb probe\n");

        if (alt->desc.bInterfaceNumber != 3 ||
            alt->desc.bInterfaceClass != USB_CLASS_VENDOR_SPEC) {
                dev_dbg(&interface->dev, "unsupported interface %u/%u\n",
                        alt->desc.bInterfaceNumber, alt->desc.bInterfaceClass);
                return -ENODEV;
        }

        ms912x = devm_drm_dev_alloc(&interface->dev, &driver,
                                    struct ms912x_device, drm);
        if (IS_ERR(ms912x))
                return PTR_ERR(ms912x);

        ms912x->intf = interface;
        dev = &ms912x->drm;

        ms912x->dmadev = usb_intf_get_dma_device(interface);
        if (!ms912x->dmadev)
                drm_warn(dev,
                         "buffer sharing not supported"); /* not an error */

        ms912x->wq = alloc_workqueue(DRIVER_NAME "_wq",
                                     WQ_UNBOUND | WQ_MEM_RECLAIM, 1);
        if (!ms912x->wq) {
                ret = -ENOMEM;
                goto err_put_device;
        }

        ret = drmm_mode_config_init(dev);
        if (ret)
                goto err_destroy_wq;

        dev->mode_config.min_width = 0;
        dev->mode_config.max_width = 2048;
        dev->mode_config.min_height = 0;
        dev->mode_config.max_height = 2048;
        dev->mode_config.funcs = &ms912x_mode_config_funcs;

        /* This stops weird behavior in the device */
        ms912x_set_resolution(ms912x, &ms912x_mode_list[0]);

        ret = ms912x_init_request(ms912x, &ms912x->requests[0],
                                  2048 * 2048 * 2);
        if (ret)
                goto err_destroy_wq;

        ret = ms912x_init_request(ms912x, &ms912x->requests[1],
                                  2048 * 2048 * 2);
        if (ret)
                goto err_free_request_0;
        complete(&ms912x->requests[1].done);

        ret = ms912x_connector_init(ms912x);
        if (ret)
                goto err_free_request_1;

        ret = drm_simple_display_pipe_init(&ms912x->drm, &ms912x->display_pipe,
                                           &ms912x_pipe_init_funcs,
                                           ms912x_pipe_formats,
                                           ARRAY_SIZE(ms912x_pipe_formats),
                                           NULL, &ms912x->connector);
        if (ret)
                goto err_free_request_1;

        /* After initialization replace the pipe callbacks with our logging
         * variants.  drm_simple_display_pipe_init() already allocated shadow
         * plane state because we passed the GEM helpers above.
         */
        ms912x->display_pipe.funcs = &ms912x_pipe_funcs;

        drm_plane_enable_fb_damage_clips(&ms912x->display_pipe.plane);

        drm_mode_config_reset(dev); // FIX: correct typo drrm_ -> drm_

        usb_set_intfdata(interface, ms912x);

        drm_kms_helper_poll_init(dev);

        ret = drm_dev_register(dev, 0);
        if (ret)
                goto err_free_request_1;

        ms912x_fbdev_setup(dev); // REPLACEMENT: optional fbdev setup

        dev_info(&interface->dev, "ms912x device bound\n");

        return 0;

err_free_request_1:
        ms912x_free_request(&ms912x->requests[1]);
err_free_request_0:
        ms912x_free_request(&ms912x->requests[0]);
err_destroy_wq:
        destroy_workqueue(ms912x->wq);
err_put_device:
        if (ms912x->dmadev)
                put_device(ms912x->dmadev);
        return ret;
}

static void ms912x_usb_disconnect(struct usb_interface *interface)
{
        struct ms912x_device *ms912x = usb_get_intfdata(interface);
        struct drm_device *dev = &ms912x->drm;
        struct usb_device *udev = interface_to_usbdev(interface);
        struct usb_host_interface *alt = interface->cur_altsetting;

        dev_info(&interface->dev,
                 "disconnect: if=%u class=%u subclass=%u id=%04x:%04x\n",
                 alt->desc.bInterfaceNumber, alt->desc.bInterfaceClass,
                 alt->desc.bInterfaceSubClass,
                 le16_to_cpu(udev->descriptor.idVendor),
                 le16_to_cpu(udev->descriptor.idProduct));
        dev_dbg(&interface->dev, "ms912x usb disconnect\n");

        cancel_work_sync(&ms912x->requests[0].work);
        cancel_work_sync(&ms912x->requests[1].work);
        destroy_workqueue(ms912x->wq);
        drm_kms_helper_poll_fini(dev);
        drm_dev_unplug(dev);
        drm_atomic_helper_shutdown(dev);
        ms912x_free_request(&ms912x->requests[0]);
        ms912x_free_request(&ms912x->requests[1]);
        if (ms912x->dmadev) {
                put_device(ms912x->dmadev);
                ms912x->dmadev = NULL;
        }
        usb_set_intfdata(interface, NULL);
}

static const struct usb_device_id id_table[] = {
        { USB_DEVICE_INTERFACE_NUMBER(0x534d, 0x6021, 3),
          .bInterfaceClass = USB_CLASS_VENDOR_SPEC,
          .bInterfaceSubClass = 0x00,
          .bInterfaceProtocol = 0x00 },
        { USB_DEVICE_INTERFACE_NUMBER(0x534d, 0x0821, 3),
          .bInterfaceClass = USB_CLASS_VENDOR_SPEC,
          .bInterfaceSubClass = 0x00,
          .bInterfaceProtocol = 0x00 },
        { USB_DEVICE_INTERFACE_NUMBER(0x345f, 0x9132, 3),
          .bInterfaceClass = USB_CLASS_VENDOR_SPEC,
          .bInterfaceSubClass = 0x00,
          .bInterfaceProtocol = 0x00 },
        { USB_DEVICE_INTERFACE_NUMBER(0x345f, 0x9133, 3),
          .bInterfaceClass = USB_CLASS_VENDOR_SPEC,
          .bInterfaceSubClass = 0x00,
          .bInterfaceProtocol = 0x00 },
        { }
};
MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_driver ms912x_driver = {
        .name = "ms912x",
        .probe = ms912x_usb_probe,
        .disconnect = ms912x_usb_disconnect,
        .suspend = ms912x_usb_suspend,
        .resume = ms912x_usb_resume,
        .id_table = id_table,
};

static int __init ms912x_init(void)
{
        int ret = usb_register(&ms912x_driver);
        if (!ret)
                pr_info("ms912x module loaded\n");
        return ret;
}

static void __exit ms912x_exit(void)
{
        usb_deregister(&ms912x_driver);
        pr_info("ms912x module unloaded\n");
}

module_init(ms912x_init);
module_exit(ms912x_exit);
MODULE_DESCRIPTION("MS912X USB-HDMI video adapter driver");
MODULE_AUTHOR("wdemon");
MODULE_LICENSE("GPL");
