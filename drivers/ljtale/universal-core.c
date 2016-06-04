/*
 * universal driver core source file @ljtale
 * */

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
#include <linux/mutex.h>
#include <linux/slab.h>

#include <linux/universal-drv.h>

extern struct list_head universal_drivers;
extern struct list_head universal_devices;

void regacc_lock_mutex(void *__dev)
{
    struct universal_device *dev = __dev;
    mutex_lock(&dev->lock);
}

void regacc_unlock_mutex(void *__dev)
{
    struct universal_device *dev = __dev;
    mutex_unlock(&dev->lock);
}

struct universal_device *check_universal_driver(struct device *dev) {
    struct universal_device *uni_dev = NULL;
    struct list_head *p;
    bool exist = false;
    list_for_each(p, &universal_devices) {
        uni_dev = list_entry(p, struct universal_device, dev_list);
        if (uni_dev->dev == dev) {
            /* compare the address values directly, the device pointer
             * should be unique */
            exist = true;
            break;
        }
    }
    /* didn't find any match */
    return (exist ? uni_dev : NULL);
}
EXPORT_SYMBOL(check_universal_driver);


struct universal_device *new_universal_device(struct device *dev) {
    struct universal_device *uni_dev;
    uni_dev = kzalloc(sizeof(struct universal_device), GFP_KERNEL);
    if (!uni_dev) {
        return NULL;
    }
    /* the device possibly has not had a driver yet */
    /* temporarily I use the bus name to identify the universal device,
     * here the universal device name really doesn't matter?  */
    uni_dev->name = dev->init_name;
    uni_dev->dev = dev;
    /* TODO: more initialization */
    uni_dev->drv = NULL;
    return uni_dev;
}
EXPORT_SYMBOL(new_universal_device);

/* For register accessors, there are two types of existing mechanism:
 * 1) The regmap framework provides a unified accessor interfaces for mfds,
 * such as the master-slave model buses, i2c or spi. Our hypothesis is drivers
 * for i2c or spi devicew will share the same set of regmap accessors. That
 * means we can build several fixed regmap buses, device vendors will only
 * need to tell the universal driver what it wish to use
 * 2) The load/store instruction set. For memory mapped I/O, device registers
 * can be read/written by load/store instructions. Only the
 * architecture-specific implementation of read/write would be different.
 * Currently in my prototype, I'll first build regmap framework, then we can
 * build a regmap-ish framework for memory mapped I/O */


static ssize_t i2c_eeprom_read(char *buf, unsigned offset, size_t count) {
    struct i2c_msg msg[2];
    u8 msgbuf[2];
    struct i2c_client *client;
    unsigned long timeout, read_time;
    int status, i;

    memset(msg, 0, sizeof(msg));

    /* copy comment from drivers/misc/eeprom/at24.c */
	/*
	 * REVISIT some multi-address chips don't rollover page reads to
	 * the next slave address, so we may need to truncate the count.
	 * Those chips might need another quirk flag.
	 *
	 * If the real hardware used four adjacent 24c02 chips and that
	 * were misconfigured as one 24c08, that would be a similar effect:
	 * one "eeprom" file not four, but larger reads would fail when
	 * they crossed certain pages.
	 */

	/*
	 * Slave address and byte offset derive from the offset. Always
	 * set the byte address; on a multi-master board, another master
	 * may have changed the chip's "current" address pointer.
	 */





    return 0;
}


/* generic i2c eeprom read function for creating regmap bus
 * parameter guessed by ljtale
 * reg: the register address
 * */
int regmap_i2c_eeprom_read(void *context, const void *reg, size_t reg_size,
        void *val, size_t val_size) {
    /* different from the at24 drive, we are working for universal driver,
     * so we should have an agreement on what the context is with the 
     * universal driver. Here I used universal_device */
    struct universal_device *dev = context;
    struct universal_driver *drv = dev->drv;
    char *buf = val;
    unsigned int offset;
    int ret = 0;
    
    BUG_ON(!drv);
    BUG_ON(reg_size != 4);
    BUG_ON(drv->regacc->reg_val_bits != 8);
    offset = __raw_readl(reg);
    /* val bytes is always 1 */
    if (unlikely(!val_size))
        return val_size;
    mutex_lock(&dev->lock);
    while (val_size) {
        ssize_t status;
        // assume I have a read function
        status = 0;
        if (status <= 0) {
            if (ret == 0)
                ret = status;
            break;
        }
        buf += status;
        offset += status;
        val_size -= status;
        ret += status;
    }
    mutex_unlock(&dev->lock);
    if (ret < 0)
        return ret;
    if (ret != val_size)
        return -EINVAL;
    return 0;
}



