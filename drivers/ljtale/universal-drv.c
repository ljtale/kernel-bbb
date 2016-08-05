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
#include <linux/err.h>

#include <linux/universal-drv.h>

/* 
 * global list to maintain driver childern of the universal driver
 * this is not actually a list of drivers, but a list of data structures
 * to store the device knowledge that drives the universal driver to do 
 * things for the device
 */
struct list_head universal_drivers;
EXPORT_SYMBOL(universal_drivers);

/*
 * global list to maintian device defined as univeral_dev
 * */
struct list_head universal_devices;
EXPORT_SYMBOL(universal_devices);

struct regmap_bus i2c_regmap_bus;
struct regmap_bus i2c_eeprom_regmap_bus;
struct regmap_bus spi_regmap_bus;
struct regmap_bus spi_eeprom_regmap_bus;

int __universal_drv_register(struct universal_driver *drv) {
    struct universal_device *dev;
    struct list_head *p;
    struct universal_rpm *rpm = &drv->rpm;
    struct universal_rpm_ops *rpm_ops = &drv->rpm_ops;
    int ret;
    if (!drv) {
        LJTALE_MSG(KERN_ERR, "universal driver pointer null\n");
        return -ENODEV;
    }
    /* each driver should only have one instance of universal driver */
    list_add_tail(&drv->drv_list, &universal_drivers);
    LJTALE_MSG(KERN_INFO, "universal driver register: %s\n", drv->name);
    debug_list_print();
    /* bind the universal driver data to the universal device */
    /* FIXME: we need a universal-driver-specific matching mechanism
     * Maybe also involves an id talbe just like the device tree, this
     * should be relatively easier than other parts of the driver  */
    list_for_each(p, &universal_devices) {
        dev = list_entry(p, struct universal_device, dev_list);
        if (dev->dev->bus)
            if (dev->dev->bus->match && !dev->drv) {
                ret = dev->dev->bus->match(dev->dev, drv->driver);
                if (ret) {
                    dev->drv = drv;
                    /* useless continue line */
                    continue;
                }
            }
    }
    /* if there is no match for this universal driver, fine, just exit */
    /* power management setup */
    
    spin_lock_init(&rpm->rpm_graph_lock);
    if (rpm_ops->rpm_graph_build)
        rpm_ops->rpm_graph_build();

    return 0;
}
EXPORT_SYMBOL(__universal_drv_register);


int __universal_drv_unregister(struct universal_driver *drv) {
    return 0;
}
EXPORT_SYMBOL(__universal_drv_unregister);

static int universal_regacc_config(struct universal_device *uni_dev, 
        struct register_accessor *regacc) {
    struct regmap_config universal_regmap_config;
    const struct regmap_bus *regmap_bus;
    struct device *dev = uni_dev->dev;
    struct platform_device *pdev;
    struct regacc_dev *regacc_dev = &uni_dev->probe_dev.regacc_dev;
    int ret;
    if (regacc->regmap_support) {
        LJTALE_LEVEL_DEBUG(2, "regmap config...%s\n", uni_dev->name);
        _populate_regmap_config(regacc, &universal_regmap_config);
        regmap_bus = _choose_regmap_bus(regacc);
        BUG_ON(!regmap_bus);
        regacc_dev->regmap = devm_regmap_init(dev, regmap_bus, dev, 
                &universal_regmap_config);
        if (IS_ERR(regacc_dev->regmap)) {
            LJTALE_LEVEL_DEBUG(2, "regmap init failed...%s\n",uni_dev->name);
            ret = PTR_ERR(regacc_dev->regmap);
            return ret;
        }
     } else {
         /* first request a memory resource */
         struct resource *res;
         /* FIXME: Assume the memory-mapped I/O is a platform device feature */
         LJTALE_LEVEL_DEBUG(2, "mmio regacc config...%s\n", uni_dev->name);
         /* the device must be platform device */
         BUG_ON(strcmp("platform", regacc->bus_name));
         pdev = to_platform_device(dev);
         res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
         regacc_dev->base = devm_ioremap_resource(dev, res);
         if (IS_ERR(regacc_dev->base))
             return PTR_ERR(regacc_dev->base);
         else
             regacc_dev->base += regacc->reg_offset;
         regacc_dev->phys_base = res->start + regacc->reg_offset;
         /* universal read/write will need to use this base address */
     }
    return 0;
}


static int universal_irq_config(struct universal_device *uni_dev,
        struct irq_config *irq_config, int index) {
    int ret;
    LJTALE_LEVEL_DEBUG(2, "IRQ config...%s -- %d\n", uni_dev->name, index);
    /* first try to get irq number from device tree or whatever */
    irq_config->irq = __universal_get_irq(uni_dev, irq_config);
    if (irq_config->irq <= 0) {
        ret = irq_config->irq;
        dev_err(uni_dev->dev, "universal irq unavailable\n");
        return ret;
    }
    LJTALE_LEVEL_DEBUG(2, "device %s gets IRQ: %d\n", 
            uni_dev->name, irq_config->irq);
    /* if the irq is good, config the irq with interrupt handlers */
    ret = devm_request_threaded_irq(uni_dev->dev,
            irq_config->irq, irq_config->handler, irq_config->thread_fn,
            irq_config->irq_flags, uni_dev->name, uni_dev->dev);
    if (ret != 0) {
        dev_err(uni_dev->dev, "universal probe failed to request IRQ %d\n",
                irq_config->irq);
        return ret;
    }
    /* any post irq configuration things to do */
    if (irq_config->post_irq_config) {
        ret = irq_config->post_irq_config(uni_dev);
        if (ret != 0)
            return ret;
    }
    return 0;
}

