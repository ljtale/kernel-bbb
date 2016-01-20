#ifndef CENTRAL_PM_
#define CENTRAL_PM_

#include <linux/kernel.h>
#include <linux/platform_device.h>

/* some helper macros */
#define LJTALE_MSG(LEVEL, stmt)             \
    do {                                        \
        printk(LEVEL "ljtale: " #stmt "\n");     \
    }                                           \
    while(0)

#define LJTALE_DEBUG_ENABLE

#ifdef LJTALE_DEBUG_ENABLE
#define LJTALE_PRINT(LEVEL, stmt)           \
    do {                                        \
        printk(LEVEL "ljtale: " #stmt "\n");    \
    }                                       \
while(0)
#else
#define LJTALE_PRINT(LEVEL, stmt)
#endif

/* values that need to be written to I2C register when resuming */
struct i2c_resume_values {
    u16 omap_i2c_con_reset_all;
    u16 omap_i2c_psc_val;
    u16 omap_i2c_scll_val;
    u16 omap_i2c_sclh_val;
    u16 omap_i2c_con_we;
    u16 omap_i2c_con_en;
    u16 omap_i2c_ie_val;
    /* bottom half of the resume function that needs to be done in the driver */
    int (*bottom_resume)(struct device *dev);

};

/* values that need to be written or read when suspending */
struct i2c_suspend_values {
    u16 omap_i2c_ie_val;    /* this value needs to be read */
    u16 omap_i2c_v2_int_mask;   /* this value needs to be written */
    u16 omap_i2c_stat_val_w;  /* this value needs to be written */
    u16 omap_i2c_stat_val_r;    /* this value needs to be read */
};

/* 
 * this context should be on a per-device manner. In a driver that supports
 * multiple devices, this is not able to describe static context for a
 * certain device. Thus the values of the context should come in through
 * something like device tree. Device tree would be a good candidate to
 * process such values
 * */
struct i2c_runtime_context {
    void __iomem *base;
    int reg_shift;
    struct i2c_resume_values resume;
    struct i2c_suspend_values suspend;
};

int central_pm_omap_i2c_ctx(struct device *dev);

int central_pm_omap_i2c_resume(struct device *dev);


/* universal init function for a device driver */
int universal_probe(struct platform_device *pdev);

/* global linked list to hold all the devices' rpm context */
extern struct list_head dev_rpm_ctx;


/* ioremap record table entry */
struct ioremap_tb_entry {
    const char *name;
    phys_addr_t phys;
    void __iomem *virt;
    struct list_head list;
};

/* global linked list to hold all the io mapping */
extern struct list_head ioremap_tbl;

#endif
