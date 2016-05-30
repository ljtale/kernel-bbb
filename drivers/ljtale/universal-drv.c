/*
 * universal driver source file @ljtale
 */

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

#include <linux/universal-drv.h>

/* 
 * global list to maintain driver childern of the universal driver
 */
struct list_head universal_drivers;
EXPORT_SYMBOL(universal_drivers);

/*
 * global list to maintian device defined as univeral_dev
 * */
struct list_head universal_devices;
EXPORT_SYMBOL(universal_devices);

extern void regacc_lock_mutex(void *__regacc);
extern void regacc_unlock_mutex(void *__regacc);

int __universal_drv_register(struct universal_driver *drv) {
    if (!drv) {
        LJTALE_MSG(KERN_ERR, "universal driver pointer null\n");
        return -ENODEV;
    }
    /* each driver should only have one instance of universal driver */
    list_add_tail(&drv->drv_list, &universal_drivers);
    LJTALE_MSG(KERN_INFO, "universal driver register: %s\n", drv->name);
    return 0;
}
EXPORT_SYMBOL_GPL(__universal_drv_register);


int __universal_drv_probe(struct universal_device *dev) {
    int ret = 0;
    
    if (!dev) {
        return -ENODEV;
    }
    LJTALE_MSG(KERN_INFO, "universal driver initialization: %s\n", dev->name);

    return ret;
err:
    /* FIXME: revoke what has been done */
    LJTALE_MSG(KERN_INFO "universal driver initializaiton failed\n");
    return ret;
}
EXPORT_SYMBOL_GPL(__universal_drv_probe);

/* TODO: function calls to revoke what has been done by the probe function */


static int __init init_universal_driver(void) {
    INIT_LIST_HEAD(&universal_drivers);
    INIT_LIST_HEAD(&universal_devices);
    return 0;
}
pure_initcall(init_universal_driver);
