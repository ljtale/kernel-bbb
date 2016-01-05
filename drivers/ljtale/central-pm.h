#ifndef DRIVERS_LJTALE_CENTRAL_PM_
#define DRIVERS_LJTALE_CENTRAL_PM_

/* values that need to be written to I2C register when resuming */
struct i2c_resume_values {
    u16 omap_i2c_con_reset_all;
    u16 omap_i2c_psc_val;
    u16 omap_i2c_scll_val;
    u16 omap_i2c_sclh_val;
    u16 omap_i2c_con_we;
    u16 omap_i2c_con_en;
    u16 omap_i2c_ie_val;
};

/* values that need to be written or read when suspending */
struct i2c_suspend_values {
    u16 omap_i2c_ie_val;    /* this value needs to be read */
    u16 omap_i2c_v2_int_mask;   /* this value needs to be written */
    u16 omap_i2c_stat_val_w;  /* this value needs to be written */
    u16 omap_i2c_stat_val_r;    /* this value needs to be read */
};

/* this context should be on a per-device manner. In a driver that supports
 * multiple devices, this is not able to describe static context for a
 * certain device. Thus the values of the context should come in through
 * something like device tree. Device tree would be a good candidate to
 * process such values */
struct i2c_runtime_context {
    void __iomem *base;
    int reg_shift;
    struct i2c_resume_values resume;
    struct i2c_suspend_values suspend;
};

int
central_pm_omap_i2c_ctx(struct device *dev);

int
central_pm_omap_i2c_resume(struct device *dev);



#endif
