/* 
 * RPM definition of OMAP I2C controller @ljtale
 * */
#include <linux/i2c-omap.h>
#include <linux/universal-drv.h>

#define OMAP_I2C_V2_IE_REG 0x2c
#define OMAP_I2C_V2_STAT_REG 0x28
#define OMAP_I2C_V2_CON_REG 0xa4
#define OMAP_I2C_IP_V2_IRQENABLE_CLR 0x30
#define OMAP_I2C_IP_V2_INTERRUPTS_MASK 0x6FFF

static struct rpm_node *omap_i2c_runtime_suspend; 
static struct rpm_node *omap_i2c_runtime_resume;

/* declaration of all nodes */
RPM_REG_READ_NODE(ie_reg_1, OMAP_I2C_V2_IE_REG, NULL);

RPM_REG_WRITE_NODE(irqenable_clr, OMAP_I2C_IP_V2_IRQENABLE_CLR, NULL);

RPM_REG_WRITE_NODE(stat_reg_1, OMAP_I2C_V2_STAT_REG, NULL);

RPM_REG_READ_NODE(stat_reg_2, OMAP_I2C_V2_STAT_REG, NULL);

RPM_PIN_STATE_SELECT_NODE(sleep_state, RPM_PINCTRL_SLEEP);

RPM_PIN_STATE_SELECT_NODE(default_state, RPM_PINCTRL_DEFAULT);


/* This function that is called in the conventional driver init.
 * It builds the suspend/resume graph */

void omap_i2c_rpm_graph_build(struct universal_device *uni_dev) {
    /* omap i2c suspend graph */
    /* FIXME: reg values should replace intermediate reg values in the
     * device state constainer, i.e., the device-specific date */

    /* set up the control flow */
    omap_i2c_runtime_suspend = &RPM_NODE_NAME(ie_reg_1);
    RPM_NODE_CONTROL(ie_reg_1, irqenable_clr);
    RPM_NODE_CONTROL(irqenable_clr, stat_reg_1);
    RPM_NODE_CONTROL(stat_reg_1, stat_reg_2);
    RPM_NODE_CONTROL(stat_reg_2, sleep_state);
    RPM_NODE_NAME(sleep_state).next = NULL;

    /* TODO: omap i2c resume graph */
    omap_i2c_runtime_resume = &RPM_NODE_NAME(default_state);
}
EXPORT_SYMBOL(omap_i2c_rpm_graph_build);
