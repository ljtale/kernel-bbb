/* ljtale: test central module for omap i2c resume
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
#include <asm/uaccess.h>
#include <linux/err.h>

#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/central-pm.h>

/* global linked list to hold all the devices' rpm context */
struct list_head dev_rpm_ctx;

/* global linked list to hold all the io mapping */
struct list_head ioremap_tbl;

int
central_pm_omap_i2c_ctx(struct device *dev) {
    struct i2c_runtime_context *i2c_ctx;
    struct device_node *node;
    if (!dev) {
        return -EFAULT;
    }
    /* get all kinds of static property values from device tree node */
    node = dev->of_node;
    i2c_ctx =
        devm_kzalloc(dev, sizeof(struct i2c_runtime_context), GFP_KERNEL);
    if (!i2c_ctx) {
        return -ENOMEM;
    }
    /* copy the register base and shift values */
//    i2c_ctx->rpm_ctx.base = _dev->base;
    /* actually reg_shift is a constant value once the device revision is
     * fixed. Given a device, the device revision is fixed by definition */
//    i2c_ctx->rpm_ctx.reg_shift = _dev->reg_shift;

    of_property_read_u32_array(node, "reset_all", 
            (u32 *)&i2c_ctx->resume.omap_i2c_con_reset_all, 1);

    of_property_read_u32_array(node, "psc", 
            (u32 *)&i2c_ctx->resume.omap_i2c_psc_val, 1);
    printk(KERN_INFO "ljtale: central-pm psc: 0x%x\n",
            i2c_ctx->resume.omap_i2c_psc_val);

    of_property_read_u32_array(node, "scll", 
            (u32 *)&i2c_ctx->resume.omap_i2c_scll_val, 1);
    printk(KERN_INFO "ljtale: central-pm scll: ox%x\n",
            i2c_ctx->resume.omap_i2c_scll_val);

    of_property_read_u32_array(node, "sclh", 
            (u32 *)&i2c_ctx->resume.omap_i2c_sclh_val, 1);

    of_property_read_u32_array(node, "westate", 
            (u32 *)&i2c_ctx->resume.omap_i2c_con_we, 1);
    dev->rpm_data = i2c_ctx;
    return 0;
}
EXPORT_SYMBOL(central_pm_omap_i2c_ctx);

int
central_pm_omap_i2c_resume(struct device *dev) {
    /* TODO: lock protected */
    pinctrl_pm_select_default_state(dev);
    /* device init process */
    return 0;
}
EXPORT_SYMBOL(central_pm_omap_i2c_resume);



static int __init init_universal_driver(void) {
    INIT_LIST_HEAD(&dev_rpm_ctx);
    INIT_LIST_HEAD(&ioremap_tbl);
    return 0;
}
core_initcall(init_universal_driver);

/* TODO: currenly no exit function */
