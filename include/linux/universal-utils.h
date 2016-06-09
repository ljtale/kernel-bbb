#ifndef _LINUX_UNIVERSAL_UTILS_H
#define _LINUX_UNIVERSAL_UTILS_H

#include <linux/universal-drv.h>

int universal_reg_read(struct register_accessor *regacc, unsigned int reg,
        unsigned int *val);

int universal_reg_write(struct register_accessor *regacc, unsigned int reg,
        unsigned int val);

#endif /* _LINUX_UNIVERSAL_UTILS_H */
