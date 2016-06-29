#ifndef _LINUX_UNIVERSAL_UTILS_H
#define _LINUX_UNIVERSAL_UTILS_H

#include <linux/universal-drv.h>

int universal_reg_read(struct device *dev, unsigned int reg,
        unsigned int *val);

int universal_reg_write(struct device *dev, unsigned int reg,
        unsigned int val);

int universal_mmio_reg_read(struct universal_device *uni_dev, unsigned int reg,
        void *val);

int universal_mmio_reg_write(struct universal_device *uni_dev,
        unsigned int reg, unsigned int val);

#endif /* _LINUX_UNIVERSAL_UTILS_H */
