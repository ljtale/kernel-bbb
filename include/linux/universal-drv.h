#ifndef _LINUX_UNIVERSAL_DRV_H
#define _LINUX_UNIVERSAL_DRV_H

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/of_device.h>
#include <linux/interrupt.h>

#include <linux/i2c.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/platform_data/at24.h>

#include <linux/ljtale-utils.h>

struct universal_driver;
struct universal_device;

enum universal_req_type {
    /* potential usage as array index */
    /* REGMAP_INIT = 0, */
    REGMAP_INIT,
    DEVM_ALLOCATE,
    OF_NODE_MATCH,
    REQUEST_IRQ,
};

/* ====== activity based variables and data structures === */
extern struct regmap_bus i2c_regmap_bus;
extern struct regmap_bus i2c_eeprom_regmap_bus;
extern struct regmap_bus spi_regmap_bus;
/* FIXME: no spi eeprom regmap buses? */
extern struct regmap_bus spi_eeprom_regmap_bus;

enum regmap_buses {
    I2C_REGMAP_BUS,
    I2C_EEPROM_REGMAP_BUS,
    SPI_REGMAP_BUS,
    SPI_EEPROM_REGMAP_BUS,
};

/** =======================================================
 * Universal driver data structures exposed to conventional driver
 */

/*
 * register accessor data structures and auxiliary types
 */

typedef void (*regacc_lock)(void *);
typedef void (*regacc_unlock)(void *);
void regacc_lock_mutex(void *__dev);
void regacc_unlock_mutex(void *__dev);

struct register_accessor {
    const char *bus_name;   /* the bus to which the device is connected */
    int reg_addr_bits;
    int reg_val_bits;
    unsigned int max_register;
    /* synchronization primitives */
    regacc_lock lock;
    regacc_unlock unlock;
    void *lock_arg;

    /* TODO: possible a way to describe regsiter layout, one-dimentional or
     * two dimentional */
    // struct reg_layout layout;
    
    /* register accessing APIs, i.e., read/write etc.
     * We will assume generic read/write APIs */
    /* 
     * regacc_read: read from register reg and put the value in val
     * return 0 on success, return error code on failure */
    int (*regacc_read)(struct device *dev, unsigned int reg, unsigned int *val);
    /* 
     * regacc_write: write value in val to register reg
     * return 0 on success, return error code on failure */
    int (*regacc_write)(struct device *dev, unsigned int reg, unsigned int val); 
    /* TODO: more accessors, like set bits */

    /* ad-hoc fields */
    bool regmap_support;
    enum regmap_buses regmap_bus;
    struct regmap *regmap;
};


/*
 * irq configuration data structure and auxiliary types
 */

struct irq_config {
    /* irq number should be got dynamically from the universal driver. But
     * basically the irq is set statically in the device tree or through PCI
     * configuration, thus TODO: IRQ either comes from device tree node or
     * from the universal driver code compiler that compiles the device
     * knowledge into universal driver data structures. */
    int irq;
    irqreturn_t (*handler)(int irq, void *data);
    irqreturn_t (*thread_fn)(int irq, void *data);
    unsigned long irqflags;
    bool irq_sharing;
    /* indicate if we need to get gpio irq in case the normal irq is
     * not available */
    bool get_gpio_irq;
    /* irq could come from static platform information, or from device tree,
     * which we can get at runtime */
};


/*
 * iomap configuration data structure and auxiliary types
 */

struct iomap_config {
    /* legacy support for port IO or memory-mapped IO, default is MMIO */
    bool pio;
    /* depending on pio or mmio, the memory range could be got from platform
     * data or device tree, which we can get at runtime */
    /* if succeeds, iomap should return a virtual address
     * FIXME: the returned address should be used somehow within the unviersal
     * driver by the same data structure set for that device */
    void *virt_addr;
};

/*
 * DMA configuration data structure and auxiliary types
 * */

struct dma_config {
    /* TODO: what is needed for DMA configuration? */
};


/*
 * communication interfaces between the driver and the kernel
 * NOTE: this part probably is the most driver-dependent, which means this
 * part contains information that is pretty different accross drivers
 * e.g., tps has its interfaces to talk to the regulator driver and backlight
 * driver, eeprom has a nvmem framework on top the the register accessors
 * TODO: a generic way to manage this, like standarlized interfaces definition ?
 * */
