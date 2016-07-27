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

#if 0
const static u32 stat_clear = 0xffffffff;
const static u32 cirq_en = (1 << 8);
const static u32 zero = 0;
static u32 dlev_dat_1 = DLEV_DAT(1);
static u32 mmc_cap_sdio_irq = (1 << 3);
static u32 hsmmc_sdio_irq_enabled = (1 << 1);
static u32 omap_hsmmc_pstate = 0;
static int return_value = 0;
static unsigned long lock_flags;

struct omap_hsmmc_rpm_context;

/* rpm suspend nodes */

RPM_REG_READ_NODE(con_reg_1, OMAP_HSMMC_CON, NULL);
RPM_REG_READ_NODE(hctl_reg_1, OMAP_HSMMC_HCTL, NULL);
RPM_REG_READ_NODE(sysctl_reg_1, OMAP_HSMMC_SYSCTL, NULL);
RPM_REG_READ_NODE(capa_reg_1, OMAP_HSMMC_CAPA, NULL);

#if 0
/* we can either define the register read nodes separately, or we can define
 * a basic block for the above four nodes */
#endif 

RPM_SPINLOCK_NODE(spinlock_1, RPM_SPIN_LOCK);

/* first condition op */
RPM_CONDITION_OP(mmc_caps_1, RPM_CONDITION_BIT_AND);
RPM_CONDITION_OP(host_flag_1, RPM_CONDITION_BIT_AND);
RPM_CONDITION_OP(and_con_1, RPM_CONDITION_AND);
RPM_CONDITION_NODE(condition_1, and_con_1);

RPM_REG_WRITE_NODE(ise_reg_1, OMAP_HSMMC_ISE, &zero);
RPM_REG_WRITE_NODE(ie_reg_1, OMAP_HSMMC_IE, &zero);

RPM_REG_READ_NODE(pstate_reg_1, OMAP_HSMMC_PSTATE, &omap_hsmmc_pstate);
RPM_CONDITION_OP(pstate_con_1, RPM_CONDITION_BIT_AND);
RPM_CONDITION_OP(not_con_1, RPM_CONDITION_NOT);
RPM_CONDITION_NODE(condition_2, not_con_1);

RPM_REG_WRITE_NODE(stat_reg_1, OMAP_HSMMC_STAT, &stat_clear);
RPM_REG_WRITE_NODE(ise_reg_2, OMAP_HSMMC_ISE, &cirq_en);
RPM_REG_WRITE_NODE(ie_reg_2, OMAP_HSMMC_IE, &cirq_en);
RPM_DEVICE_CALL_NODE(rpm_mark_last_busy, RPM_MARK_LAST_BUSY);
RPM_ASSIGNMENT_NODE(ebusy, -EBUSY);

RPM_DEVICE_CALL_NODE(pinctrl_idle_state, RPM_PINCTRL_IDLE);

RPM_SPINLOCK_NODE(spinlock_2, RPM_SPIN_UNLOCK);

RPM_RETURN_NODE(ret, &return_value);

void omap_hsmmc_rpm_graph_build(void) {
    LJTALE_LEVEL_DEBUG(3, "build rpm graph at: %s\n", __func__);
    /* suspend control flow graph */
    RPM_NODE_CONTROL(con_reg_1, hctl_reg_1);
    RPM_NODE_CONTROL(hctl_reg_1, sysctl_reg_1);
    RPM_NODE_CONTROL(sysctl_reg_1, capa_reg_1);
    RPM_NODE_CONTROL(capa_reg_1, spinlock_1);

    RPM_NODE_CONTROL(spinlock_1, condition_1);

    RPM_CONDITION_OP_CONTROL(and_con_1, mmc_caps_1, host_flag_1);
    RPM_CONDITION_CONTROL(condition_1, ise_reg_1, pinctrl_idle_state);

    RPM_NODE_CONTROL(ise_reg_1, ie_reg_1);
    RPM_NODE_CONTROL(ie_reg_1, pstate_reg_1);
    RPM_NODE_CONTROL(pstate_reg_1, condition_2);

    RPM_CONDITION_OP_NAME(not_con_1).left_op = 
        &RPM_CONDITION_OP_NAME(pstate_con_1);
    RPM_CONDITION_CONTROL(condition_2, stat_reg_1, pinctrl_idle_state);

    RPM_NODE_CONTROL(stat_reg_1, ise_reg_2);
    RPM_NODE_CONTROL(ise_reg_2, ie_reg_2);
    RPM_NODE_CONTROL(ie_reg_2, rpm_mark_last_busy);
    RPM_NODE_CONTROL(rpm_mark_last_busy, ebusy);
    RPM_NODE_CONTROL(ebusy, spinlock_2);

    RPM_NODE_CONTROL(pinctrl_idle_state, spinlock_2);
    RPM_NODE_CONTROL(spinlock_2, ret);

    RPM_NODE_NAME(ret).next = NULL;

    /* specify the context pointers that don't change */
    RPM_CONDITION_OP_NAME(mmc_caps_1).right_value = &mmc_cap_sdio_irq;
    RPM_CONDITION_OP_NAME(host_flag_1).right_value = &hsmmc_sdio_irq_enabled;
    RPM_CONDITION_OP_NAME(pstate_con_1).left_value = &omap_hsmmc_pstate;
    RPM_CONDITION_OP_NAME(pstate_con_1).right_value = &dlev_dat_1;
    RPM_SPINLOCK_NODE_NAME(spinlock_1).flags = &lock_flags;
    RPM_SPINLOCK_NODE_NAME(spinlock_2).flags = &lock_flags;
}

void omap_hsmmc_rpm_populate_suspend_graph(struct universal_device *uni_dev) {
    struct omap_hsmmc_rpm_context *rpm_context = uni_dev->rpm_data_dev;
    if (!rpm_context) {
        rpm_context = devm_kzalloc(uni_dev->dev, 
                sizeof(struct omap_hsmmc_rpm_context), GFP_KERNEL);
        if (!rpm_context)
            return;
        uni_dev->rpm_data_dev = rpm_context;
    }
    /* fill rpm context */
    RPM_REG_NODE_NAME(con_reg_1).reg_value = &rpm_context->con;
    RPM_REG_NODE_NAME(hctl_reg_1).reg_value = &rpm_context->hctl;
    RPM_REG_NODE_NAME(sysctl_reg_1).reg_value = &rpm_context->sysctl;
    RPM_REG_NODE_NAME(capa_reg_1).reg_value = &rpm_context->capa;

    RPM_CONDITION_OP_NAME(mmc_caps_1).left_value = &rpm_context->mmc_caps;
    RPM_CONDITION_OP_NAME(host_flag_1).left_value =&rpm_context->host_flags;
    RPM_SPINLOCK_NODE_NAME(spinlock_1).lock = &rpm_context->irq_lock;
    RPM_SPINLOCK_NODE_NAME(spinlock_2).lock = &rpm_context->irq_lock;
}
#endif


/* =============== rpm generic logic part ===== */
int omap_hsmmc_rpm_create_reg_context (struct universal_device *uni_dev) {
    u32 *array = uni_dev->drv->ref_ctx.array;
    int size = uni_dev->drv->ref_ctx.size;
    if (!array)
        return 0;
    int i;
    u32 *dev_array = devm_kzalloc(uni_dev->dev, sizeof(u32) * size, GFP_KERNEL);
    if (!dev_array)
        return -ENOMEM;
    for (i = 0; i < size; i++)
        dev_array[i] = array[i];
    uni_dev->rpm_context.array = dev_array;
    uni_dev->rpm_context.size = size;
    return 0;
}
