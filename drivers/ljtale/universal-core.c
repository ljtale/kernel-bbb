/*
 * universal driver core source file @ljtale
 * */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/pm_runtime.h>
#include <linux/pm_domain.h>
#include <linux/err.h>
#include <linux/mutex.h>

#include <linux/universal-drv.h>

extern struct list_head universal_drivers;
extern struct list_head universal_devices;

void regacc_lock_mutex(void *__regacc)
{
    struct register_accessor *regacc = __regacc;
    mutex_lock(&regacc->mutex);
}

void regacc_unlock_mutex(void *__regacc)
{
    struct register_accessor *regacc = __regacc;
    mutex_unlock(&regacc->mutex);
}

struct universal_device *check_universal_driver(struct device *dev) {
    struct universal_device *uni_dev = NULL;
    struct list_head *p;
    list_for_each(p, &universal_devices) {
        uni_dev = list_entry(p, struct universal_device, dev_list);
        if (uni_dev->dev == dev) {
            /* compare the address values directly, the device pointer
             * should be unique */
            break;
        }
    }
    /* didn't find any match */
    return uni_dev;
}
EXPORT_SYMBOL(check_universal_driver);
