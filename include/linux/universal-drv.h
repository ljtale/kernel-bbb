#ifndef _LINUX_UNIVERSAL_DRV_H
#define _LINUX_UNIVERSAL_DRV_H

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/of_device.h>

#include <linux/ljtale-utils.h>

enum universal_req_type {
    /* potential usage as array index */
    /* REGMAP_INIT = 0, */
    REGMAP_INIT,
    DEVM_ALLOCATE,
    OF_NODE_MATCH,
};

/*
 * Corresponding to different request types, there should be a data structure
 * associate with the data pointer */
struct universal_regmap_type {
    struct regmap_bus *regmap_bus;
    struct regmap_config *regmap_config;
    void *regmap_bus_context;
    struct regmap *regmap;
};

struct universal_devm_alloc_type {
    /* the device pointer for the allocated memory to be attached to*/
    struct device *dev;
    size_t size;
    gfp_t gfp;
    void *ret_addr;
};

struct universal_of_node_match_type {
    struct of_device_id *matches;
    struct device *dev;
    struct of_device_id *ret_match;
};

struct universal_request {
    enum universal_req_type type;
    /* FIXME: potentially type unsafe */
    void *data;
};

/*
 * universal driver configuration struct 
 */

struct universal_drv_config {
    const char *name;
    struct regmap_bus *regmap_bus;
    void *regmap_bus_context;
    struct regmap_config *regmap_config;
    struct regmap *regmap;
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
    struct device *dev;
    struct universal_drv_config config;
    /* 
     * The universal driver probe function could take a list of requests
     * from conventional drivers and do them according to the order specified
     */
    struct universal_request *requests;
    int request_size;
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

