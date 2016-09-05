/**
 * universal system-wide PM source file @ljtale
 */

#include <linux/universal-utils.h>
#include <linux/universal-pm.h>


int universal_pm_create_reg_context(struct universal_device *uni_dev) {
    struct universal_pm *pm = &uni_dev->drv->pm;
    u32 *array = pm->ref_ctx.array;
    int size = pm->ref_ctx.size;
    int i;
    u32 *dev_array;
    if (!array)
        return 0;
    dev_array = devm_kzalloc(uni_dev->dev, sizeof(u32) * size, GFP_KERNEL);
    if (!dev_array)
        return -ENOMEM;
    /* copy everthing including the constant values */
    for (i = 0; i < size; i++)
        dev_array[i] = array[i];

    uni_dev->pm_dev.pm_context.array = dev_array;
    uni_dev->pm_dev.pm_context.size = size;
    LJTALE_LEVEL_DEBUG(4, "%s created PM reg context array\n", uni_dev->name);
    return 0;
}

static int universal_pm_pin_control(struct universal_device *uni_dev,
        enum pm_action action) {
    struct universal_pm *pm = &uni_dev->drv->pm;
    struct universal_pin_control *pin_control = pm->pin_control;
    enum pm_device_call pin_state = PM_PINCTRL_DEFAULT;
    if (!pin_control)
        return 0;
    switch(action) {
        case SUSPEND:
            pin_state = pin_control->suspend_state;
            break;
        case RESUME:
            pin_state = pin_control->resume_state;
        case SUSPEND_NOIRQ:
            pin_state = pin_control->suspend_noirq_state;
            break;
        case RESUME_NOIRQ:
            pin_state = pin_control->resume_noirq_state;
            break;
        /* TODO: more system PM actions */
        default:
            break;
    }
    switch(pin_state) {
        case PM_PINCTRL_DEFAULT:
            pinctrl_pm_select_default_state(uni_dev->dev);
            break;
        case PM_PINCTRL_SLEEP:
            pinctrl_pm_select_sleep_state(uni_dev->dev);
            break;
        case PM_PINCTRL_IDLE:
            pinctrl_pm_select_idle_state(uni_dev->dev);
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static int universal_pm_save_context(struct universal_device *uni_dev) {
    return 0;
};


static int universal_pm_restore_context(struct universal_device *uni_dev) {
    int ret = 0;
    struct universal_pm *pm = &uni_dev->drv->pm;
    struct universal_restore_context *restore_context = &pm->restore_context;
    struct universal_restore_context_tbl *tbl = 
        restore_context->restore_tbl;
    /* for system PM, usually no context loss will be counted, directly
     * restore context from the restore table */
    if (!tbl)
        return 0;
    ret = process_reg_table(uni_dev, tbl->table, tbl->table_size);
    if (ret)
        return ret;
    if (restore_context->pm_local_restore_context)
        ret = restore_context->pm_local_restore_context(uni_dev);
    return ret;
}

static int universal_del_timer_sync(struct universal_device *uni_dev) {
    return 0;
}

int universal_suspend(struct device *dev) {
    struct universal_device *uni_dev;
    struct universal_pm_dev *pm_dev;
    struct universal_pm_ops *pm_ops;
    int ret = 0;
    uni_dev = check_universal_driver(dev);
    if (!uni_dev) {
        LJTALE_MSG(KERN_ERR, "no universal driver for: %s\n", dev_name(dev));
        return ret;
    }
    pm_dev = &uni_dev->pm_dev;
    pm_ops = &uni_dev->drv->pm_ops;
    LJTALE_LEVEL_DEBUG(2, "universal system suspend...%s\n", uni_dev->name);

    /* determine if the rpm is enabled for this device */
    if (pm_runtime_enabled(dev))
        /* bring up the device if it is runtime suspended */
        pm_runtime_get_sync(dev);
    /* save context */
    ret = universal_pm_save_context(uni_dev);
    if (ret) {
        LJTALE_LEVEL_DEBUG(3, "universal PM save context failed: %d\n", ret);
        goto lock_err;
    }


    if (pm_ops->local_suspend)
        ret = pm_ops->local_suspend(dev);
    /* disable clock */
    ret = universal_disable_clk(uni_dev);
    if (ret)
        goto lock_err;
    /* deactive timer and wait for the hanlder to finish */
    ret = universal_del_timer_sync(uni_dev);
    if (ret)
        goto lock_err;

    universal_pm_pin_control(uni_dev, SUSPEND);
    /* drop the reference */
    if (pm_runtime_enabled(dev))
        pm_runtime_put_sync(dev);
    return ret;
/* TODO: device lock */
lock_err:
    return ret;
}

int universal_resume(struct device *dev) {
    struct universal_device *uni_dev;
    struct universal_pm_dev *pm_dev;
    struct universal_pm_ops *pm_ops;
    int ret = 0;
    uni_dev = check_universal_driver(dev);
    if (!uni_dev) {
        LJTALE_MSG(KERN_ERR, "no universal driver for: %s\n", dev_name(dev));
        return ret;
    }
    pm_dev = &uni_dev->pm_dev;
    pm_ops = &uni_dev->drv->pm_ops;
    LJTALE_LEVEL_DEBUG(2, "universal system resume...%s\n", uni_dev->name);

    /* probably rely on the runtime resume to restore the context */
    if (pm_runtime_enabled(dev))
        pm_runtime_get_sync(dev);

    /* select pin states at first */
    universal_pm_pin_control(uni_dev, RESUME);

    /* enable clock */
    ret = universal_enable_clk(uni_dev);
    if (ret)
       goto lock_err;

    /* restore the context */
    ret = universal_pm_restore_context(uni_dev);
    if (ret) {
        LJTALE_LEVEL_DEBUG(3, "universal PM restore context failed: %d\n", ret);
        goto lock_err;
    }

    if (pm_ops->local_resume)
        ret = pm_ops->local_resume(dev);

    /* lastly mark the device last busy and allow defered runtime suspend */
    if (pm_runtime_enabled(dev)) {
        pm_runtime_mark_last_busy(dev);
        pm_runtime_put_autosuspend(dev);
    }
    return ret;
lock_err:
    return ret;
}
