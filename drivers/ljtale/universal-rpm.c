/*
 * universal driver runtime pm source file @ljtale
 * */

#include <linux/universal-utils.h>
#include <linux/universal-rpm.h>

#if 0

static inline int rpm_reg_read(struct universal_device *uni_dev,
        struct rpm_node *node) {
    struct rpm_reg_node *reg_node = node->op_args;
    return universal_reg_read(uni_dev, reg_node->reg_addr, 
            reg_node->reg_value); 
}

static inline int rpm_reg_write(struct universal_device *uni_dev, 
        struct rpm_node *node) {

    struct rpm_reg_node *reg_node = node->op_args;
    return universal_reg_write(uni_dev, reg_node->reg_addr, 
            *(reg_node->reg_value)); 
}

static inline int rpm_device_call(struct universal_device *uni_dev,
        struct rpm_node *node) {
    int ret = 0;
    struct rpm_device_call_node *device_call_node = node->op_args;
    switch (device_call_node->call) {
        case RPM_PINCTRL_DEFAULT:
            pinctrl_pm_select_default_state(uni_dev->dev);
            break;
        case RPM_PINCTRL_SLEEP:
            pinctrl_pm_select_sleep_state(uni_dev->dev);
            break;
        case RPM_PINCTRL_IDLE:
            pinctrl_pm_select_idle_state(uni_dev->dev);
            break;
        case RPM_MARK_LAST_BUSY:
            pm_runtime_mark_last_busy(uni_dev->dev);
            break;
        default:
            return -EINVAL;
    }
    return ret;
    /* we don't care about pin select return value */
}

/* return boolean value of a condition operation */
/* FIXME: recursive calls could have some hazards sometimes ???*/
static u32 rpm_condition_value(struct rpm_condition_op *con_op) {
    u32 left_value, right_value;
    if (!con_op)
        return 0;
    left_value = con_op->is_left_value ? 
        *(con_op->left_value) : rpm_condition_value(con_op->left_op);
    right_value = con_op->is_right_value ?
        *(con_op->right_value) : rpm_condition_value(con_op->right_op);
    switch (con_op->type) {
        case RPM_CONDITION_SIMPLE:
            return left_value;
        case RPM_CONDITION_LT:
            return left_value < right_value;
        case RPM_CONDITION_GT:
            return left_value > right_value;
        case RPM_CONDITION_LTEQ:
            return left_value <= right_value;
        case RPM_CONDITION_GTEQ:
            return left_value >= right_value;
        case RPM_CONDITION_EQ:
            return left_value == right_value;
        case RPM_CONDITION_BIT_AND:
            return left_value & right_value;
        case RPM_CONDITION_BIT_OR:
            return left_value | right_value;
        case RPM_CONDITION_AND:
            return left_value && right_value;
        case RPM_CONDITION_OR:
            return left_value || right_value;
        default:
            return 0;
    }
}

static inline struct rpm_node *rpm_condition(struct universal_device *uni_dev,
        struct rpm_node *node) {
    struct rpm_node *condition_path;
    struct rpm_condition_node *con_node = node->op_args;

    /* first recursively determine the boolean (or u32) value returned by
     * the condition op, then decide whether to go true path or false path */
    /* confidition value should be boolean, but for value-compatible, we 
     * use u32 here. When the value is non-zero, condition is true */
    u32 condition_value = rpm_condition_value(con_node->op);
    if (condition_value)
        condition_path = con_node->true_path;
    else
        condition_path = con_node->false_path;
    /* condition_path could terminate the graph process if its last and NULL*/
    return condition_path;
}

int universal_rpm_process_graph(struct universal_device *uni_dev, 
        struct rpm_node *graph) {
    struct rpm_node *cursor = graph;
    enum rpm_op node_op;
    if (!graph) {
        LJTALE_LEVEL_DEBUG(4, "in %s graph is empty\n",  __func__);
        return 0;
    }
    while (cursor) {
        node_op = cursor->op;
        switch (node_op) {
            case RPM_REG_READ:
                if (rpm_reg_read(uni_dev, cursor) < 0)
                    return -EINVAL;
                cursor = cursor->next;
                break;
            case RPM_REG_WRITE:
                if (rpm_reg_write(uni_dev, cursor) < 0)
                    return -EINVAL;
                cursor = cursor->next;
                break;
            case RPM_DEVICE_CALL:
                if (rpm_device_call(uni_dev, cursor) < 0)
                    return -EINVAL;
                cursor = cursor->next;
                break;
            case RPM_CONDITION:
                cursor = rpm_condition(uni_dev, cursor);
                break;
            case RPM_SPIN_LOCK:
                /* TODO: */
                break;
            case RPM_SPIN_UNLOCK:
                /* TODO: */
                break;
            /* TODO: more rpm node operations */
            default:
                break;
        }
    }
    return 0;
}

int universal_runtime_suspend(struct device *dev) {
    struct universal_device *uni_dev;
    unsigned long flags;
    int ret;
    /* first find universal device */
    uni_dev = check_universal_driver(dev);
    if (!uni_dev) {
        LJTALE_MSG(KERN_ERR, "no universal driver for: %s\n", dev_name(dev));
        return 0;
    }
    LJTALE_LEVEL_DEBUG(3, "universal rpm suspend...%s\n", uni_dev->name);
    spin_lock_irqsave(&uni_dev->drv->rpm_graph_lock, flags);
    if (uni_dev->drv->rpm_populate_suspend_graph)
        uni_dev->drv->rpm_populate_suspend_graph(uni_dev);
    /* Or the suspend graph is not populated or does not need to be */
    ret = universal_rpm_process_graph(uni_dev, uni_dev->rpm_suspend_graph);
    spin_unlock_irqrestore(&uni_dev->drv->rpm_graph_lock, flags);
    return ret;
}

