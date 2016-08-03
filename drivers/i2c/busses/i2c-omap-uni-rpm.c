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

#if 0
struct omap_i2c_rpm_reg_value;

const static u16 omap_i2c_interrupts_mask = 0x6fff;
const static u16 zero = 0;
const static u16 omap_i2c_con_enable = 0x1 << 15;

/* declaration of all nodes */
/* suspend */
RPM_REG_READ_NODE(ie_reg_1, OMAP_I2C_V2_IE_REG, NULL);

RPM_REG_WRITE_NODE(irqenable_clr, OMAP_I2C_IP_V2_IRQENABLE_CLR,
        &omap_i2c_interrupts_mask);

RPM_REG_WRITE_NODE(stat_reg_1, OMAP_I2C_V2_STAT_REG, NULL);

RPM_REG_READ_NODE(stat_reg_2, OMAP_I2C_V2_STAT_REG, NULL);

RPM_DEVICE_CALL_NODE(pinctrl_sleep_state, RPM_PINCTRL_SLEEP);

/* resume */
RPM_DEVICE_CALL_NODE(pinctrl_default_state, RPM_PINCTRL_DEFAULT);

RPM_REG_WRITE_NODE(con_reg_1, OMAP_I2C_V2_CON_REG, &zero);

RPM_REG_WRITE_NODE(psc_reg_1, OMAP_I2C_V2_PSC_REG, NULL);

RPM_REG_WRITE_NODE(scll_reg_1, OMAP_I2C_V2_SCLL_REG, NULL);

RPM_REG_WRITE_NODE(sclh_reg_1, OMAP_I2C_V2_SCLH_REG, NULL);

RPM_REG_WRITE_NODE(we_reg_1, OMAP_I2C_V2_WE_REG, NULL);

RPM_REG_WRITE_NODE(con_reg_2, OMAP_I2C_V2_CON_REG, &omap_i2c_con_enable);

RPM_CONDITION_OP(ie_check, RPM_CONDITION_SIMPLE);
RPM_CONDITION_NODE(if_iestate, ie_check);

RPM_REG_WRITE_NODE(ie_reg_2, OMAP_I2C_V2_IE_REG, NULL);

/* This function that is called in the conventional driver init.
 * It builds the suspend/resume graph, graph is the same across different
 * devices of the same type, thus on device argument is taken */
void omap_i2c_rpm_graph_build(void) {
    LJTALE_LEVEL_DEBUG(3, "build rpm graph at: %s\n", __func__);
    /* set up the suspend control flow */
    RPM_NODE_CONTROL(ie_reg_1, irqenable_clr);
    RPM_NODE_CONTROL(irqenable_clr, stat_reg_1);
    RPM_NODE_CONTROL(stat_reg_1, stat_reg_2);
    RPM_NODE_CONTROL(stat_reg_2, pinctrl_sleep_state);
    RPM_NODE_NAME(pinctrl_sleep_state).next = NULL;

    /* set up the resume control flow */
    RPM_NODE_CONTROL(pinctrl_default_state, con_reg_1);
    RPM_NODE_CONTROL(con_reg_1, psc_reg_1);
    RPM_NODE_CONTROL(psc_reg_1, scll_reg_1);
    RPM_NODE_CONTROL(scll_reg_1, sclh_reg_1);
    RPM_NODE_CONTROL(sclh_reg_1, we_reg_1);
    RPM_NODE_CONTROL(we_reg_1, con_reg_2);
    RPM_NODE_CONTROL(con_reg_2, if_iestate);
    RPM_CONDITION_NODE_NAME(if_iestate).true_path = &RPM_NODE_NAME(ie_reg_2);
    RPM_CONDITION_NODE_NAME(if_iestate).false_path = NULL;
    RPM_NODE_NAME(ie_reg_2).next = NULL;
}
EXPORT_SYMBOL(omap_i2c_rpm_graph_build);

/* called every time before doing runtime power managment */
void omap_i2c_rpm_populate_suspend_graph(struct universal_device *uni_dev) {
    /* FIXME: reg values should replace intermediate reg values in the
     * device state constainer, i.e., the device-specific date */
    struct omap_i2c_rpm_reg_value *reg_values = uni_dev->rpm_data_dev;
    if (!reg_values) {
        reg_values = devm_kzalloc(uni_dev->dev, 
                sizeof(struct omap_i2c_rpm_reg_value), GFP_KERNEL);
        if (!reg_values)
            return;
        uni_dev->rpm_data_dev = reg_values;
    }
    /* fill reg value dependencies */
    RPM_REG_NODE_NAME(ie_reg_1).reg_value = (u32 *)&reg_values->iestate;
    RPM_REG_NODE_NAME(stat_reg_1).reg_value = (u32 *)&reg_values->iestate;
    uni_dev->rpm_suspend_graph = &RPM_NODE_NAME(ie_reg_1);
}
EXPORT_SYMBOL(omap_i2c_rpm_populate_suspend_graph);

void omap_i2c_rpm_populate_resume_graph(struct universal_device *uni_dev) {
    struct omap_i2c_rpm_reg_value *reg_values = uni_dev->rpm_data_dev;
    if (!reg_values) {
        reg_values = devm_kzalloc(uni_dev->dev,
                sizeof(struct omap_i2c_rpm_reg_value), GFP_KERNEL);
        if (!reg_values)
            return;
        uni_dev->rpm_data_dev = reg_values;
    }
    /* fill reg value dependencies */
    RPM_REG_NODE_NAME(psc_reg_1).reg_value = (u32 *)&reg_values->pscstate;
    RPM_REG_NODE_NAME(scll_reg_1).reg_value = (u32 *)&reg_values->scllstate;
    RPM_REG_NODE_NAME(sclh_reg_1).reg_value = (u32 *)&reg_values->sclhstate;
    RPM_REG_NODE_NAME(we_reg_1).reg_value = (u32 *)&reg_values->westate;
    RPM_CONDITION_OP_NAME(ie_check).left_value = (u32 *)&reg_values->iestate;
    RPM_CONDITION_OP_NAME(ie_check).is_left_value = true;
    RPM_REG_NODE_NAME(ie_reg_2).reg_value = (u32 *)&reg_values->iestate;
    uni_dev->rpm_resume_graph = &RPM_NODE_NAME(pinctrl_default_state);
}
EXPORT_SYMBOL(omap_i2c_rpm_populate_resume_graph);
#endif

/* ============ rpm generic logic part ===========*/
int omap_i2c_rpm_create_reg_context(struct universal_device *uni_dev) {
    struct universal_rpm *rpm = &uni_dev->drv->rpm;
    u32 *array = rpm->ref_ctx.array;
    int size = rpm->ref_ctx.size;
    if (!array)
        return 0;
    int i;
    u32 *dev_array = devm_kzalloc(uni_dev->dev, sizeof(u32) * size, GFP_KERNEL);
    if (!dev_array)
        return -ENOMEM;
    /* copy everything including the constant values */
    for (i = 0; i < size; i++)
        dev_array[i] = array[i];

    uni_dev->rpm_dev.rpm_context.array = dev_array;
    uni_dev->rpm_dev.rpm_context.size = size;
    return 0;
}
EXPORT_SYMBOL(omap_i2c_rpm_create_reg_context);

