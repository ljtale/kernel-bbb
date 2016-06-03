/*
 * Potentially helpful debug functions @ljtale
 */

#include <linux/universal-drv.h>

char *universal_req_type_str(enum universal_req_type type) {
    switch (type) {
        case REGMAP_INIT:
            return "regmap init";
        case DEVM_ALLOCATE:
            return "devm allocation";
        case OF_NODE_MATCH:
            return "of node match";
        default:
            return "invalid type";
    }
}
EXPORT_SYMBOL(universal_req_type_str);


extern struct list_head universal_drivers;
extern struct list_head universal_devices;

#ifdef LJTALE_DEBUG_ENABLE
void debug_list_print(void) {
    struct list_head *p;
    struct universal_device *dev;
    struct universal_driver *drv;
    int i = 0;
    LJTALE_MSG(KERN_INFO, "current universal device and driver list: \n");
    list_for_each(p, &universal_devices) {
        dev = list_entry(p, struct universal_device, dev_list);
        LJTALE_MSG(KERN_INFO, "universal_device: %s_%d\n", dev->name, i++);
    }
    i = 0;
    list_for_each(p, &universal_drivers) {
        drv = list_entry(p, struct universal_driver, drv_list);
        LJTALE_MSG(KERN_INFO, "universal_driver: %s_%d\n", drv->name, i++);
    }
    LJTALE_MSG(KERN_INFO, "current universal device and driver list end.\n");
}
#else
void debug_list_print(void) {
}
#endif
EXPORT_SYMBOL(debug_list_print);

