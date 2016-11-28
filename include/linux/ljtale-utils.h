#ifndef LINUX_LJTLAE_UTILS_H
#define LINUX_LJTALE_UTILS_H

#define LJTALE_DEBUG_LEVEL 3

// #define LJTALE_DEBUG_ENABLE
// #define LJTALE_DEBUG_ENABLE_LIST

#define LJTALE_MSG(LEVEL, args...)             \
    do {                                        \
        printk(LEVEL "ljtale: " args);                   \
    }                                           \
    while(0)

#ifdef LJTALE_DEBUG_ENABLE
#define LJTALE_LEVEL_DEBUG(level, args...) \
    do {                                    \
        if (level <= LJTALE_DEBUG_LEVEL) {           \
            printk(KERN_INFO "ljtale-" #level ": " args);  \
        }                                       \
    } while (0)
#else
#define LJTALE_LEVEL_DEBUG(level, args...)
#endif

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

#define LJTALE_DEBUG_PRINT(args...) \
    do {                            \
        printk(KERN_INFO "ljtale-debug: %s: line: %d " args, \
                __FILE__, __LINE__);                \
    }                               \
    while(0)

// #define LJTALE_PERF

void ljtale_perf_init(void);
u32 ljtale_read_pmc(void);

#endif /* LINUX_LJTALE_UTILS_H */
