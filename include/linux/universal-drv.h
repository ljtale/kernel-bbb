#ifndef _LINUX_UNIVERSAL_DRV_H
#define _LINUX_UNIVERSAL_DRV_H

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/ljtale-utils.h>


/*
 * universal driver configuration struct 
 */

struct universal_drv_config {
    const char *name;
    struct regmap_config *regmap_config;
    struct regmap *regmap;
};

/*
 * universal driver struct 
 */

struct universal_drv {
    const char *name;
    /* FIXME: this only works for i2c devices
     * Since the universal driver does not know the data structure in the 
     * specific driver, it should not assume it can use container of to
     * go back to the original  */
    struct i2c_client *client;
    struct universal_drv_config config;
    struct list_head list;
};



extern int __universal_drv_register(struct universal_drv *drv);
#define universal_drv_register(drv) \
    __universal_drv_register(drv)

extern int __universal_drv_probe(struct universal_drv *drv);
#define universal_drv_init(drv) \
    __universal_drv_probe(drv)

#endif

