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
#include <linux/slab.h>

#include <linux/universal-drv.h>

extern struct list_head universal_drivers;
extern struct list_head universal_devices;

void regacc_lock_mutex(void *__dev)
{
    struct universal_device *dev = __dev;
    mutex_lock(&dev->lock);
}

void regacc_unlock_mutex(void *__dev)
{
    struct universal_device *dev = __dev;
    mutex_unlock(&dev->lock);
}

/* check if there is a universal device for the existing struct device, also
 * check if there is a universal driver for that device. If both conditions
 * are true, return the universal device pointer, otherwise return NULL */
struct universal_device *check_universal_driver(struct device *dev) {
    struct universal_device *uni_dev = NULL;
    struct list_head *p;
    bool exist = false;
    list_for_each(p, &universal_devices) {
        uni_dev = list_entry(p, struct universal_device, dev_list);
        if (uni_dev->dev == dev && uni_dev->drv) {
            /* compare the address values directly, the device pointer
             * should be unique */
            LJTALE_MSG(KERN_INFO, "universal device %s and driver: %s exist\n",
                    uni_dev->name, uni_dev->drv->name);
            exist = true;
            break;
        }
    }
    /* didn't find any match */
    return (exist ? uni_dev : NULL);
}
EXPORT_SYMBOL(check_universal_driver);


struct universal_device *new_universal_device(struct device *dev) {
    struct universal_device *uni_dev;
    /* FIXME: probably should use devm for proper memory management? */
    uni_dev = kzalloc(sizeof(struct universal_device), GFP_KERNEL);
    if (!uni_dev) {
        return NULL;
    }
    /* the device possibly has not had a driver yet */
    /* temporarily I use the bus name to identify the universal device,
     * here the universal device name really doesn't matter?  */
    uni_dev->name = dev_name(dev);
    uni_dev->dev = dev;
    /* TODO: more initialization */
    uni_dev->drv = NULL;
    return uni_dev;
}
EXPORT_SYMBOL(new_universal_device);

/* match if there is a universal driver supporting the universal device
 * match is based on the compatible string provided by both the universal
 * driver and device */
int universal_driver_match_device(struct universal_driver *drv,
        struct universal_device *dev) {
    return 1;
}
EXPORT_SYMBOL(universal_driver_match_device);

/* For register accessors, there are two types of existing mechanism:
 * 1) The regmap framework provides a unified accessor interfaces for mfds,
 * such as the master-slave model buses, i2c or spi. Our hypothesis is drivers
 * for i2c or spi devicew will share the same set of regmap accessors. That
 * means we can build several fixed regmap buses, device vendors will only
 * need to tell the universal driver what it wish to use
 * 2) The load/store instruction set. For memory mapped I/O, device registers
 * can be read/written by load/store instructions. Only the
 * architecture-specific implementation of read/write would be different.
 * Currently in my prototype, I'll first build regmap framework, then we can
 * build a regmap-ish framework for memory mapped I/O */