static int universal_dma_config(struct universal_device *uni_dev,
        struct dma_config *dma_config, int index) {
    struct dma_config_dev_num *dma_config_dev_num = 
        &uni_dev->probe_dev.dma_config_dev_num;
    dma_cap_mask_t mask;
    unsigned fn_param;
    struct device_node *nd = uni_dev->dev->of_node;
    struct resource *res;
    struct dma_chan *chan;
    LJTALE_LEVEL_DEBUG(2, "DMA config...%s -- %d\n", uni_dev->name, index);
    if (nd) 
        fn_param = -1;
    else {
        /* get the platform resource by name */
        struct platform_device *pdev = to_platform_device(uni_dev->dev);
        BUG_ON(!pdev);
        res = platform_get_resource_byname(pdev, IORESOURCE_DMA,
                dma_config->dma_name);
        if (!res) {
            dev_err(uni_dev->dev, "cannot get DMA %s channel\n",
                    dma_config->dma_name);
            return -EINVAL;
        }
        fn_param = res->start;
    }

    dma_cap_zero(mask);
    dma_cap_set(dma_config->tx_type, mask);

    chan = dma_request_slave_channel_compat_reason(mask,
                dma_config->dma_filter_fn, &fn_param, uni_dev->dev,
                dma_config->dma_name);
    if (IS_ERR(chan)) {
        dev_err(uni_dev->dev, "unable to obtain DMA %s channel %u\n",
                dma_config->dma_name, fn_param);
        return PTR_ERR(chan);
    }
    dma_config_dev_num->dma_config_dev[index].channel = chan;
    return 0;
}

/* populate device knowledge from device tree for serving runtime pm */
void inline rpm_knowledge_from_dt(struct universal_device *uni_dev) {
    struct universal_rpm_dev *rpm_dev = &uni_dev->rpm_dev;
    struct device_node *of_node = uni_dev->dev->of_node;
    BUG_ON(!of_node);
    if (of_property_read_bool(of_node, "support_irq"))
        rpm_dev->support_irq = true;
    else
        rpm_dev->support_irq = false;
    if (of_property_read_bool(of_node, "irq_need_lock")) {
        rpm_dev->irq_need_lock = true;
        spin_lock_init(&rpm_dev->irq_lock);
        LJTALE_LEVEL_DEBUG(4 ,"IRQ need lock read bool true: %s\n",
                uni_dev->name);
    } else
        rpm_dev->irq_need_lock = false;
    if (of_property_read_bool(of_node, "dmas")) {
        rpm_dev->support_dma = true;
        LJTALE_LEVEL_DEBUG(4, "DMA is supported: %s\n", uni_dev->name);
    }
    else
        rpm_dev->support_dma = false;
}
 
