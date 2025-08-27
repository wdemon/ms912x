#ifndef MS912X_COMPAT_H
#define MS912X_COMPAT_H

#include <linux/version.h>
#include <linux/timer.h>
#include <linux/container_of.h> /* container_of for helpers */
#include <linux/workqueue.h>    /* still used elsewhere */
#include <drm/drm_device.h>

/*
 * Modern kernels (>=6.5) removed del_timer* helpers.  Provide a thin wrapper
 * around timer_shutdown_sync() which guarantees the timer is cancelled and no
 * callback is running.  The wrapper mirrors the previous ms912x_del_timer_sync
 * semantics but does not return a value since timer_shutdown_sync() does not.
 */
static inline void ms912x_shutdown_timer(struct timer_list *timer)
{
        if (!timer)
                return;

        timer_shutdown_sync(timer);
}


/* REPLACEMENT: wrapper for fbdev setup so driver builds without drm_fbdev_generic */
#if __has_include(<drm/drm_fbdev_generic.h>)
#include <drm/drm_fbdev_generic.h>
static inline void ms912x_fbdev_setup(struct drm_device *dev)
{
        drm_fbdev_generic_setup(dev, 0);
}
#else
static inline void ms912x_fbdev_setup(struct drm_device *dev)
{
        /* fbdev emulation not available */
}
#endif

#endif /* MS912X_COMPAT_H */
