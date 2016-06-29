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
#include <linux/of_gpio.h>
#include <linux/pm_runtime.h>
#include <linux/pm_domain.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#include <linux/universal-drv.h>
#include <linux/universal-utils.h>

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
            LJTALE_LEVEL_DEBUG(5, "universal device %s and driver: %s exist\n",
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
        /* big endian as here */
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
    /* The old version of eeprom regmap bus takes the i2c_client pointer
     * as the context passed to the regmap, however here I'll pass the
     * conventional device pointer as the context to be compatible with
     * the generic i2c regmap bus. Doing this makes it possible to reuse
     * regmap initialization code */
    struct device *dev = context;
    struct universal_device *uni_dev;
    struct register_accessor *regacc = NULL;
    unsigned int offset;
    int ret;
    
    uni_dev = check_universal_driver(dev);
    if (!uni_dev) {
        LJTALE_MSG(KERN_ERR, "universal driver unavailable for device: %s\n",
                dev_name(dev));
        return -EINVAL;
    }
    if (uni_dev->drv)
        regacc = uni_dev->drv->regacc;

    BUG_ON(!regacc);
    BUG_ON(reg_size != 4);
    if (regacc)
        BUG_ON(regacc->reg_val_bits != 8);
    else
        return -EINVAL;
 
    offset = __raw_readl(reg);
  
    ret = eeprom_read(uni_dev, val, offset, val_size);
    if (ret < 0)
        return ret;
    if (ret != val_size)
        return -EINVAL;
    return 0;
}
EXPORT_SYMBOL(regmap_i2c_eeprom_read);

/* The same as in the at24.c at24 drive, we use page mode writes */
static int i2c_eeprom_write(struct universal_device *uni_dev, const char *buf,
       unsigned offset, size_t count) {
    struct i2c_client *client = to_i2c_client(uni_dev->dev);
    struct i2c_eeprom_client *eeprom_clients = &client->clients;
    struct i2c_msg msg;
    ssize_t status = 0;
    int i;
    unsigned long timeout, write_time;
    unsigned next_page;

    /* choose the client in case of multiple clients in one chip */
    if (eeprom_clients->flags & 0x80) {
        /* hardcoded the flag for 16-bit address pointer, i.e., 0x80 */
        i = offset >> 16;
        offset &= 0xffff;
    } else {
        i = offset >> 8;
        offset &= 0xff;
    }
    client = eeprom_clients->clients[i];

    if (count > eeprom_clients->write_max)
        count = eeprom_clients->write_max;

    next_page = roundup(offset + 1, eeprom_clients->page_size);
    if (offset + count > next_page)
        count = next_page - offset;
    /* default i2c calls for all I/Os */
    i = 0;
    msg.addr = client->addr;
    msg.flags = 0;
    msg.buf = eeprom_clients->write_buf; 
    if (eeprom_clients->flags & 0x80)
        msg.buf[i++] = offset >> 8;
    msg.buf[i++] = offset;
    memcpy(&msg.buf[i], buf, count);
    msg.len = i + count;

    timeout = jiffies + msecs_to_jiffies(eeprom_clients->write_timeout);
    do {
        write_time = jiffies;
        status = i2c_transfer(client->adapter, &msg, 1);
        if (status == 1)
            status = count;
        dev_dbg(&client->dev, "write %zu@%d --> %zd (%ld)\n",
                count, offset, status, jiffies);
        if (status == count)
            return count;
        msleep(1);
    } while (time_before(write_time, timeout));
    return -ETIMEDOUT;
} 