int __universal_drv_probe(struct universal_device *dev) {
    int ret = 0;
    struct universal_driver *drv;
    struct universal_probe_dev *probe_dev;
    struct universal_rpm_dev *rpm_dev;
    struct register_accessor *regacc;
    struct irq_config_num *irq_config_num;
    struct dma_config_num *dma_config_num;
    struct universal_rpm_ops *rpm_ops;
    int i;
    
    if (!dev || !dev->drv) {
        LJTALE_MSG(KERN_WARNING, 
                "no universal driver for universal device: %s\n", dev->name);
        return -ENODEV;
    }
    LJTALE_MSG(KERN_INFO, "universal driver probe: %s\n", dev->name);
    drv = dev->drv;
    probe_dev = &dev->probe_dev;
    rpm_dev = &dev->rpm_dev;
    /* do a set of initialization */
    mutex_init(&dev->probe_dev.lock);
    spin_lock_init(&dev->probe_dev.spinlock);

    /* do a series of universal driver probe */
    /* for register accessors */
    /* Initialize the register accessors for the device. If the device uses
     * regmap support, instantiate a regmap instance for the device, otherwise
     * instantiate a universal register accessor for the device.*/
    regacc = drv->regacc;
    if (regacc) {
        ret = universal_regacc_config(dev, regacc);
        if (ret < 0)
            goto regacc_err;
    }

    dma_config_num = drv->dma_config_num;
    if (dma_config_num) {
        struct dma_config_dev_num *dma_config_dev_num = 
            &probe_dev->dma_config_dev_num;
        dma_config_dev_num->dma_config_dev = devm_kzalloc(dev->dev,
                sizeof(struct dma_config_dev) * dma_config_num->dma_num, 
                GFP_KERNEL);
        if (!dma_config_dev_num->dma_config_dev) {
            LJTALE_MSG(KERN_ERR, "dma config dev allocation failed\n");
            return -ENOMEM;
        }
        dma_config_dev_num->dma_num = dma_config_num->dma_num;
        for (i = 0; i < dma_config_num->dma_num; i++) {
            ret = universal_dma_config(dev, &dma_config_num->dma_config[i], i);
            if (ret < 0)
                goto dma_config_err;
        }
        rpm_dev->dma_channel_requested = true;
    }

    /* TODO: runtime pm configuration and clock configuration */
    /* get properties from device tree to populate the device knowledge */
    if (dev->dev->of_node) {
        rpm_knowledge_from_dt(dev);
    } else {
        /* FIXME: we assume there is a device tree node for the device
         * in this prototype */
        LJTALE_LEVEL_DEBUG(1, "device: %s does not have a device tree node\n",
                dev->name);
        return 0;
    }
    rpm_dev->first_resume_called = false;
    rpm_dev->context_loss_cnt = 0;
    rpm_ops = &drv->rpm_ops;
    if (rpm_ops->rpm_create_reg_context) {
        ret = rpm_ops->rpm_create_reg_context(dev);
        if (ret < 0)
            goto rpm_error;
    }

    /* ... */
    /* do a local probe */
    if (drv->local_probe) {
        ret = drv->local_probe(dev);
        if (ret < 0)
            goto local_probe_err;
    }

    irq_config_num = drv->irq_config_num;
    if (irq_config_num) {
        struct irq_config_dev_num *irq_config_dev_num = 
            &probe_dev->irq_config_dev_num;
        irq_config_dev_num->irq_value = devm_kzalloc(dev->dev,
                sizeof(int) * irq_config_num->irq_num, GFP_KERNEL);
        if (!irq_config_dev_num->irq_value)
            return -ENOMEM;
        irq_config_dev_num->irq_num = irq_config_num->irq_num;
        for (i = 0; i < irq_config_num->irq_num; i++) {
            /* It is the responsibility of the device knowledge provider to
             * make sure that the provided IRQ numbers are unique, otherwise
             * there will be an error when requesting an interrupt line */
            ret = universal_irq_config(dev, &irq_config_num->irq_config[i], i);
            if (ret != 0)
                goto irq_config_err;
            irq_config_dev_num->irq_value[i] =
                irq_config_num->irq_config[i].irq;
        }
    }

    LJTALE_MSG(KERN_INFO, "universal probe done: %s -> %d\n", dev->name, ret);
    return ret;
    
    /* TODO: error handling */
irq_config_err:
local_probe_err:
rpm_error:
dma_config_err:
regacc_err:
    return ret;
}
EXPORT_SYMBOL(__universal_drv_probe);


int __universal_dev_register(struct universal_device *dev) {
    struct universal_driver *drv;
    struct list_head *p;
    int ret;
    if (!dev) {
        LJTALE_MSG(KERN_ERR, "universal device pointer null\n");
        return -ENODEV;
    }
    /* temporarily the regisration only adds the device to the device list
     * TODO: more registration information needed here later */
    list_add_tail(&dev->dev_list, &universal_devices);
    debug_list_print();
    /* bind the universal driver data to the universal driver device */
    if (dev->dev->bus) {
        if(dev->dev->bus->match) {
           list_for_each(p, &universal_drivers) {
               drv = list_entry(p, struct universal_driver, drv_list);
               ret = dev->dev->bus->match(dev->dev, drv->driver);
               if (ret) {
                   dev->drv = drv;
                   break;
               }
           }
           if (!dev->drv) {
               LJTALE_MSG(KERN_WARNING, 
                       "universal device: %s does not find a " 
                       "universal driver data to match\n", dev->name);
           }
        }
    } 
    else
        return -ENODEV;
    LJTALE_MSG(KERN_INFO, "universal device registered: %s\n", dev->name);
    return 0;
}
EXPORT_SYMBOL(__universal_dev_register);

int __universal_dev_unregister(struct universal_device *dev) {
    return 0;
}
EXPORT_SYMBOL(__universal_dev_unregister);


/* TODO: function calls to revoke what has been done by the probe function */


static int __init init_universal_driver(void) {
//    struct regmap_config *regmap_config_temp;
    INIT_LIST_HEAD(&universal_drivers);
    INIT_LIST_HEAD(&universal_devices);
    /* for register accessors, initialize regmap buses and normal load/store
     * accessors */
    i2c_regmap_bus = *regmap_get_i2c_bus_general();
    /* build a regmap bus for i2c eeprom device according to the at24 driver */
    i2c_eeprom_regmap_bus.read = regmap_i2c_eeprom_read;
    i2c_eeprom_regmap_bus.write = regmap_i2c_eeprom_write;
    i2c_eeprom_regmap_bus.gather_write = regmap_i2c_eeprom_gather_write;
    i2c_eeprom_regmap_bus.reg_format_endian_default = REGMAP_ENDIAN_NATIVE;
    i2c_eeprom_regmap_bus.val_format_endian_default = REGMAP_ENDIAN_NATIVE;

    return 0;
}
pure_initcall(init_universal_driver);
