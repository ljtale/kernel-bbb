#ifndef _LINUX_UNIVERSAL_UTILS_H
#define _LINUX_UNIVERSAL_UTILS_H

#include <linux/universal-drv.h>

int universal_reg_read(struct universal_device *uni_dev, unsigned int reg,
        void *val);

int universal_reg_write(struct universal_device *uni_dev, unsigned int reg,
        unsigned int val);

int universal_mmio_reg_read(struct universal_device *uni_dev, unsigned int reg,
        void *val);

int universal_mmio_reg_write(struct universal_device *uni_dev,
        unsigned int reg, unsigned int val);

int universal_runtime_suspend(struct device *dev);
int universal_runtime_resume(struct device *dev);
int universal_suspend(struct device *dev);
int universal_resume(struct device *dev);



#endif /* _LINUX_UNIVERSAL_UTILS_H */
