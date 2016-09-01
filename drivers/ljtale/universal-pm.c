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

    if (pm_ops->local_suspend)
        ret = pm_ops->local_suspend(dev);

    /* drop the reference */
    if (pm_runtime_enabled(dev))
        pm_runtime_put_sync(dev);
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

    /* select pin states at first */

    /* probably rely on the runtime resume to restore the context */
    if (pm_runtime_enabled(dev))
        pm_runtime_get_sync(dev);

    if (pm_ops->local_resume)
        ret = pm_ops->local_resume(dev);

    /* lastly mark the device last busy and allow defered runtime suspend */
    if (pm_runtime_enabled(dev)) {
        pm_runtime_mark_last_busy(dev);
        pm_runtime_put_autosuspend(dev);
    }
    return ret;
}
