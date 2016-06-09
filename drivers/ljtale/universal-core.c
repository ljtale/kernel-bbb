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

/* check if there is a universal device for the existing struct device, also
 * check if there is a universal driver for that device. If both conditions
 * are true, return the universal device pointer, otherwise return NULL */
struct universal_device *check_universal_driver(struct device *dev) {
    struct universal_device *uni_dev = NULL;
    struct list_head *p;
    bool exist = false;
    list_for_each(p, &universal_devices) {
        uni_dev = list_entry(p, struct universal_device, dev_list);
        if (uni_dev->dev == dev && uni_dev->drv) {
            /* compare the address values directly, the device pointer
             * should be unique */
            LJTALE_MSG(KERN_INFO, "universal device %s and driver: %s exist\n",
                    uni_dev->name, uni_dev->drv->name);
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
    /* FIXME: probably should use devm for proper memory management? */
    uni_dev = kzalloc(sizeof(struct universal_device), GFP_KERNEL);
    if (!uni_dev) {
        return NULL;
    }
    /* the device possibly has not had a driver yet */
    /* temporarily I use the bus name to identify the universal device,
     * here the universal device name really doesn't matter?  */
    uni_dev->name = dev_name(dev);
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

static ssize_t i2c_eeprom_read(struct universal_device *uni_dev, char *buf,
        unsigned offset, size_t count) {
    struct i2c_client *client = to_i2c_client(uni_dev->dev);
    struct i2c_eeprom_client *eeprom_clients = &client->clients;
    struct i2c_msg msg[2];
    u8 msgbuf[2];
    unsigned long timeout, read_time;
    int i;
    ssize_t status;

    memset(msg, 0, sizeof(msg));
    if (eeprom_clients->flags & 0x80) {
        /* hardcoded the flag for 16-bit address pointer, i.e., 0x80 */
        i = offset >> 16;
        offset &= 0xffff;
    } else {
        i = offset >> 8;
        offset &= 0xff;
    }
    client = eeprom_clients->clients[i];
    if (count > eeprom_clients->io_limit)
        count = eeprom_clients->io_limit;
    /* no SMBUS will be used, we jump to i2c semantics directly */
    i = 0;
    if (eeprom_clients->flags & 0x80)
        /* little endian as here */
        msgbuf[i++] = offset >> 8;
    msgbuf[i++] = offset;

    msg[0].addr = client->addr;
    msg[0].buf = msgbuf;
    msg[0].len = i;

    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].buf = buf;
    msg[1].len = count;

    timeout = jiffies + msecs_to_jiffies(eeprom_clients->write_timeout);
    do {
        read_time = jiffies;
        /* assume i2c semantics by default */
        status = i2c_transfer(client->adapter, msg, 2);
        if (status == 2)
            status = count;
        dev_dbg(&client->dev, "read %zu@%d --> %d(%ld)\n",
                count, offset, status, jiffies);
        if (status == count)
            return count;
        msleep(1);
    } while (time_before(read_time, timeout));
    return -ETIMEDOUT;
}

static ssize_t eeprom_read(struct universal_device *uni_dev, char *buf,
    loff_t off, size_t count) {
    ssize_t ret = 0;
    ssize_t status;

    if (unlikely(!count))
        return count;
    mutex_lock(&uni_dev->lock);
    while (count) {
        status = i2c_eeprom_read(uni_dev, buf, off, count);
        if (status <= 0) {
            if (ret == 0)
                ret = status;
            break;
        }
        buf += status;
        off += status;
        count -= status;
        ret += status;
    }
    mutex_unlock(&uni_dev->lock);
    return ret;
}


int regmap_i2c_eeprom_read(void *context, const void *reg, size_t reg_size,
        void *val, size_t val_size) {
    /* the old version of eeprom regmap bus takes the i2c_client pointer
     * as the context passed to the regmap, however here I'll pass the
     * universal device pointer as the context */
    struct universal_device *uni_dev = context;
   struct register_accessor *regacc;
   unsigned int offset;
   int ret;

   if (uni_dev->drv)
       regacc = uni_dev->drv->regacc;
   BUG_ON(!regacc);
   BUG_ON(reg_size != 4);
   BUG_ON(regacc->reg_val_bits != 8);
   offset = __raw_readl(reg);

   ret = eeprom_read(uni_dev, val, offset, val_size);
   if (ret < 0)
       return ret;
   if (ret != val_size)
       return -EINVAL;
   return 0;
}

int regmap_i2c_eeprom_write(void *context, const void *data, size_t count) {
    
    return 0;
} 
