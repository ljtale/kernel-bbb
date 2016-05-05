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
 */
static struct list_head universal_drivers;

int __universal_drv_register(struct universal_drv *drv) {
    if (!drv) {
        LJTALE_MSG(KERN_ERR, "universal driver pointer null\n");
        return -ENODEV;
    }
    /* each driver should only have one instance of universal driver */
    list_add_tail(&drv->list, &universal_drivers);
    LJTALE_MSG(KERN_INFO, "universal driver register: %s\n", drv->name);
    return 0;
}
EXPORT_SYMBOL_GPL(__universal_drv_register);


/*
 * Pre-defined universal driver request type pointers to avoid dynamic
 * definition, which is a mess in some cases. Each pointer must be
 * reset after using for reuse
 * TODO: these pointers should be moved into corresponding request
 * service routine, then they will be defined on the kernel stack
 */
static struct universal_regmap_type *regmap_ptr = NULL;
static struct universal_devm_alloc_type *devm_alloc_ptr = NULL ;
static struct universal_of_node_match_type *of_node_match_ptr = NULL;

int __universal_drv_probe(struct universal_drv *drv) {
    int ret = -EINVAL;
    struct universal_request *request;
    int count = 0;
    
    if (!drv) {
        return -ENODEV;
    }
    LJTALE_MSG(KERN_INFO, "universal driver initialization: %s\n", drv->name);
    BUG_ON(drv->request_size < 0);

    while (count < drv->request_size) {
        request = &drv->requests[count];
        /* FIXME: for each of the request type, it may require separate
         * well-defined functions to handle them */
        switch (request->type) {
            case REGMAP_INIT:
                regmap_ptr = (struct universal_regmap_type *)request->data; 
                regmap_ptr->regmap = devm_regmap_init(drv->dev, 
                        regmap_ptr->regmap_bus, regmap_ptr->regmap_bus_context,
                        regmap_ptr->regmap_config);
                if (IS_ERR(regmap_ptr->regmap)) {
                    ret = PTR_ERR(regmap_ptr->regmap);
                    dev_err(drv->dev, 
                            "Failed to allocate register map: %d\n", ret);
                    goto err;
                }
                break;
            case DEVM_ALLOCATE:
                devm_alloc_ptr = 
                    (struct universal_devm_alloc_type *)request->data; 
                devm_alloc_ptr->ret_addr = 
                    devm_kzalloc(devm_alloc_ptr->dev, devm_alloc_ptr->size,
                            devm_alloc_ptr->gfp);
                if (!devm_alloc_ptr->ret_addr) {
                    ret = -ENOMEM;
                    goto err;
                }
                /* set the driver data, this should be generic for all
                 * the drivers */
                dev_set_drvdata(devm_alloc_ptr->dev, devm_alloc_ptr->ret_addr);
                break;
            case OF_NODE_MATCH:
                of_node_match_ptr = 
                    (struct universal_of_node_match_type *)request->data; 
                of_node_match_ptr->ret_match = 
                    of_match_device(of_node_match_ptr->matches,
                            of_node_match_ptr->dev);
                if (!of_node_match_ptr->ret_match) {
                    dev_err(of_node_match_ptr->dev, 
                            "Failed to find matching dt id\n");
                    ret = -EINVAL;
                    goto err;
                }
                break;
            default:
                goto err;
                break;
        }
    }
    return 0;
err:
    /* FIXME: revoke what has been done */
    LJTALE_MSG(KERN_INFO "universal driver initializaiton failed\n");
    return ret;
}
EXPORT_SYMBOL_GPL(__universal_drv_probe);

/* TODO: function calls to revoke what has been done by the probe function */


static int __init init_universal_driver(void) {
    INIT_LIST_HEAD(&universal_drivers);
    return 0;
}
pure_initcall(init_universal_driver);