int universal_runtime_resume(struct device *dev) {
    struct universal_device *uni_dev;
    unsigned long flags;
    int ret;
    /* first find universal device */
    uni_dev = check_universal_driver(dev);
    if (!uni_dev) {
        LJTALE_MSG(KERN_ERR, "no universal driver for: %s\n", dev_name(dev));
        return 0;
    }
    LJTALE_LEVEL_DEBUG(3, "universal rpm resume...%s\n", uni_dev->name);
    spin_lock_irqsave(&uni_dev->drv->rpm_graph_lock, flags);
    if (uni_dev->drv->rpm_populate_resume_graph)
        uni_dev->drv->rpm_populate_resume_graph(uni_dev);
    ret = universal_rpm_process_graph(uni_dev, uni_dev->rpm_resume_graph);
    spin_unlock_irqrestore(&uni_dev->drv->rpm_graph_lock, flags);
    return ret;
}
#endif

/*============== generic logic =====================*/

static int process_reg_table(struct universal_device *uni_dev,
        struct universal_reg_entry *tbl, int table_size) {
    struct universal_reg_entry *tbl_entry;
    struct universal_rpm_ctx *reg_ctx = &uni_dev->rpm_context;
    int ret = 0;
    int i;
    for (i = 0; i < table_size; i++) {
        tbl_entry = &tbl[i];
        if (tbl_entry->reg_op == RPM_REG_READ) {
            if (tbl_entry->flushing)
                ret = universal_reg_read(uni_dev, tbl_entry->reg_offset, NULL);
            else
                ret = universal_reg_read(uni_dev, tbl_entry->reg_offset,
                        &(reg_ctx->array[tbl_entry->ctx_index]));

        } else if (tbl_entry->reg_op == RPM_REG_WRITE) {
            /* no flushing on the register writing */
            ret = universal_reg_write(uni_dev, tbl_entry->reg_offset,
                    reg_ctx->array[tbl_entry->ctx_index]);
        } else {
            /* unsupported yet */
            LJTALE_LEVEL_DEBUG(3, "unsupported op for %s\n", uni_dev->name);
        }
        if (ret) {
            LJTALE_LEVEL_DEBUG(3, "reg table process error: %d\n", ret);
            break;
        }
    }
    return ret;
}

/* Disable IRQ for runtime suspend, the logic is:
 * Disable IRQ =>
 * Check if there are pending IRQ handling =>
 *      If there is IRQ handling => reconfigure IRQ and abort 
 *      If there is no IRQ handling => succeed*/
int universal_disable_irq(struct universal_device *uni_dev) {
    struct universal_rpm *rpm = &uni_dev->drv->rpm;
    struct universal_disable_irq *disable_irq = rpm->disable_irq; 
    /* assume when calling this function, the reg context has been created */
    struct universal_disable_irq_tbl *tbl = &disable_irq->disable_table;
    int ret;
    if (!tbl->table) {
        LJTALE_LEVEL_DEBUG(3, "device %s does not support disabling IRQ\n",
                uni_dev->name);
        return 0;
    }
    /* pass through the register table to disable the IRQs */
    ret = process_reg_table(uni_dev, tbl->table, tbl->table_size);
    if (ret)
        return ret;
    /* if check_pending flag is set, check pending IRQ according to
     * either reading a device register or a runtime monitor (TODO) */
    if (disable_irq->check_pending) {
        /* check pending IRQ */
        u32 reg_value;
        if (disable_irq->pending.reg_op == RPM_REG_READ)
            universal_reg_read(uni_dev, disable_irq->pending.reg_offset,
                    &reg_value);
        else
            LJTALE_LEVEL_DEBUG(2, "pending check not read %s\n", uni_dev->name);

        if (reg_value ^ disable_irq->pending.compare_value)
            disable_irq->pending.pending = false;
        else
            disable_irq->pending.pending = true;
        
        /* after checking, the pending flag should be set */
        if (disable_irq->pending.pending) {
            /* reconfigure the IRQ according to the reconfigure table */
            tbl = &disable_irq->reconfigure_table;
            ret = process_reg_table(uni_dev, tbl->table, tbl->table_size);
            if (ret)
                return ret;
        } else {
            /* we are good, no pending IRQ handling*/
        }
    }
    return 0;
}

int universal_enable_irq(struct universal_device *uni_dev) {
    return 0;
};


int universal_pin_contrl(struct universal_device *uni_dev,
        enum rpm_action action) {
    struct universal_rpm *rpm = &uni_dev->drv->rpm;
    struct universal_pin_control *pin_control = rpm->pin_control;
    enum rpm_device_call pin_state = RPM_PINCTRL_DEFAULT;
    if (action == SUSPEND)
        pin_state = pin_control->suspend_state;
    else if (action == RESUME)
        pin_state = pin_control->resume_state;
    switch (pin_state) {
        case RPM_PINCTRL_DEFAULT:
            pinctrl_pm_select_default_state(uni_dev->dev);
            break;
        case RPM_PINCTRL_SLEEP:
            pinctrl_pm_select_sleep_state(uni_dev->dev);
            break;
        case RPM_PINCTRL_IDLE:
            pinctrl_pm_select_idle_state(uni_dev->dev);
            break;
        default:
            return -EINVAL;
    }
    return 0;
    /* we don't care about pin select return value */
}
