#ifndef MS912X_COMPAT_H
#define MS912X_COMPAT_H

#include <linux/version.h>
#include <linux/timer.h>
#include <linux/workqueue.h> /* flush_workqueue support */
#include <linux/rcupdate.h>  /* needed for synchronize_rcu */
#include <linux/jiffies.h>   /* jiffies helper for mod_timer */
#include <linux/container_of.h> /* container_of for from_timer */
#include <drm/drm_device.h>

/*
 * REPLACEMENT: safe del_timer_sync analogue for older kernels.
 * Takes an optional workqueue to avoid flushing the global queues.
 */
static inline int ms912x_del_timer_sync(struct timer_list *timer,
                                       struct workqueue_struct *wq)
{
        int ret = del_timer(timer);
        if (wq)
                flush_workqueue(wq);
        synchronize_rcu();
        return ret;
}

/*
 * Provide from_timer() for kernels where it is missing.  The macro
 * behaves like the upstream helper and resolves the containing structure
 * from the timer pointer.
 */
#ifndef from_timer
#define from_timer(var, timer, field) \
        container_of(timer, typeof(*var), field)
#endif

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
