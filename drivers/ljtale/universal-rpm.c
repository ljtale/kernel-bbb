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
    struct universal_rpm_dev *rpm_dev;
    unsigned long flags;
    int ret;
    /* first find universal device */
    uni_dev = check_universal_driver(dev);
    if (!uni_dev) {
        LJTALE_MSG(KERN_ERR, "no universal driver for: %s\n", dev_name(dev));
        return 0;
    }
    rpm_dev = &uni_dev->rpm_dev;
    LJTALE_LEVEL_DEBUG(3, "universal rpm suspend...%s\n", uni_dev->name);
    spin_lock_irqsave(&uni_dev->drv->rpm_graph_lock, flags);
    if (uni_dev->drv->rpm_populate_suspend_graph)
        uni_dev->drv->rpm_populate_suspend_graph(uni_dev);
    /* Or the suspend graph is not populated or does not need to be */
    ret = universal_rpm_process_graph(uni_dev, rpm_dev->rpm_suspend_graph);
    spin_unlock_irqrestore(&uni_dev->drv->rpm_graph_lock, flags);
    return ret;
}

int universal_runtime_resume(struct device *dev) {
    struct universal_device *uni_dev;
    struct universal_rpm_dev *rpm_dev;
    unsigned long flags;
    int ret;
    /* first find universal device */
    uni_dev = check_universal_driver(dev);
    if (!uni_dev) {
        LJTALE_MSG(KERN_ERR, "no universal driver for: %s\n", dev_name(dev));
        return 0;
    }
    rpm_dev = &uni_dev->rpm_dev;
    LJTALE_LEVEL_DEBUG(3, "universal rpm resume...%s\n", uni_dev->name);
    spin_lock_irqsave(&uni_dev->drv->rpm_graph_lock, flags);
    if (uni_dev->drv->rpm_populate_resume_graph)
        uni_dev->drv->rpm_populate_resume_graph(uni_dev);
    ret = universal_rpm_process_graph(uni_dev, rpm_dev->rpm_resume_graph);
    spin_unlock_irqrestore(&uni_dev->drv->rpm_graph_lock, flags);
    return ret;
}
#endif

/*============== generic logic =====================*/

int universal_rpm_create_reg_context(struct universal_device *uni_dev) {
    struct universal_rpm *rpm = &uni_dev->drv->rpm;
    u32 *array = rpm->ref_ctx.array;
    int size = rpm->ref_ctx.size;
    int i;
    u32 *dev_array;
    if (!array)
        return 0;
    dev_array = devm_kzalloc(uni_dev->dev, sizeof(u32) * size, GFP_KERNEL);
    if (!dev_array)
        return -ENOMEM;
    /* copy everything including the constant values */
    for (i = 0; i < size; i++)
        dev_array[i] = array[i];

    uni_dev->rpm_dev.rpm_context.array = dev_array;
    uni_dev->rpm_dev.rpm_context.size = size;
    LJTALE_LEVEL_DEBUG(4, "%s successfully created reg context array\n",
            uni_dev->name);
    return 0;
}
EXPORT_SYMBOL(universal_rpm_create_reg_context);

