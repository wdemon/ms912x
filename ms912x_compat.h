#ifndef MS912X_COMPAT_H
#define MS912X_COMPAT_H

#include <linux/version.h>
#include <linux/timer.h>
#include <linux/rcupdate.h>
#include <linux/container_of.h>
#include <drm/drm_device.h>

/* REPLACEMENT: provide from_timer macro when missing */
#ifndef from_timer
#define from_timer(var, timer, field) container_of(timer, typeof(*var), field)
#endif

/* REPLACEMENT: safe del_timer_sync analogue for older kernels */
static inline int ms912x_del_timer_sync(struct timer_list *timer)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0)
        return del_timer_sync(timer);
#else
        int ret = del_timer(timer);
        synchronize_rcu();
        return ret;
#endif
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
