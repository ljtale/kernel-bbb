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

/*===========extern variables and functions====*/
extern void debug_list_print(void);
extern struct regmap_bus *regmap_get_i2c_bus_general(void);

/* 
 * global list to maintain driver childern of the universal driver
 * this is not actually a list of drivers, but a list of data structures
 * to store the device knowledge that drives the universal driver to do 
 * things for the device
 */
struct list_head universal_drivers;
EXPORT_SYMBOL(universal_drivers);

/*
 * global list to maintian device defined as univeral_dev
 * */
struct list_head universal_devices;
EXPORT_SYMBOL(universal_devices);

struct regmap_bus i2c_regmap_bus;
struct regmap_bus i2c_eeprom_regmap_bus;
struct regmap_bus spi_regmap_bus;
struct regmap_bus spi_eeprom_regmap_bus;


int __universal_drv_register(struct universal_driver *drv) {
    struct universal_device *dev;
    struct list_head *p;
    int ret;
    if (!drv) {
        LJTALE_MSG(KERN_ERR, "universal driver pointer null\n");
        return -ENODEV;
    }
    /* each driver should only have one instance of universal driver */
    list_add_tail(&drv->drv_list, &universal_drivers);
    LJTALE_MSG(KERN_INFO, "universal driver register: %s\n", drv->name);
    debug_list_print();
    /* bind the universal driver data to the universal device */
    /* FIXME: we need a universal-driver-specific matching mechanism
     * Maybe also involves an id talbe just like the device tree, this
     * should be relatively easier than other parts of the driver  */
    list_for_each(p, &universal_devices) {
        dev = list_entry(p, struct universal_device, dev_list);
        if (dev->dev->bus)
            if (dev->dev->bus->match && !dev->drv) {
                ret = dev->dev->bus->match(dev->dev, drv->driver);
                if (ret) {
                    dev->drv = drv;
                    break;
                }
            }
    }
    /* if there is no match for this universal driver, fine, just exit */
    return 0;
}
EXPORT_SYMBOL(__universal_drv_register);


int __universal_drv_unregister(struct universal_driver *drv) {
    return 0;
}
EXPORT_SYMBOL(__universal_drv_unregister);

int __universal_drv_probe(struct universal_device *dev) {
    int ret = 0;
    struct universal_driver *drv;
    
    if (!dev || !dev->drv) {
        LJTALE_MSG(KERN_WARNING, 
                "no universal driver for universal device: %s\n", dev->name);
        return -ENODEV;
    }
    LJTALE_MSG(KERN_INFO, "universal driver probe: %s\n", dev->name);
    drv = dev->drv;
    /* do a set of initialization */

    /* do a series of universal driver probe */
    /* for register accessors */
    mutex_init(&dev->lock);
    
    /* ... */
    /* do a local probe */
    if (drv->local_probe)
        ret = drv->local_probe(dev);
    return ret;
}
EXPORT_SYMBOL(__universal_drv_probe);


int __universal_dev_register(struct universal_device *dev) {
    struct universal_driver *drv;
    struct list_head *p;
    int ret;
    if (!dev) {
        LJTALE_MSG(KERN_ERR, "universal device pointer null\n");
        return -ENODEV;
    }
    /* temporarily the regisration only adds the device to the device list
     * TODO: more registration information needed here later */
    list_add_tail(&dev->dev_list, &universal_devices);
    LJTALE_MSG(KERN_INFO, "universal device registered: %s\n", dev->name);
    debug_list_print();
    /* bind the universal driver data to the universal driver device */
    if (dev->dev->bus) {
        if(dev->dev->bus->match) {
           list_for_each(p, &universal_drivers) {
               drv = list_entry(p, struct universal_driver, drv_list);
               ret = dev->dev->bus->match(dev->dev, drv->driver);
               if (ret) {
                   dev->drv = drv;
                   break;
               }
           }
           if (!dev->drv) {
               LJTALE_MSG(KERN_WARNING, 
                       "universal device: %s does not find a" 
                       "universal driver data to match\n", dev->name);
           }
        }
    } 
    else
        return -ENODEV;
    return 0;
}
EXPORT_SYMBOL(__universal_dev_register);

int __universal_dev_unregister(struct universal_device *dev) {
    return 0;
}
EXPORT_SYMBOL(__universal_dev_unregister);


/* TODO: function calls to revoke what has been done by the probe function */


static int __init init_universal_driver(void) {
    struct regmap_bus *regmap_bus_temp;
//    struct regmap_config *regmap_config_temp;
    INIT_LIST_HEAD(&universal_drivers);
    INIT_LIST_HEAD(&universal_devices);
    /* for register accessors, initialize regmap buses and normal load/store
     * accessors */
    regmap_bus_temp = regmap_get_i2c_bus_general();
    i2c_regmap_bus = *regmap_bus_temp;
    /* build a regmap bus for i2c eeprom device according to the at24 driver */

    return 0;
}
pure_initcall(init_universal_driver);
