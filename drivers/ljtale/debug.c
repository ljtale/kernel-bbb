/*
 * Potentially helpful debug functions @ljtale
 */

#include <linux/universal-drv.h>

char *universal_req_type_str(enum universal_req_type type) {
    switch (type) {
        case REGACC:
            return "register accessor";
        case IRQ_CONFIG:
            return "irq config";
        case DMA_CONFIG:
            return "dma config";
        case PM_CONFIG:
            return "pm config";
        case CLOCK_CONFIG:
            return "clock config";
        default:
            return "invalid type or TODO";
    }
}
EXPORT_SYMBOL(universal_req_type_str);


extern struct list_head universal_drivers;
extern struct list_head universal_devices;

#ifdef LJTALE_DEBUG_ENABLE_LIST
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

