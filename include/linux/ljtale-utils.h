#ifndef LINUX_LJTLAE_UTILS_H
#define LINUX_LJTALE_UTILS_H

#define LJTALE_MSG(LEVEL, args...)             \
    do {                                        \
        printk(LEVEL "ljtale: " args);                   \
    }                                           \
    while(0)

#define LJTALE_DEBUG_ENABLE

#ifdef LJTALE_DEBUG_ENABLE
#define LJTALE_PRINT(LEVEL, args...)           \
    do {                                        \
        printk(LEVEL "ljtale: " args);    \
    }                                       \
    while(0)

#define LJTALE_WARN(args...)           \
    do {                                        \
        printk(KERN_WARNING "ljtale-warning: " args);    \
    }                                       \
    while(0)
#else
#define LJTALE_PRINT(LEVEL, args...)
#define LJTALE_WARN(args...)
#endif

#endif /* LINUX_LJTALE_UTILS_H */