static int inline process_reg_table(struct universal_device *uni_dev,
        struct universal_reg_entry *tbl, int table_size) {
    struct universal_reg_entry *tbl_entry;
    struct universal_rpm_ctx *reg_ctx = &uni_dev->rpm_dev.rpm_context;
    int ret = 0;
    int i;
    u32 temp_value;
    u32 timeout_compare = 0, timeout_read;
    unsigned long timeout;
    if (!tbl)
        return 0;
    for (i = 0; i < table_size; i++) {
        tbl_entry = &tbl[i];
        if (tbl_entry->reg_op == RPM_REG_WRITE_AUG_OR ||
                tbl_entry->reg_op == RPM_REG_WRITE_AUG_AND) {
            // compute the augment value before continuing
            if (tbl_entry->reg_write_augment_flag) {
                struct reg_write_augment *augment = 
                    &tbl_entry->reg_write_augment;
                /* FIXME: a generic computation method is needed */
                switch(augment->op) {
                    case REG_BIT_AND:
                        tbl_entry->write_augment = 
                            reg_ctx->array[augment->index1] & 
                            reg_ctx->array[augment->index2];
                        break;
                    case REG_BIT_OR:
                        tbl_entry->write_augment = 
                            reg_ctx->array[augment->index1] & 
                            reg_ctx->array[augment->index2];
                        break;
                        /* TODO: more ops */
                    default:
                        break;
                }
            }
        }
        switch(tbl_entry->reg_op) {
            case RPM_REG_READ:
                ret = universal_reg_read(uni_dev, tbl_entry->reg_offset,
                        &(reg_ctx->array[tbl_entry->ctx_index]));
                break;
            case RPM_REG_WRITE:
                ret = universal_reg_write(uni_dev, tbl_entry->reg_offset,
                        reg_ctx->array[tbl_entry->ctx_index]);
                if (tbl_entry->timeout.check_timeout) {
                    timeout_compare = reg_ctx->array[tbl_entry->ctx_index];
                    goto timeout_check;
                }
                break;
            case RPM_REG_WRITE_READ:
                /* a write followed by a read flush */
                ret = universal_reg_write(uni_dev, tbl_entry->reg_offset,
                        reg_ctx->array[tbl_entry->ctx_index]);
                if (ret)
                    break;
                ret = universal_reg_read(uni_dev, tbl_entry->reg_offset, NULL);
                break;
            case RPM_REG_READ_WRITE_OR:
                temp_value = 0;
                ret = universal_reg_read(uni_dev, tbl_entry->reg_offset,
                        &temp_value);
                if (ret)
                    break;
                ret = universal_reg_write(uni_dev, tbl_entry->reg_offset,
                        reg_ctx->array[tbl_entry->ctx_index] | temp_value);
                break;

            case RPM_REG_READ_WRITE_AND:
                temp_value = 0xffffffff;
                ret = universal_reg_read(uni_dev, tbl_entry->reg_offset,
                        &temp_value);
                if (ret)
                    break;
                ret = universal_reg_write(uni_dev, tbl_entry->reg_offset,
                        reg_ctx->array[tbl_entry->ctx_index] & temp_value);
                break;
            case RPM_REG_WRITE_AUG_OR:
                ret = universal_reg_write(uni_dev, tbl_entry->reg_offset,
                    (reg_ctx->array[tbl_entry->ctx_index] | 
                     tbl_entry->write_augment));
                if (tbl_entry->timeout.check_timeout) {
                    timeout_compare = 
                        reg_ctx->array[tbl_entry->ctx_index] | 
                        tbl_entry->write_augment;
                    goto timeout_check;
                }
                break;
            case RPM_REG_WRITE_AUG_AND:
                ret = universal_reg_write(uni_dev, tbl_entry->reg_offset,
                    (reg_ctx->array[tbl_entry->ctx_index] &
                     tbl_entry->write_augment));
                if (tbl_entry->timeout.check_timeout) {
                    timeout_compare = 
                        reg_ctx->array[tbl_entry->ctx_index] & 
                        tbl_entry->write_augment;
                    goto timeout_check;
                }
                break;

            default:
                /* unsupported yet */
                LJTALE_LEVEL_DEBUG(3, "unsupported op for %s\n",
                        uni_dev->name);
                break;
        }
timeout_check:
        if (tbl_entry->timeout.check_timeout) {
            /* assume timeout is microsecond */
            timeout = jiffies + msecs_to_jiffies(tbl_entry->timeout.timeout);
            ret =  universal_reg_read(uni_dev, 
                    tbl_entry->reg_offset, &timeout_read);
            while(time_before(jiffies, timeout) && 
                  timeout_read != timeout_compare)
                ret = universal_reg_read(uni_dev, tbl_entry->reg_offset,
                        &timeout_read);
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
static int universal_disable_irq(struct universal_device *uni_dev) {
    struct universal_rpm *rpm = &uni_dev->drv->rpm;
    struct universal_disable_irq *disable_irq = rpm->disable_irq; 
    /* assume when calling this function, the reg context has been created */
    struct universal_disable_irq_tbl *tbl;
    int ret = 0;
    if (!rpm->disable_irq) {
        LJTALE_LEVEL_DEBUG(3, "device %s does not support disabling IRQ\n",
                uni_dev->name);
        return 0;
    }
    tbl = &disable_irq->disable_table;
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

        if (reg_value & disable_irq->pending.compare_value)
            /* reg value is high, fine */
            disable_irq->pending.pending = false ;
        else
            /* reg value is low */
            disable_irq->pending.pending = true;
        
        /* after checking, the pending flag should be set */
        if (disable_irq->pending.pending) 
            return -EBUSY;
    }
     
    /* we are good, no pending IRQ handling*/
    /* pass through the register table to disable the IRQs */
    ret = process_reg_table(uni_dev, tbl->table, tbl->table_size);
    return ret;
}

static int universal_enable_irq(struct universal_device *uni_dev) {
    struct universal_rpm *rpm = &uni_dev->drv->rpm;
    struct universal_enable_irq *enable_irq = rpm->enable_irq;
    struct universal_enable_irq_tbl *tbl;
    if (!rpm->enable_irq) {
        LJTALE_LEVEL_DEBUG(3, "device %s does not support enabling IRQ\n",
                uni_dev->name);
        return 0;
    }
    tbl = &enable_irq->enable_table;
    return process_reg_table(uni_dev, tbl->table, tbl->table_size);
};


static int universal_pin_control(struct universal_device *uni_dev,
        enum rpm_action action) {
    struct universal_rpm *rpm = &uni_dev->drv->rpm;
    struct universal_pin_control *pin_control = rpm->pin_control;
    enum rpm_device_call pin_state = RPM_PINCTRL_DEFAULT;
    if (!pin_control)
        return 0;
    switch(action) {
        case SUSPEND:
            pin_state = pin_control->suspend_state;
            break;
        case RESUME:
            pin_state = pin_control->resume_state;
            break;
        case IDLE:
            pin_state = RPM_PINCTRL_IDLE;
            break;
        case AUTOSUSPEND:
            pin_state = pin_control->suspend_state;
            break;
        default:
            break;
    }
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

static int universal_save_context(struct universal_device *uni_dev) {
    struct universal_rpm *rpm = &uni_dev->drv->rpm;
    struct universal_save_context_tbl *tbl = rpm->save_context.save_tbl;
    struct universal_rpm_dev *rpm_dev = &uni_dev->rpm_dev;
    int ret = 0;
    if (rpm_dev->save_context_once && !rpm_dev->context_saved) {
        ret = process_reg_table(uni_dev, tbl->table, tbl->table_size);
        if (ret)
            return ret;
        rpm_dev->context_saved = true;
        return 0;
    }
    return process_reg_table(uni_dev, tbl->table, tbl->table_size);
}

/* check if there is a context loss for a device
 * return true if there is a context loss, otherwise return false */
static bool universal_check_context_loss(struct universal_device *uni_dev) {
    /* similar to the register table process, except that we only
     * read registers */
    struct universal_rpm *rpm = &uni_dev->drv->rpm;
    struct universal_rpm_ctx *reg_ctx = &uni_dev->rpm_dev.rpm_context;
    /* we assume check_context_loss has been set */
    struct universal_restore_context_tbl *check_ctx_loss_tbl = 
        rpm->restore_context.check_ctx_loss_tbl;
    struct universal_reg_entry *reg_entry;
    int table_size, i;
    u32 temp_value;
    BUG_ON(!check_ctx_loss_tbl);
    if(!check_ctx_loss_tbl->table)
        return false;
    table_size = check_ctx_loss_tbl->table_size;
    for (i = 0; i < table_size; i++) {
        reg_entry = &check_ctx_loss_tbl->table[i];
        if (universal_reg_read(uni_dev, reg_entry->reg_offset, &temp_value))
            return false;
        /* compare the regsiter values against the saved context */
        if (temp_value != reg_ctx->array[reg_entry->ctx_index])
            return true;
    }
    return false;
}

static int universal_restore_context(struct universal_device *uni_dev) {
    int ret = 0;
    struct universal_rpm *rpm = &uni_dev->drv->rpm;
    struct universal_restore_context *restore_context =
        &rpm->restore_context;
    struct universal_restore_context_tbl *tbl = 
        restore_context->restore_tbl;
    /* for context restoring, we follow these procedures:
     * 1) check context if there is a context loos 
     * 2) if there is a context loss restore the context 
     * 3) if local context restore is required, call local context restore */
    if (restore_context->check_context_loss) {
        if (!universal_check_context_loss(uni_dev))
            /* if there is no context loss, we are safe to go */
            return 0;
        else
            uni_dev->rpm_dev.context_loss_cnt++;
        /* else there is a context loss, restore the context */
    }
    /* process the restore table to restore the context */
    ret = process_reg_table(uni_dev, tbl->table, tbl->table_size);
    if (ret)
        return ret;
    /* call local context restore callback */
    if (restore_context->rpm_local_restore_context)
        return restore_context->rpm_local_restore_context(uni_dev);
    return ret;
}

/* IMPORTATN NOTE:
 * DMA disable/enable don't really need rpm knowledge from the conventional
 * driver, because some information has been passed through DT and others
 * are got from universal probe, e.g., dma channels. 
 * For a full DMA disable, we need to call dmaengine_terminate_all() to
 * to terminate the dma channel, but here maybe data loss on the running
 * channel, thus we have to have device-specific callbacks to copy/save
 * the DMA buffer to prevent data loss. However, this is very burden-intensive.
 * Moreover, during full DMA enable, we need to reconfigure the DMA channels
 * such as setting completion callback, submit DMA request, which is also
 * burden-intensive. Therefore, full disable/enable DMA is not a good option
 * for us.
 * Newer kernel versions (>= 4.6) provide dmaengine_terminate_sync() to
 * completely terminate the dma channel but make sure the data is not lost
 * (synchronized). If the DMA engine supports this, then this is a good 
 * feature to do runtime DMA disabling/enabling. To prevent the DMA
 * synchronization from blocking the kernel, we can add a timeout mechanism
 * to make the runtime suspend return after a certain amount of time.
 * For a simpler version of runtime DMA disabling/enabling, we use the 
 * dmaengine_pause/resume() calls to pause the DMA channels and resume them
 * later without reconfiguring DMA stuff. We can rely on the call
 * dma_async_is_complete() to check the status of the last cookie sent by
 * dmaengine_submitt (I demonstrated this a little bit for MMC controller).*/

static int universal_rpm_disable_dma(struct universal_device *uni_dev) {
    struct universal_rpm_dev *rpm_dev = &uni_dev->rpm_dev;
    /* we rely on the per-device probe states to do runtime PM */
    struct universal_probe_dev *probe_dev = &uni_dev->probe_dev;
    struct dma_config_dev_num *dma_config_dev_num = 
        &probe_dev->dma_config_dev_num;
    struct dma_config_dev *dma_config_dev;
    int i;
    /* only when the device supports DMA and DMA channel is requested
     * should we perform DMA pausing */
    if (!rpm_dev->support_dma || !rpm_dev->dma_channel_requested)
        return 0;
    dma_config_dev = dma_config_dev_num->dma_config_dev;
    for (i = 0; i < dma_config_dev_num->dma_num; i++) {
        /* we should check the last cookie returned by dmaengine_submitt().
         * If the cookie is IN_PROGRESS or ERROR, we should abor.
         * This is an inaccurate estimation of the channel activities */
        /* TODO: this is not critical right now, we do it later */
        if (!dma_config_dev[i].chan_paused &&
                dma_config_dev[i].channel) {
            dmaengine_pause(dma_config_dev[i].channel);
            dma_config_dev[i].chan_paused = true;
            LJTALE_LEVEL_DEBUG(3, "%s dma channel %d paused\n",
                    uni_dev->name, i);
        }
    }
    return 0;
#if 0
abort:
    for (i = 0; i < dma_config_dev->dma_num; i++) {
        if (dma_config_dev[i].chan_paused &&
                dma_config_dev[i].channel)
            dmaengine_resume(dma_config_dev[i].channel);
    }
    return -EBUSY;
#endif 
}

static int universal_rpm_enable_dma(struct universal_device *uni_dev) {
    struct universal_rpm_dev *rpm_dev = &uni_dev->rpm_dev;
    struct universal_probe_dev *probe_dev = &uni_dev->probe_dev;
    struct dma_config_dev_num *dma_config_dev_num = 
        &probe_dev->dma_config_dev_num;
    struct dma_config_dev *dma_config_dev;
    int i;
    /* only when the device supports DMA and DMA channel is requested
     * should we perform DMA pausing */
    if (!rpm_dev->support_dma || !rpm_dev->dma_channel_requested)
        return 0;
    dma_config_dev = dma_config_dev_num->dma_config_dev;
    for (i = 0; i < dma_config_dev_num->dma_num; i++) {
        /* TODO: if the channel is fully terminated, we need device-specific
         * callback to reconfigure the channels */
        if (dma_config_dev[i].chan_paused &&
                dma_config_dev[i].channel) {
            dmaengine_resume(dma_config_dev[i].channel);
            dma_config_dev[i].chan_paused = false;
            LJTALE_LEVEL_DEBUG(3, "%s dma channel %d resumed\n",
                    uni_dev->name, i);
        }
    }
    return 0;
}

static int universal_setup_wakeup(struct universal_device *uni_dev) {
    struct universal_rpm *rpm = &uni_dev->drv->rpm;
    struct universal_reg_entry *reg_tbl = rpm->setup_wakeup->reg_table;
    int table_size = rpm->setup_wakeup->table_size;
    return process_reg_table(uni_dev, reg_tbl, table_size);
}

static int universal_disable_clk(struct universal_device *uni_dev) {
   struct universal_rpm *rpm = &uni_dev->drv->rpm;
   struct universal_probe_dev *probe_dev = &uni_dev->probe_dev;
   struct universal_reg_entry *reg_tbl = rpm->disable_clk->reg_table;
   int table_size = rpm->disable_clk->table_size;
   struct clk_config_dev_num *clk_num = &probe_dev->clk_config_dev_num;
   int ret;
   int i;
   ret = process_reg_table(uni_dev, reg_tbl, table_size);
   if (ret)
       return ret;
   else {
       for (i = 0; i < clk_num->clk_num; i++) {
           if (clk_num->clk_config_dev[i].clock_flag)
               clk_disable(clk_num->clk_config_dev[i].clk);
       }
   }
   return 0;
}

static int universal_enable_clk(struct universal_device *uni_dev) {
   struct universal_rpm *rpm = &uni_dev->drv->rpm;
   struct universal_probe_dev *probe_dev = &uni_dev->probe_dev;
   struct clk_config_dev_num *clk_num = &probe_dev->clk_config_dev_num;
   struct universal_reg_entry *reg_tbl = rpm->disable_clk->reg_table;
   int table_size = rpm->disable_clk->table_size;
   int ret;
   int i;
   for (i = 0; i < clk_num->clk_num; i++) {
       if (clk_num->clk_config_dev[i].clock_flag)
           clk_enable(clk_num->clk_config_dev[i].clk);
   }
   ret = process_reg_table(uni_dev, reg_tbl, table_size);
   if (ret)
       return ret;
   return 0;

}

int universal_runtime_suspend(struct device *dev) {
    struct universal_device *uni_dev;
    struct universal_rpm_dev *rpm_dev;
    struct universal_rpm_ops *rpm_ops;
    unsigned long irq_lock_flags = 0;
    unsigned long dev_lock_flags = 0;
    int ret = 0;
    uni_dev = check_universal_driver(dev);
    if (!uni_dev) {
        LJTALE_MSG(KERN_ERR, "no universal driver for: %s\n", dev_name(dev));
        return 0;
    }
    rpm_dev = &uni_dev->rpm_dev;
    rpm_ops = &uni_dev->drv->rpm_ops;
    LJTALE_LEVEL_DEBUG(3, "universal rpm suspend...%s\n", uni_dev->name);

    /* device access lock */
    /* TODO: we should build a universal locking mechanism for all type of
     * locks */
    if (rpm_dev->dev_access_needs_spinlock)
        spin_lock_irqsave(&uni_dev->probe_dev.spinlock, dev_lock_flags);
    else if (rpm_dev->dev_access_needs_raw_spinlock)
        raw_spin_lock_irqsave(&uni_dev->probe_dev.raw_spinlock, dev_lock_flags);

    /* save context */
    ret = universal_save_context(uni_dev);
    if (ret) {
        LJTALE_LEVEL_DEBUG(3, "universal save context failed: %d\n", ret);
        goto lock_err;
    }

    /* disable dma */
    ret = universal_rpm_disable_dma(uni_dev);
    if (ret) {
        LJTALE_LEVEL_DEBUG(3, "universal disable dma failed: %d\n",ret);
        goto lock_err;
    }
    /* disable irq */
    /* If the irq needs a lock to protect mutex access, then the lock should
     * protect until the pinctrl state is selected */
    if (rpm_dev->irq_need_lock)
        spin_lock_irqsave(&rpm_dev->irq_lock, irq_lock_flags);

    ret = universal_disable_irq(uni_dev);
    if (ret) {
        LJTALE_LEVEL_DEBUG(3 ,"universal disable irq failed: %d\n", ret);
        goto irq_lock_err;
    }
    /* select pinctrl state */
    universal_pin_control(uni_dev, SUSPEND);

    /* call local suspend within lock */
    if (rpm_dev->local_suspend_lock && rpm_ops->local_runtime_suspend) {
        ret = rpm_ops->local_runtime_suspend(uni_dev->dev);
        if (ret)
            goto irq_lock_err;
    }
irq_lock_err:
    if (rpm_dev->irq_need_lock)
        spin_unlock_irqrestore(&rpm_dev->irq_lock, irq_lock_flags);

    /* in case local suspend does not need a lock */
    if (!rpm_dev->local_suspend_lock && rpm_ops->local_runtime_suspend)
        ret = rpm_ops->local_runtime_suspend(uni_dev->dev);
        if (ret)
            goto lock_err;

    /* setup wakeup event */
    ret = universal_setup_wakeup(uni_dev);
    if (ret)
        goto lock_err;
    ret = universal_disable_clk(uni_dev);
    if (ret)
        goto lock_err;
lock_err:

    if (rpm_dev->dev_access_needs_spinlock)
        spin_unlock_irqrestore(&uni_dev->probe_dev.spinlock, dev_lock_flags);
    else if (rpm_dev->dev_access_needs_raw_spinlock)
        raw_spin_unlock_irqrestore(&uni_dev->probe_dev.raw_spinlock, 
                dev_lock_flags);

    return ret;
}

int universal_runtime_resume(struct device *dev) {
    struct universal_device *uni_dev;
    struct universal_rpm_dev *rpm_dev;
    struct universal_rpm_ops *rpm_ops;
    unsigned long irq_lock_flags = 0;
    unsigned long dev_lock_flags = 0;
    int ret = 0;
    uni_dev = check_universal_driver(dev);
    if (!uni_dev) {
        LJTALE_MSG(KERN_ERR, "no universal driver for: %s\n", dev_name(dev));
        return 0;
    }
    rpm_dev = &uni_dev->rpm_dev;
    rpm_ops = &uni_dev->drv->rpm_ops;
    LJTALE_LEVEL_DEBUG(3, "universal rpm resume...%s\n", uni_dev->name);
    /* Make sure universal runtime resume is not the first resume for
     * the device. Because the context for runtime suspend/resume does
     * not depend on probe context, universal runtime suspend must
     * be called before universal resume to build the context. */

    /* TODO: if we want to put the first resume in the universal resume,
     * driver developers should define the first context that is used
     * to configure the device to an initial states */
    if (!rpm_dev->first_resume_called) {
        BUG_ON(!rpm_ops->first_runtime_resume);
        LJTALE_LEVEL_DEBUG(3 ,"first rpm resume from conventional driver: %s\n",
                uni_dev->name);
        ret = rpm_ops->first_runtime_resume(dev);
        rpm_dev->first_resume_called = true;
        return ret;
    }
    /* not the first time, do generic procedures */
    /* FIXME: assume the first assume provides its own lock, but this lock
     * should be the same ones defined in uni_dev or rpm_dev. */
    if (rpm_dev->dev_access_needs_spinlock)
        spin_lock_irqsave(&uni_dev->probe_dev.spinlock, dev_lock_flags);
    else if (rpm_dev->dev_access_needs_raw_spinlock)
        raw_spin_lock_irqsave(&uni_dev->probe_dev.raw_spinlock, dev_lock_flags);
    
    /* restore the context */
    ret = universal_restore_context(uni_dev);
    if (ret) {
        LJTALE_LEVEL_DEBUG(3, "universal restore context failed: %d\n", ret);
        goto lock_err;
    }
    /* enable clock */
    ret = universal_enable_clk(uni_dev);
    if (ret)
        goto irq_lock_err;

    if (rpm_dev->irq_need_lock)
        spin_lock_irqsave(&rpm_dev->irq_lock, irq_lock_flags);
    /* select pinctrl state */
    ret = universal_pin_control(uni_dev, RESUME);
    /* enable irq */
    ret = universal_enable_irq(uni_dev);
    if (ret)
        goto irq_lock_err;

    /* TODO: device-specific configuration logic */

    if (rpm_dev->local_resume_lock && rpm_ops->local_runtime_resume) {
        ret = rpm_ops->local_runtime_resume(uni_dev->dev);
        if (ret)
            goto irq_lock_err;
    }
irq_lock_err:
    if (rpm_dev->irq_need_lock)
        spin_unlock_irqrestore(&rpm_dev->irq_lock, irq_lock_flags);

    /* enable dma */
    ret = universal_rpm_enable_dma(uni_dev);
    if (ret) {
        LJTALE_LEVEL_DEBUG(3, "universal enable dma failed: %d\n",ret);
        goto lock_err;
    }

    /* in case the local resume does not need lock */
    if (!rpm_dev->local_resume_lock && rpm_ops->local_runtime_resume)
        ret = rpm_ops->local_runtime_resume(uni_dev->dev);
lock_err:

    if (rpm_dev->dev_access_needs_spinlock)
        spin_unlock_irqrestore(&uni_dev->probe_dev.spinlock, dev_lock_flags);
    else if (rpm_dev->dev_access_needs_raw_spinlock)
        raw_spin_unlock_irqrestore(&uni_dev->probe_dev.raw_spinlock, 
                dev_lock_flags);

    return ret;
}

