/*
 * RPM definition of OMAP HSMMC controller @ljtale
 * */

#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/universal-drv.h>

/* register offset */
#define OMAP_HSMMC_SYSSTATUS	0x0014
#define OMAP_HSMMC_CON		0x002C
#define OMAP_HSMMC_DLL		0x0034
#define OMAP_HSMMC_SDMASA	0x0100
#define OMAP_HSMMC_BLK		0x0104
#define OMAP_HSMMC_ARG		0x0108
#define OMAP_HSMMC_CMD		0x010C
#define OMAP_HSMMC_RSP10	0x0110
#define OMAP_HSMMC_RSP32	0x0114
#define OMAP_HSMMC_RSP54	0x0118
#define OMAP_HSMMC_RSP76	0x011C
#define OMAP_HSMMC_DATA		0x0120
#define OMAP_HSMMC_PSTATE	0x0124
#define OMAP_HSMMC_HCTL		0x0128
#define OMAP_HSMMC_SYSCTL	0x012C
#define OMAP_HSMMC_STAT		0x0130
#define OMAP_HSMMC_IE		0x0134
#define OMAP_HSMMC_ISE		0x0138
#define OMAP_HSMMC_AC12		0x013C
#define OMAP_HSMMC_CAPA		0x0140
#define OMAP_HSMMC_CAPA2	0x0144


// #define MMC_CAP_SDIO_IRQ (1 << 3) /* defined in linux/mmc/host.h */
#define HSMMC_SDIO_IRQ_ENABLED (1 << 1)
#define CLKEXTFREE_ENABLED (1 << 2)

#define DLEV_DAT(x) (1 << (20 + (x))) /* present state beginning from 20th bit*/

const static u32 stat_clear = 0xffffffff;
const static u32 cirq_en = 0x1 << 8;
const static u32 zero = 0;

