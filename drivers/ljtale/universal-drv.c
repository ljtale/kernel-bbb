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
    BUG_ON(!drv);
    list_add_tail(&drv->list, &universal_drivers);
    LJTALE_MSG(KERN_INFO, "universal driver register: %s\n", drv->name);
    return 0;
}
EXPORT_SYMBOL_GPL(__universal_drv_register);

int __universal_drv_init (struct universal_drv *drv) {
    LJTALE_MSG(KERN_INFO, "universal driver initialization: %s\n", drv->name);
    BUG_ON(!drv);
    struct universal_drv_config *config = &drv->config;
    config->regmap = devm_regmap_init_i2c(client, config->regmap_config);
    return 0;
}
EXPORT_SYMBOL_GPL(__universal_drv_init);

static int __init init_universal_driver(void) {
    INIT_LIST_HEAD(universal_drivers);
    return 0;
}
core_initcall(init_universal_driver);
