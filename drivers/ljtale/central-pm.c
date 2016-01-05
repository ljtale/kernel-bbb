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
#include "central-pm.h"


int
central_pm_omap_i2c_ctx(struct device *dev) {
    struct platform_device *pdev = to_platform_device(dev);
    struct omap_i2c_dev *_dev = platform_get_drvdata(pdev);
    /* get all kinds of static property values from device tree node */
    struct device_node *node = dev->of_node;
    u16 psc = 0;
    of_property_read_u16_array(node, "psc", &psc, 1);
    printk(KERN_INFO "ljtale: central-pm psc: 0x%x\n", psc);

#if 0
    /* copy the register base and shift values */
    _dev->rpm_ctx.base = _dev->base;
    /* actually reg_shift is a constant value once the device revision is
     * fixed. Given a device, the device revision is fixed by definition */
    _dev->rpm_ctx.reg_shift = _dev->reg_shift;

    of_property_read_u16_array(node, "reset_all", 
            &_dev->rpm_ctx.resume.omap_i2c_con_reset_all, 1);

    of_property_read_u16_array(node, "psc", 
            &_dev->rpm_ctx.resume.omap_i2c_psc_val, 1);

    of_property_read_u16_array(node, "scll", 
            &_dev->rpm_ctx.resume.omap_i2c_scll_val, 1);

    of_property_read_u16_array(node, "sclh", 
            &_dev->rpm_ctx.resume.omap_i2c_sclh_val, 1);

    of_property_read_u16_array(node, "westate", 
            &_dev->rpm_ctx.resume.omap_i2c_con_we, 1);
#endif
    return 0;
}
EXPORT_SYMBOL(central_pm_omap_i2c_ctx);

int
central_pm_omap_i2c_resume(struct device *dev) {
    return 0;
}
EXPORT_SYMBOL(central_pm_omap_i2c_resume);
