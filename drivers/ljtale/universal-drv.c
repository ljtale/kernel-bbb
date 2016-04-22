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
#include <asm/uaccess.h>
#include <linux/err.h>

#include <linux/universal-drv.h>

/* 
 * global list to maintain driver childern of the universal driver
 */
static struct list_head universal_drivers;

int __universal_drv_register(struct universal_drv *drv) {
    if (!drv) {
        LJTALE_MSG(KERN_ERR, "universal driver pointer null\n");
        return -ENODEV;
    }
    /* each driver should only have one instance of universal driver */
    list_add_tail(&drv->list, &universal_drivers);
    LJTALE_MSG(KERN_INFO, "universal driver register: %s\n", drv->name);
    return 0;
}
EXPORT_SYMBOL_GPL(__universal_drv_register);

int __universal_drv_probe(struct universal_drv *drv) {
    int ret;
    struct universal_drv_config *config;

    if (!drv) {
        return -ENODEV;
    }
    LJTALE_MSG(KERN_INFO, "universal driver initialization: %s\n", drv->name);
    config = &drv->config;
    /* use the devm_regmap_init API provided by the regmap framework, this
     * should work for any regmap buses. 
     * The regmap_bus_context should be populated outside universal driver
     */
    config->regmap = devm_regmap_init(drv->dev, config->regmap_bus, 
                        config->regmap_bus_context, config->regmap_config);
    if (IS_ERR(config->regmap)) {
        ret = PTR_ERR(config->regmap);
        dev_err(drv->dev, 
                "Failed to allocate register map: %d\n", ret);
        return ret;
    }
    return 0;
}
EXPORT_SYMBOL_GPL(__universal_drv_probe);

static int __init init_universal_driver(void) {
    INIT_LIST_HEAD(&universal_drivers);
    return 0;
}
pure_initcall(init_universal_driver);
