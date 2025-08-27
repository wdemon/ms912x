#ifndef MS912X_COMPAT_H
#define MS912X_COMPAT_H

#include <linux/version.h>
#include <linux/timer.h>
#include <linux/workqueue.h> // COMPAT: flush_workqueue support
#include <linux/rcupdate.h> // COMPAT: needed for synchronize_rcu
#include <linux/jiffies.h> // COMPAT: jiffies helper for mod_timer
#include <linux/container_of.h>
#include <drm/drm_device.h>

/* REPLACEMENT: provide from_timer macro when missing */
#ifndef from_timer
#define from_timer(var, timer, field) container_of(timer, typeof(*var), field)
#endif

/* REPLACEMENT: safe del_timer_sync analogue for older kernels */
static inline int ms912x_del_timer_sync(struct timer_list *timer)
{
        int was_pending = mod_timer(timer, ~0UL); // COMPAT: emulate del_timer
        flush_workqueue(system_long_wq);
        synchronize_rcu();
        return was_pending;
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
