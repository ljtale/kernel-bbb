/* 
 * RPM definition of OMAP I2C controller @ljtale
 * */
#include <linux/i2c-omap.h>
#include <linux/universal-drv.h>

#define OMAP_I2C_V2_IE_REG 0x2c
#define OMAP_I2C_V2_STAT_REG 0x28
#define OMAP_I2C_V2_WE_REG 0x34
#define OMAP_I2C_V2_CON_REG 0xa4
#define OMAP_I2C_V2_PSC_REG 0xb0
#define OMAP_I2C_V2_SCLL_REG 0xb4
#define OMAP_I2C_V2_SCLH_REG 0xb8
#define OMAP_I2C_IP_V2_IRQENABLE_CLR 0x30
#define OMAP_I2C_IP_V2_INTERRUPTS_MASK 0x6FFF

/* const shared reads */
const static u16 omap_i2c_interrupts_mask = 0x6fff;
const static u16 zero = 0;
const static u16 omap_i2c_con_enable = 0x1 << 15;

#if 0

/* declaration of all nodes */
/* suspend */
RPM_REG_READ_NODE(ie_reg_1, OMAP_I2C_V2_IE_REG, NULL);

RPM_REG_WRITE_NODE(irqenable_clr, OMAP_I2C_IP_V2_IRQENABLE_CLR,
        &omap_i2c_interrupts_mask);

RPM_REG_WRITE_NODE(stat_reg_1, OMAP_I2C_V2_STAT_REG, NULL);

RPM_REG_READ_NODE(stat_reg_2, OMAP_I2C_V2_STAT_REG, NULL);

RPM_PIN_STATE_SELECT_NODE(sleep_state, RPM_PINCTRL_SLEEP);

/* resume */
RPM_PIN_STATE_SELECT_NODE(default_state, RPM_PINCTRL_DEFAULT);

RPM_REG_WRITE_NODE(con_reg_1, OMAP_I2C_V2_CON_REG, &zero);

RPM_REG_WRITE_NODE(psc_reg_1, OMAP_I2C_V2_PSC_REG, NULL);

RPM_REG_WRITE_NODE(scll_reg_1, OMAP_I2C_V2_SCLL_REG, NULL);

RPM_REG_WRITE_NODE(sclh_reg_1, OMAP_I2C_V2_SCLH_REG, NULL);

RPM_REG_WRITE_NODE(we_reg_1, OMAP_I2C_V2_WE_REG, NULL);

RPM_REG_WRITE_NODE(con_reg_2, OMAP_I2C_V2_CON_REG, &omap_i2c_con_enable);

RPM_CONDITION_OP(ie_check, RPM_CONDITION_SIMPLE);
RPM_CONDITION_NODE(if_iestate, ie_check);

RPM_REG_WRITE_NODE(ie_reg_2, OMAP_I2C_V2_IE_REG, NULL);
#endif

/* This function that is called in the conventional driver init.
 * It builds the suspend/resume graph */