struct drv_kernel_interface {
    /* ... */
}; 


#define UNIDRV_TYPE(activity) \
    struct universal_##activity_type

/*
 * universal driver struct 
 */

struct universal_driver {
    const char *name;
    /* the driver reference helps to use existing driver mechanisms such
     * as the device-driver matching mechanism */
    struct device_driver *driver;

    /* universal data structures, some of the information should be device
     * specific, theoretcially one universal device model should have
     * enough device information for the universal driver.
     * TODO: the fields of register accessors are divided into two parts, one
     * is universal driver specific such as the accessor methods function
     * pointers, the other is universal device specific such as device register
     * address bits and value bits */
    struct register_accessor *regacc;
    /* IRQ configuration */
    struct irq_config *irq_config;

    /* local data structrue that is only known to the conventional drivers */
    void *local_data;
    /* the universal driver provides a universal probe function for the 
     * device driver to call upon a device-driver binding, but there
     * are certain parts of the work that has to be done in the conventional
     * driver TODO: define a proper argument list */
    int (*local_probe)(struct universal_device *dev);

    /* Currently we assume each device will have a universal driver attached */
    struct list_head drv_list;
    /* like the normal device driver, one such universal driver data structure
     * is supposed to support one or more devices */
};

/* 
 * upon calling the probe function of universal driver, we will assume there is
 * a struct device instance already created, and this universal_device will need
 * to be created on a per-device base.
 * */
struct universal_device {
    const char *name;
    struct device *dev;

    struct universal_driver *drv;   
     /* lock protects against concurrent access to this device, used by
      * various purposes. Could add more locks. 
      * Could use spinlocks for fast I/O */
    struct mutex lock;

    /* Add the device to a global list for further reference */
    struct list_head dev_list;
};

/** =======================================================
 * Universal driver model methods
 */

 /* The registration function should be called from init calls */
extern int __universal_drv_register(struct universal_driver *drv);
#define universal_driver_register(drv) \
    __universal_drv_register(drv)

extern int __universal_drv_unregister(struct universal_driver *drv);
#define universal_driver_unregister(drv) \
    __universal_drv_unregister(drv)

extern int __universal_dev_register(struct universal_device *dev);
#define universal_device_register(dev) \
    __universal_dev_register(dev)

extern int __universal_dev_unregister(struct universal_device *dev);
#define universal_device_unregister \
    __universal_dev_unregister(dev)

extern int __universal_drv_probe(struct universal_device *dev);
#define universal_driver_probe(dev) \
    __universal_drv_probe(dev)

/** ==== universal driver core methods ==== */

/* check if there is a universal driver for the device, if so
 * return the universal device handle, otherwise return NULL */
struct universal_device *check_universal_driver(struct device *dev);
/* Create a universal device based on the existing device structure
 * This creation is bus-agnostic */
struct universal_device *new_universal_device(struct device *dev);



extern void debug_list_print(void);
/* regmap related functions */
extern struct regmap_bus *regmap_get_i2c_bus_general(void);
/* FIXME: this definition should be moved to individual i2c device drivers */
extern const struct i2c_device_id *i2c_match_id_general(
        const struct i2c_device_id *id, const struct i2c_client *client);
extern int regmap_i2c_eeprom_read(void *context, const void *reg,
       size_t reg_size, void *val, size_t val_size);
extern int regmap_i2c_eeprom_write(void *context, const void *data,
        size_t val_size);
extern int regmap_i2c_eeprom_gather_write(void *context, const void *reg,
                    size_t reg_size, const void *val, size_t val_size);
extern void _populate_regmap_config(struct register_accessor *regacc,
        struct regmap_config *config);
extern struct regmap_bus *_choose_regmap_bus(struct register_accessor *regacc);

/* IRQ configuration related functions */
extern int __universal_get_irq(struct universal_device *uni_dev);


/* TODO: debugfs support for universal driver debugging */
char *universal_req_type_str (enum universal_req_type type);

#endif /* _LINUX_UNIVERSAL_DRV_H */

