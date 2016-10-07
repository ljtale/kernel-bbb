#ifndef _LINUX_UNIVERSAL_RPM_H
#define _LINUX_UNIVERSAL_RPM_H

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/of_device.h>
#include <linux/interrupt.h>
#include <linux/dmaengine.h>
#include <linux/omap-dmaengine.h>
#include <linux/pm_runtime.h>

#include <linux/i2c.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/platform_data/at24.h>

struct universal_driver;
struct universal_device;

int universal_runtime_suspend(struct device *dev);
int universal_runtime_resume(struct device *dev);

int universal_rpm_create_reg_context(struct universal_device *uni_dev);

/* operation type of each step of runtime pm doing */
enum pm_reg_op {
    PM_REG_WRITE,
    PM_REG_READ,
    PM_REG_WRITE_READ, /* read after write, flushing read */
    PM_REG_READ_WRITE_OR, /* write value =  read value | write_augment */
    PM_REG_READ_WRITE_AND, /* write value = read value & write_augment */
    PM_REG_WRITE_AUG_OR,
    PM_REG_WRITE_AUG_AND,
    PM_REG_WRITE_BITS, /* write value | aug, then write value & ~aug */
};

enum reg_context_op {
    REG_BIT_AND,
    REG_BIT_OR,
    REG_BIT_NOT, // unary operation
    REG_AND,
    REG_OR,
    REG_NOT,    // unary operation
};

enum pm_device_call {
    RPM_MARK_LAST_BUSY,
    PM_PINCTRL_DEFAULT,
    PM_PINCTRL_SLEEP,
    PM_PINCTRL_IDLE,
};

/* ====== data structure for generic logic ====== */
enum pm_action {
    PREPARE,
    COMPLETE,
    SUSPEND,
    RESUME,
    FREEZE,
    THAW,
    SUSPEND_LATE,
    RESUME_EARLY,
    FREEZE_LATE,
    THAW_EARLY,
    SUSPEND_NOIRQ,
    RESUME_NOIRQ,
    FREEZE_NOIRQ,
    THAW_NOIRQ,
    RUNTIME_SUSPEND,
    RUNTIME_RESUME,
    RUNTIME_IDLE,
};

#define INVALID_INDEX -1

/* a reference array for register context, individual device needs
 * to create a per-device context based on this reference */
struct universal_rpm_ctx {
    u32 *array;
    int size;
};

struct reg_timeout {
    bool check_timeout;
    u32 compare_value;
    /* assume timeout is microseconds */
    unsigned long timeout;
};

struct reg_write_augment {
    /* TODO: define a generic computation method to get this augment value */
    int index1;
    int index2;
    enum reg_context_op op;
};

struct universal_reg_entry {
    enum pm_reg_op reg_op;
    u32 reg_offset;
    int ctx_index;
    u32 write_augment;  /* this is a value to augment write operations */

    /* timing information for a register
     * Usually the reg op is write and timing check is specified, we need
     * to read the same register and compare against it to a pre-defined
     * value. Only when the value is stable (equal to the value) or a
     * timeout is triggered should we proceed. */
    struct reg_timeout timeout;

    // For some devices, the write_augment value comes from the register
    // context, thus this augment value should be specified by device vendor
    bool reg_write_augment_flag;
    struct reg_write_augment reg_write_augment;
};

struct universal_save_context_tbl {
    struct universal_reg_entry *table;
    int table_size;
};

struct universal_restore_context_tbl {
    struct universal_reg_entry *table;
    int table_size;
};

struct universal_enable_irq_tbl {
    struct universal_reg_entry *table;
    int table_size;
};

struct universal_disable_irq_tbl {
    struct universal_reg_entry *table;
    int table_size;
};

/* Pending IRQ handling could be checked at runtime, just like Chao's work
 * to check device using by memory monitor. To check pending IRQ at runtime,
 * we can rely on the IRQ table to intercept any registered interrupt handlers.
 * On a per-device manner, checking ending IRQ could also be done by reading 
 * a specific register and compare the register value with a certain value.*/
struct universal_check_pending_irq {
    /* reg_op by default is register read */
    enum pm_reg_op reg_op;
    u32 reg_offset;
    u32 compare_value;
    bool pending;
};

struct universal_disable_irq {
    bool check_pending;
    struct universal_check_pending_irq pending;
    struct universal_disable_irq_tbl disable_table;
};

struct universal_enable_irq {
    struct universal_enable_irq_tbl enable_table;
};

struct universal_pin_control {
    enum pm_device_call suspend_state;
    enum pm_device_call resume_state;
    enum pm_device_call idle_state;
    enum pm_device_call freeze_state;
    enum pm_device_call thaw_state;
    enum pm_device_call suspend_noirq_state;
    enum pm_device_call resume_noirq_state;
};

struct universal_save_context {
    struct universal_save_context_tbl *save_tbl;
};

/* restoring context is a little bit complex, besides writing whatever
 * is saved during suspening, some device specific context has to be
 * restored. We pass a callback to the conventional driver to do that */
struct universal_restore_context {
    bool check_context_loss;
    struct universal_restore_context_tbl *check_ctx_loss_tbl;
    struct universal_restore_context_tbl *restore_tbl;
    int (*rpm_local_restore_context)(struct universal_device *uni_dev);
    int (*pm_local_restore_context)(struct universal_device *uni_dev);
};

struct universal_setup_wakeup {
    // setup wakeup in rpm is likely to be GPIO-specific, essentially change
    // level-triggered pins to be edge-triggered in order to receive wakeup
    // interrupts. According to what is saved during suspend we can know what
    // registers to write to setup wakeup events
    struct universal_reg_entry *reg_table;
    int table_size;
};

struct universal_reset_wakeup {
};

struct universal_disable_clk {
    struct universal_reg_entry *reg_table;
    int table_size;
};

struct universal_enable_clk {
    struct universal_reg_entry *reg_table;
    int table_size;
};

#endif /* _LINUX_UNIVERSAL_RPM_H */
