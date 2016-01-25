#ifndef CENTRAL_PM_
#define CENTRAL_PM_

#include <linux/kernel.h>
#include <linux/platform_device.h>

/* some helper macros */
#define LJTALE_MSG(LEVEL, args...)             \
    do {                                        \
        printk(LEVEL "ljtale: " args);                   \
    }                                           \
    while(0)

#define LJTALE_DEBUG_ENABLE

#ifdef LJTALE_DEBUG_ENABLE
#define LJTALE_PRINT(LEVEL, args...)           \
    do {                                        \
        printk(LEVEL "ljtale: " args);    \
    }                                       \
    while(0)
#else
#define LJTALE_PRINT(LEVEL, args...)
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

/* for all the function calls, the size of the function pointer should
 * be fixed. Thus we use an index to retrieve each function */
enum {
    PLATFORM_GET_IRQ = 0,
    DEVM_KZALLOC,
    PLATFORM_GET_RESOURCE,
    DEVM_IOREMAP_RESOURCE,
    OF_MATCH_DEVICE,
    SPIN_LOCK_INIT,
    PLATFORM_SET_DRVDATA,
    PM_RUNTIME_ENABLE,
    PM_RUNTIME_DISABLE,
    /* pm runtime put/get methods could be replaced by central pm manager */
    PM_RUNTIME_GET,
    PM_RUNTIME_PUT,
    PM_RUNTIME_GET_SYNC,
    PM_RUNTIME_PUT_SYNC,
    CLK_GET,
    CLK_PUT,
    CLK_GET_RATE,
    CLK_SET_RATE,
    READW_RELAXED,
    WRITEW_RELAXED,
};

struct atomic_ops {
    int (*platform_get_irq)(struct platform_device *dev, unsigned int num);
    void *(*devm_kzalloc)(struct device *dev, size_t size, gfp_t gfp);
    struct resource *(*platform_get_resource)(struct platform_device *dev,
            unsigned int type, unsigned int num);
    void __iomem *(*devm_ioremap_resource)(struct device *dev,
                                           struct resource *res);
    const struct of_device_id *(*of_match_device)(
            const struct of_device_id *matches, const struct device *dev);
    /* FIXME: this is a macro defined in include/linux/spinlock.h */
    void (*spin_lock_init)(spinlock_t *lock);
    void (*platform_set_drvdata)(struct platform_device *dev, void *data);
    /* pm related ops */
    void (*pm_runtime_enable)(struct device *dev);
    void (*pm_runtime_disable)(struct device *dev);
    int (*pm_runtime_get)(struct device *dev);
    int (*pm_runtime_put)(struct device *dev);
    int (*pm_runtime_get_sync)(struct device *dev);
    int (*pm_runtime_put_sync)(struct device *dev);

    /* clk related ops */
    struct clk *(*clk_get)(struct device *dev, const char *id);
    unsigned long (*clk_get_rate)(struct clk *clk);
    int (*clk_set_rate)(struct clk *clk, unsigned long rate);
    void (*clk_put)(struct clk *clk);
    
    /* register access ops */
    u16 (*readw_relaxed)(void __iomem *addr);
    void (*writew_relaxed)(u16 val, void __iomem *addr);
};

#endif
