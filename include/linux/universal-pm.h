#ifndef _LINUX_UNIVERSAL_PM_H
#define _LINUX_UNIVERSAL_PM_H

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/of_device.h>
#include <linux/interrupt.h>
#include <linux/dmaengine.h>
#include <linux/omap-dmaengine.h>
#include <linux/pm_runtime.h>

/*
#include <linux/i2c.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/platform_data/at24.h>
*/

#include <linux/universal-rpm.h>

int universal_suspend(struct device *dev);
int universal_resume(struct device *dev);
int universal_pm_create_reg_context(struct universal_device *uni_dev);

extern int process_reg_table(struct universal_device *uni_dev,
        struct universal_reg_entry *tbl, int table_size);
extern int universal_disable_clk(struct universal_device *uni_dev);
extern int universal_enable_clk(struct universal_device *uni_dev);

struct universal_reg_entry;

struct universal_pm_ctx {
    u32 *array;
    int size;
};

struct universal_save_context_tbl;
struct universal_restore_context_tbl;
struct universal_save_context;
struct universal_restore_context;

struct universal_disable_irq_tbl;
struct universal_enable_irq_tbl;
struct universal_disable_irq;
struct universal_enable_irq;

struct universal_pin_control;

struct universal_disable_clk;
struct universal_enabled_clk;

/* if we move timer initialization to the universal driver, we
 * will not need to pass the timer variable here. */
struct universal_deactivate_timer {
    struct timer_list timer;
};

struct universal_reactivate_timer {
    struct timer_list timer;
};

#endif /* _LINUX_UNIVERSAL_PM_H */
