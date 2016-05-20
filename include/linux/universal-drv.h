#ifndef _LINUX_UNIVERSAL_DRV_H
#define _LINUX_UNIVERSAL_DRV_H

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/of_device.h>
#include <linux/interrupt.h>

#include <linux/ljtale-utils.h>

enum universal_req_type {
    /* potential usage as array index */
    /* REGMAP_INIT = 0, */
    REGMAP_INIT,
    DEVM_ALLOCATE,
    OF_NODE_MATCH,
    REQUEST_IRQ,
};

/*
 * Corresponding to different request types, there should be a data structure
 * associate with the data pointer */
struct universal_regmap_type {
    char *name;
    struct device *dev;
    struct regmap_bus *regmap_bus;
    void *regmap_bus_context;
    struct regmap_config *regmap_config;
    struct regmap *regmap;
};

struct universal_devm_alloc_type {
    char *name;
    /* the device pointer for the allocated memory to be attached to*/
    struct device *dev;
    size_t size;
    gfp_t gfp;
    void *ret_addr;
    /* indicate if this data needs to be added to the driver data */
    bool drv_data_flag :1;
    /* Usually the allocated memory needs to be populated, we provide a 
     * function pointer to point back to the conventional driver to do
     * the population.
     * FIXME: this is a temporary approach of proof-of-concept, later we
     * need to think about separating the conventional drivers and the
     * universal driver, providing a clean interface between them instead of
     * using C pointers. A similar problem is also exposed to other types. */
    int (*populate)(struct universal_devm_alloc_type *ptr);
};

struct universal_of_node_match_type {
    char *name;
    struct of_device_id *matches;
    struct device *dev;
    struct of_device_id *ret_match;
};

/* TODO: distinguish threaded and non-threaded irq request */
struct universal_request_irq_type {
    char *name;
    struct device *dev;
    unsigned int irq;
    irq_handler_t handler;
    irq_handler_t thread_fn;
    unsigned long irqflags;
    const char *devname;
    void *dev_id;
    int ret_irq;
    int (*post_process)(struct universal_request_irq_type *ptr);
};

struct universal_request {
    enum universal_req_type type;
    /* FIXME: potentially type unsafe */
    void *data;
};

#define UNIDRV_TYPE(activity) \
    struct universal_##activity_type

/*
 * universal driver struct 
 */

struct universal_drv {
    const char *name;
    /*
     * As a generic interface for all the devices, the universal driver should
     * use a device reference that is generic for all the devices. Since the
     * universal driver could potentially maintain device states, this pointer
     * is used to uniquely identify universal drivers
     */
    /* FIXME: since the universal driver is for a specific device, this pointer
     * should replace any one that is used in specific activity type */
    // struct device *dev;
    /* 
     * The universal driver probe function could take a list of requests
     * from conventional drivers and do them according to the order specified
     */
    struct universal_request *requests;
    int request_size;
    /* Local data for the conventional driver */
    void *local_data;
    
    /* the universal driver provides a universal probe function for the 
     * device driver to call upon a device-driver binding, but there
     * are certain parts of the work that has to be done in the conventional
     * driver TODO: define a proper argument list */
    int (*local_probe)(struct universal_drv *drv);

    /* Currently we assume each device will have a universal driver attached */
    struct list_head list;
};

 /* The registration function should be called from init calls */
extern int __universal_drv_register(struct universal_drv *drv);
#define universal_drv_register(drv) \
    __universal_drv_register(drv)

extern int __universal_drv_probe(struct universal_drv *drv);
#define universal_drv_init(drv) \
    __universal_drv_probe(drv)

/* TODO: debugfs support for universal driver debugging */

char *universal_req_type_str (enum universal_req_type type);

#endif