static int eeprom_write(struct universal_device *uni_dev, const char *buf,
        loff_t off, size_t count) {
    ssize_t ret = 0;
    ssize_t status;

    if (unlikely(!count))
        return count;

    mutex_lock(&uni_dev->lock);
    while (count) {
        status = i2c_eeprom_write(uni_dev, buf, off, count);
        if (status < 0) {
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

int regmap_i2c_eeprom_gather_write(void *context, const void *reg,
        size_t reg_size, const void *val, size_t val_size) {
    struct device *dev = context;
    struct universal_device *uni_dev;
    struct register_accessor *regacc = NULL;
    unsigned int offset;
    int ret;
    
    uni_dev = check_universal_driver(dev);
    if (!uni_dev) {
        LJTALE_MSG(KERN_ERR, "universal driver unavailable for device: %s\n",
                dev_name(dev));
        return -EINVAL;
    }
    BUG_ON(reg_size != 4);
    if (uni_dev->drv)
        regacc = uni_dev->drv->regacc;
    BUG_ON(!regacc);
    BUG_ON(regacc->reg_val_bits != 8);

    offset = __raw_readl(reg);

    ret = eeprom_write(uni_dev, val, offset, val_size);
    if (ret < 0)
        return ret;
    if (ret != val_size)
        return -EINVAL;
    return 0;
}
EXPORT_SYMBOL(regmap_i2c_eeprom_gather_write);

int regmap_i2c_eeprom_write(void *context, const void *data, size_t count) {
    /* same as the read function, here we pass the context for eeprom write
     * using conventioal device and use it to get the universal device */
    struct device *dev = context;
    struct universal_device *uni_dev;
    struct register_accessor *regacc = NULL;
    unsigned int reg_bytes, offset;

    uni_dev = check_universal_driver(dev);
    if (!uni_dev) {
        LJTALE_MSG(KERN_ERR, "universal driver unavailable for device: %s\n",
                dev_name(dev));
        return -EINVAL;
    }
    if (uni_dev->drv)
        regacc = uni_dev->drv->regacc;
    if (!regacc)
        return -EINVAL;

    reg_bytes = regacc->reg_addr_bits / 8;
    offset = reg_bytes;

    BUG_ON(reg_bytes != 4);
    BUG_ON(count <= offset);

    return regmap_i2c_eeprom_gather_write(context, data, reg_bytes,
            data + offset, count - offset);
} 
EXPORT_SYMBOL(regmap_i2c_eeprom_write);


int universal_reg_read(struct device *dev, unsigned int reg, 
        unsigned int *val) {
    struct universal_device *uni_dev;
    struct register_accessor *regacc = NULL;
    
    uni_dev = check_universal_driver(dev);
    if (!uni_dev) {
        dev_dbg(dev, "universal driver not available for device: %s\n",
                dev_name(dev));
        return -EINVAL;
    }
    /* check_universal_driver already make sure the drv pointer for uni_dev
     * is not NULL */
    regacc = uni_dev->drv->regacc;
    BUG_ON(!regacc);
    if (regacc->regmap_support)
        return regmap_read(regacc->regmap, reg, val);
    else if (regacc->regacc_read)
        return regacc->regacc_read(reg, val);
        
    else 
        LJTALE_MSG(KERN_ERR, "no universal reg read method for device: %s\n",
                dev_name(dev));
    return -EINVAL;
}

/* this function is used only when the device registers are memory mapped */
int universal_mmio_reg_read(struct universal_device *uni_dev,
        unsigned int reg, void *val) {
    /* TODO: the register description should be part of the device model, which
     * should come from the universal_device instance. */
    struct register_accessor *regacc;
    BUG_ON(!uni_dev->drv);
    regacc = uni_dev->drv->regacc;
    switch(regacc->reg_addr_bits) {
        case 8:
            return -EINVAL;
        case 16:
            return -EINVAL;
        case 32:
            switch(regacc->reg_val_bits) {
                case 8:
                    if (regacc->mb)
                        *((u8 *) val) = readb(regacc->base + reg);
                    else
                        *((u8 *) val) = readb_relaxed(regacc->base + reg);
                    break;
                case 16:
                    if (regacc->mb)
                        *((u16 *) val) = readw(regacc->base + reg);
                    else
                        *((u16 *) val) = readw_relaxed(regacc->base + reg);
                    break;
                case 32:
                    if (regacc->mb)
                        *((u32 *) val) = readl(regacc->base + reg);
                    else
                        *((u32 *) val) = readl_relaxed(regacc->base + reg);
                    break;
                default:
                    return -EINVAL;
            }
            break;
        default:
            return -EINVAL;
    }
    LJTALE_LEVEL_DEBUG(4, "universal mmio read: %s\n", uni_dev->name);
    return 0;
}


int universal_reg_write(struct device *dev, unsigned int reg,
        unsigned int val) {
    struct universal_device *uni_dev;
    struct register_accessor *regacc = NULL;
    
    uni_dev = check_universal_driver(dev);
    if (!uni_dev) {
        dev_dbg(dev, "universal driver not available for device: %s\n",
                dev_name(dev));
        return -EINVAL;
    }
    /* check_universal_driver already make sure the drv pointer for uni_dev
     * is not NULL */
    regacc = uni_dev->drv->regacc;
    BUG_ON(!regacc);
    if (regacc->regmap_support)
        return regmap_write(regacc->regmap, reg, val);
    else if (regacc->regacc_write)
        return regacc->regacc_write(reg, val);
    else 
        LJTALE_MSG(KERN_ERR, "no universal reg write method for device: %s\n",
                dev_name(dev));
    return -EINVAL;
}

int universal_mmio_reg_write(struct universal_device *uni_dev,
        unsigned int reg, unsigned int val) {
    /* TODO: the register description should be part of the device model, which
     * should come from the universal_device instance. */
    struct register_accessor *regacc;
    BUG_ON(!uni_dev->drv);
    regacc = uni_dev->drv->regacc;
    switch(regacc->reg_addr_bits) {
        case 8:
            return -EINVAL;
        case 16:
            return -EINVAL;
        case 32:
            switch(regacc->reg_val_bits) {
                case 8:
                    if (regacc->mb)
                        writeb((u8)val, regacc->base + reg);
                    else
                        writeb_relaxed((u8)val, regacc->base + reg);
                    break;
                case 16:
                    if (regacc->mb)
                        writew((u16)val, regacc->base + reg);
                    else
                        writew_relaxed((u16)val, regacc->base + reg);
                    break;
                case 32:
                    if (regacc->mb)
                        writel((u32)val, regacc->base + reg);
                    else
                        writel_relaxed((u32)val, regacc->base + reg);
                    break;
                default:
                    return -EINVAL;
            }
            break;
        default:
            return -EINVAL;
    }
    LJTALE_LEVEL_DEBUG(4, "universal mmio write: %s\n", uni_dev->name);
    return 0;
}

void _populate_regmap_config(struct register_accessor *regacc, 
        struct regmap_config *config) {
    /* outside world makes sure both regacc and config are valid */
    /* clear out the config memory */
    memset(config, 0, sizeof(*config));
    config->reg_bits = regacc->reg_addr_bits;
    config->val_bits = regacc->reg_val_bits;
    config->max_register = regacc->max_register;
    /* FIXME: cache mechanism should be passed to regacc as well */
    config->cache_type = REGCACHE_NONE;
}

struct regmap_bus *_choose_regmap_bus(struct register_accessor *regacc) {
    /* outside world makes sure regacc is valid */
    switch(regacc->regmap_bus) {
        case I2C_REGMAP_BUS:
            return &i2c_regmap_bus;
        case I2C_EEPROM_REGMAP_BUS:
            return &i2c_eeprom_regmap_bus;
        case SPI_REGMAP_BUS:
            return &spi_regmap_bus;
        case SPI_EEPROM_REGMAP_BUS:
            return &spi_eeprom_regmap_bus;
        default:
            return NULL;
    }
}

int __universal_get_irq(struct universal_device *uni_dev,
                        struct irq_config *irq_config) {
    struct device *dev = uni_dev->dev;
    struct of_phandle_args oirq;
    int ret = -EINVAL;
    /* supposedly the uni_dev has already been matched with a universal driver
     * instance */
    BUG_ON(!uni_dev->drv);
    BUG_ON(!irq_config);
    /* first try to get irq from device tree node if there is one */
    if (dev->of_node) {
        ret = of_irq_parse_one(dev->of_node, irq_config->irq_index, &oirq);
        if (ret)
            /* of irq parse one should return 0 on success, otherwise it
             * should return error code */
            goto no_device_node_irq;
        if (irq_config->defered_probe) {
            struct irq_domain *domain;
            domain = irq_find_host(oirq.np);
            if (!domain) {
                LJTALE_LEVEL_DEBUG(4,"domain: 0x%x\n",(unsigned int) domain); 
                ret = -EPROBE_DEFER;
                goto no_device_node_irq;
            }
        }
        ret = irq_create_of_mapping(&oirq);
    }
no_device_node_irq:
    if (ret >=0 || ret == -EPROBE_DEFER)
        return ret;
    else if (irq_config->platform_irq) {
        struct resource *r;
        struct platform_device *pdev;
        /* in this case, the struct device must be a platform device,
         * otherwise there should be some problems */
        pdev = to_platform_device(dev);
        BUG_ON(!pdev);
        r = platform_get_resource(pdev, IORESOURCE_IRQ, irq_config->irq_index);
        if (r && r->flags & IORESOURCE_BITS)
            irqd_set_trigger_type(irq_get_irq_data(r->start),
                    r->flags & IORESOURCE_BITS);
        return r ? r->start : -ENXIO;

    } else if (irq_config->get_gpio_irq) {
        /* get irq from gpio pin */
        int irq_gpio = -1;
        irq_gpio = of_get_named_gpio(dev->of_node, "irq-gpio", 0);
        if (irq_gpio >= 0 ) {
            /* TODO: as I checked, this piece of code is not used 
             * at present, will add later */
        }
    } 
    return ret;
}
