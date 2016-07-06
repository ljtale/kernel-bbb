/* 
 * RPM definition of OMAP I2C controller @ljtale
 * */

#include <linux/universal-drv.h>

#define OMAP_I2C_V2_IE_REG 0x2c
#define OMAP_I2C_V2_STAT_REG 0x28
#define OMAP_I2C_V2_CON_REG 0xa4
#define OMAP_I2C_IP_V2_IRQENABLE_CLR 0x30


static struct rpm_suspend_graph *omap_i2c_runtime_suspend; 