void omap_i2c_rpm_graph_build(struct universal_device *uni_dev) {
    /* FIXME: reg values should replace intermediate reg values in the
     * device state constainer, i.e., the device-specific date */
    struct omap_i2c_rpm_reg_value *reg_values = uni_dev->rpm_data_dev;
    struct device *dev = uni_dev->dev;
    BUG_ON(!reg_values);
    LJTALE_LEVEL_DEBUG(3, "build rpm graph for: %s\n", uni_dev->name);

    /* declare rpm nodes for this device */
    RPM_DECLARE_REG_NODE(ie_reg_1);
    RPM_DECLARE_REG_NODE(irqenable_clr_1);
    RPM_DECLARE_REG_NODE(stat_reg_1);
    RPM_DECLARE_REG_NODE(stat_reg_2);
    RPM_DECLARE_PIN_STATE_SELECT_NODE(sleep_state);

    RPM_DECLARE_PIN_STATE_SELECT_NODE(default_state);
    RPM_DECLARE_REG_NODE(con_reg_1);
    RPM_DECLARE_REG_NODE(psc_reg_1);
    RPM_DECLARE_REG_NODE(scll_reg_1);
    RPM_DECLARE_REG_NODE(sclh_reg_1);
    RPM_DECLARE_REG_NODE(we_reg_1);
    RPM_DECLARE_REG_NODE(con_reg_2);
    RPM_DECLARE_CONDITION_OP(ie_check);
    RPM_DECLARE_CONDITION_NODE(if_iestate);
    RPM_DECLARE_REG_NODE(ie_reg_2);

    RPM_ALLOCATE_REG_NODE(dev, 
            RPM_REG_READ, ie_reg_1, OMAP_I2C_V2_IE_REG, NULL);
    RPM_ALLOCATE_REG_NODE(dev,
            RPM_REG_WRITE, irqenable_clr_1, OMAP_I2C_IP_V2_IRQENABLE_CLR,
            &omap_i2c_interrupts_mask);
    RPM_ALLOCATE_REG_NODE(dev, 
            RPM_REG_WRITE, stat_reg_1, OMAP_I2C_V2_STAT_REG, NULL);
    RPM_ALLOCATE_REG_NODE(dev,
            RPM_REG_READ, stat_reg_2, OMAP_I2C_V2_STAT_REG, NULL);
    RPM_ALLOCATE_PIN_STATE_SELECT_NODE(dev, sleep_state, RPM_PINCTRL_SLEEP);

    /* omap i2c suspend graph */
    /* fill reg value dependencies */
    RPM_REG_NODE_NAME(ie_reg_1)->reg_value = (u32 *)&reg_values->iestate;
    RPM_REG_NODE_NAME(stat_reg_1)->reg_value = (u32 *)&reg_values->iestate;
    /* set up the control flow */
    uni_dev->rpm_suspend_graph = RPM_NODE_NAME(ie_reg_1);
    RPM_NODE_CONTROL(ie_reg_1, irqenable_clr_1);
    RPM_NODE_CONTROL(irqenable_clr_1, stat_reg_1);
    RPM_NODE_CONTROL(stat_reg_1, stat_reg_2);
    RPM_NODE_CONTROL(stat_reg_2, sleep_state);
    RPM_NODE_NAME(sleep_state)->next = NULL;

    RPM_ALLOCATE_PIN_STATE_SELECT_NODE(dev, 
            default_state, RPM_PINCTRL_DEFAULT);
    RPM_ALLOCATE_REG_NODE(dev,
            RPM_REG_WRITE, con_reg_1, OMAP_I2C_V2_CON_REG, &zero);
    RPM_ALLOCATE_REG_NODE(dev,
            RPM_REG_WRITE, psc_reg_1, OMAP_I2C_V2_PSC_REG, NULL);
    RPM_ALLOCATE_REG_NODE(dev,
            RPM_REG_WRITE, scll_reg_1, OMAP_I2C_V2_SCLL_REG, NULL);
    RPM_ALLOCATE_REG_NODE(dev,
            RPM_REG_WRITE, sclh_reg_1, OMAP_I2C_V2_SCLH_REG, NULL);
    RPM_ALLOCATE_REG_NODE(dev,
        RPM_REG_WRITE, we_reg_1, OMAP_I2C_V2_WE_REG, NULL);
    RPM_ALLOCATE_REG_NODE(dev,
            RPM_REG_WRITE, con_reg_2, OMAP_I2C_V2_CON_REG, 
            &omap_i2c_con_enable);

    RPM_ALLOCATE_CONDITION_OP(dev, ie_check, RPM_CONDITION_SIMPLE);
    RPM_ALLOCATE_CONDITION_NODE(dev, if_iestate, ie_check);

    RPM_ALLOCATE_REG_NODE(dev,
            RPM_REG_WRITE, ie_reg_2, OMAP_I2C_V2_IE_REG, NULL);
    /* fill reg value dependencies */
    RPM_REG_NODE_NAME(psc_reg_1)->reg_value = (u32 *)&reg_values->pscstate;
    RPM_REG_NODE_NAME(scll_reg_1)->reg_value = (u32 *)&reg_values->scllstate;
    RPM_REG_NODE_NAME(sclh_reg_1)->reg_value = (u32 *)&reg_values->sclhstate;
    RPM_REG_NODE_NAME(we_reg_1)->reg_value = (u32 *)&reg_values->westate;
    RPM_CONDITION_OP_NAME(ie_check)->left_value = (u32 *)&reg_values->iestate;
    RPM_CONDITION_OP_NAME(ie_check)->is_left_value = true;
    RPM_REG_NODE_NAME(ie_reg_2)->reg_value = (u32 *)&reg_values->iestate;
    /* set up the control flow */
    uni_dev->rpm_resume_graph = RPM_NODE_NAME(default_state);
    RPM_NODE_CONTROL(default_state, con_reg_1);
    RPM_NODE_CONTROL(con_reg_1, psc_reg_1);
    RPM_NODE_CONTROL(psc_reg_1, scll_reg_1);
    RPM_NODE_CONTROL(scll_reg_1, sclh_reg_1);
    RPM_NODE_CONTROL(sclh_reg_1, we_reg_1);
    RPM_NODE_CONTROL(we_reg_1, con_reg_2);
    RPM_NODE_CONTROL(con_reg_2, if_iestate);
    RPM_CONDITION_NODE_NAME(if_iestate)->true_path = RPM_NODE_NAME(ie_reg_2);
    RPM_CONDITION_NODE_NAME(if_iestate)->false_path = NULL;
    RPM_NODE_NAME(ie_reg_2)->next = NULL;
}
EXPORT_SYMBOL(omap_i2c_rpm_graph_build);
