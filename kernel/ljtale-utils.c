#include <linux/kernel.h>
#include <linux/ljtale-utils.h>

void ljtale_perf_init(void) {
    // enable monitor
    asm volatile("mcr p15, 0, %0, c9, c12, 0" :: "r"(7 | 16));
    // enable all the counters
    asm volatile("mcr p15, 0, %0, c9, c12, 1" :: "r"(0x8000000f));
}

u32 ljtale_read_pmc(void) {
    u32 res = 0;
    asm volatile("mrc p15, 0, %0, c9, c13, 0" :"=r"(res));
    return res;
}
