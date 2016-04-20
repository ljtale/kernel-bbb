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
#else
#define LJTALE_PRINT(LEVEL, args...)
#endif

#endif
