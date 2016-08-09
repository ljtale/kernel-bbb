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

/* device-specific structs that should be provided by each device */
struct omap_i2c_rpm_reg_value {
    u16 iestate;
    u16 westate;
    u16 pscstate;
    u16 scllstate;
    u16 sclhstate;
};

struct omap_hsmmc_rpm_context {
    u32 con;
    u32 hctl;
    u32 sysctl;
    u32 capa;
    u32 mmc_caps;
    u32 host_flags;
    spinlock_t irq_lock;
    u32 lock_flags;
};

int universal_runtime_suspend(struct device *dev);
int universal_runtime_resume(struct device *dev);
int universal_suspend(struct device *dev);
int universal_resume(struct device *dev);

int universal_rpm_create_reg_context(struct universal_device *uni_dev);

/* operation type of each step of runtime pm doing */
enum rpm_op {
    RPM_START,
    RPM_STOP,
    RPM_REG_WRITE,
    RPM_REG_READ,
    RPM_REG_WRITE_READ, /* read after write, flushing read */
    RPM_REG_READ_WRITE_OR, /* write value =  read value | value */
    RPM_REG_READ_WRITE_AND, /* write value = read value & value */
    RPM_REG_WRITE_AUG_OR,
    RPM_REG_WRITE_AUG_AND,
    RPM_PIN_STATE_SELECT, /* could be merged to DEVICE_CALL op */
    RPM_SPIN_LOCK,
    RPM_SPIN_UNLOCK,
    RPM_CONDITION,
    RPM_RETURN,
    RPM_DEVICE_CALL,
    RPM_BASIC_BLOCK,
    RPM_ASSIGNMENT,
    /* TODO: more rpm ops */
};

#if 0
enum rpm_pinctrl_state {
    RPM_PINCTRL_DEFAULT,
    RPM_PINCTRL_SLEEP,
    RPM_PINCTRL_IDLE,
};
#endif

enum rpm_device_call {
    RPM_MARK_LAST_BUSY,
    RPM_PINCTRL_DEFAULT,
    RPM_PINCTRL_SLEEP,
    RPM_PINCTRL_IDLE,
};

/* start and stop nodes could be only dummy nodes */
struct rpm_start_node {
};

struct rpm_stop_node {
};

struct rpm_reg_node {
    unsigned int reg_addr;
    u32 *reg_value;
};

struct rpm_spinlock_node {
    spinlock_t *lock;
    unsigned long *flags;
};

struct rpm_return_node {
    int *ret_value;
};

struct rpm_device_call_node {
    enum rpm_device_call call;
};

/* assignment node */
struct rpm_assignment_node {
    int *source;
    int value;
};

struct rpm_node {
    enum rpm_op op;
    void *op_args;
    struct rpm_node *next;
};

/* this node represent a basic block, which is a sequence of nodes without
 * branches. By this node,we don't need to redundantly build the node's next
 * pointers for each element of the basic block. */
struct rpm_basic_block {
    struct rpm_node *start; /* this should be an array start address */
    int node_num;
    struct rpm_node *end;
};

#define RPM_REG_READ_NODE(name, addr, value); \
    static struct rpm_reg_node rpm_reg_##name = { \
        .reg_addr = addr,   \
        .reg_value = (u32 *)value, \
    }; \
    static struct rpm_node rpm_node_##name = { \
        .op = RPM_REG_READ, \
        .op_args = &rpm_reg_##name, \
    };

#define RPM_REG_WRITE_NODE(name, addr, value); \
    static struct rpm_reg_node rpm_reg_##name = { \
        .reg_addr = addr,   \
        .reg_value = (u32 *)value, \
    }; \
    static struct rpm_node rpm_node_##name = { \
        .op = RPM_REG_WRITE, \
        .op_args = &rpm_reg_##name, \
    };
#if 0
#define RPM_PIN_STATE_SELECT_NODE(name, state); \
    static struct rpm_pinctrl_node rpm_pinctrl_##name = { \
        .pinctrl_state = state,   \
    }; \
    static struct rpm_node rpm_node_##name = { \
        .op = RPM_PIN_STATE_SELECT, \
        .op_args = &rpm_pinctrl_##name, \
    };
#endif

#define RPM_RETURN_NODE(name, value); \
    static struct rpm_return_node rpm_return_##name = { \
        .ret_value = value, \
    }; \
    static struct rpm_node rpm_node_##name = { \
        .op = RPM_RETURN, \
        .op_args = &rpm_return_##name, \
    };

#define RPM_DEVICE_CALL_NODE(name, fun); \
    static struct rpm_device_call_node rpm_device_call_##name = { \
        .call = fun, \
    }; \
    static struct rpm_node rpm_node_##name = { \
        .op = RPM_DEVICE_CALL, \
        .op_args = &rpm_device_call_##name, \
    };


#define RPM_SPINLOCK_NODE(name, lock); \
    static struct rpm_spinlock_node rpm_spinlock_##name = { \
    }; \
    static struct rpm_node rpm_node_##name = { \
        .op = lock, \
        .op_args = &rpm_spinlock_##name, \
    };

#define RPM_BASIC_BLOCK_NODE(name, node_start, num, node_end); \
    static struct rpm_basic_block rpm_basic_block_##name = { \
        .start = &RPM_NODE_NAME(node_start),    \
        .node_num = num,    \
        .end = &NAME_NODE_NAME(node_end), \
    };  \
    static struct rpm_node rpm_node_##name = { \
        .op = RPM_BASIC_BLOCK, \
        .op_args = &rpm_basic_block_##name, \
    };

#define RPM_ASSIGNMENT_NODE(name, assign_value); \
    static struct rpm_assignment_node rpm_assignment_##name = { \
        .value = assign_value, \
    }; \
    static struct rpm_node rpm_node_##name = { \
        .op = RPM_ASSIGNMENT, \
        .op_args = &rpm_assignment_##name, \
    };

#define RPM_NODE_NAME(name) rpm_node_##name
#define RPM_REG_NODE_NAME(name) rpm_reg_##name
#define RPM_SPINLOCK_NODE_NAME(name) rpm_spinlock_##name
#define RPM_BASIC_BLOCK_ARRAY(name) rpm_array_##name

enum rpm_condition_type {
    RPM_CONDITION_SIMPLE,
    RPM_CONDITION_LT,
    RPM_CONDITION_GT,
    RPM_CONDITION_LTEQ,
    RPM_CONDITION_GTEQ,
    RPM_CONDITION_EQ,
    RPM_CONDITION_BIT_AND,
    RPM_CONDITION_BIT_OR,
    RPM_CONDITION_BIT_NOT,
    RPM_CONDITION_AND,
    RPM_CONDITION_OR,
    RPM_CONDITION_NOT,
};

struct rpm_condition_op {
    enum rpm_condition_type type;
    /* for simple condition type, only left if valid, right is ignored */
    bool is_left_value;
    union {
        u32 *left_value;
        struct rpm_condition_op *left_op;
    };
    bool is_right_value;
    union {
        u32 *right_value;
        struct rpm_condition_op *right_op;
    };
};

struct rpm_condition_node {
    struct rpm_condition_op *op;
    struct rpm_node *true_path;
    struct rpm_node *false_path;
};

#define RPM_CONDITION_OP(name, con_type); \
    static struct rpm_condition_op rpm_con_op_##name = { \
        .type = con_type, \
    };

#define RPM_CONDITION_OP_NAME(name) rpm_con_op_##name
#define RPM_CONDITION_NODE_NAME(name) rpm_condition_##name

#define RPM_CONDITION_NODE(name, op_name); \
    static struct rpm_condition_node rpm_condition_##name = { \
        .op = &rpm_con_op_##op_name, \
    };  \
    static struct rpm_node rpm_node_##name = { \
       .op = RPM_CONDITION, \
       .op_args = &rpm_condition_##name, \
    };

#define RPM_NODE_CONTROL(name1, name2) \
    do { \
        rpm_node_##name1.next = &rpm_node_##name2; \
    } while (0)

#define RPM_CONDITION_CONTROL(condition_node, true_node, false_node) \
    do { \
        struct rpm_condition_node *node = \
            rpm_node_##condition_node.op_args; \
        node->true_path = &RPM_NODE_NAME(true_node); \
        node->false_path = &RPM_NODE_NAME(false_node); \
    } while(0)

#define RPM_CONDITION_OP_CONTROL(con_op, left, right) \
    do { \
        struct rpm_condition_op *op = &RPM_CONDITION_OP_NAME(con_op); \
        op->left_op = &RPM_CONDITION_OP_NAME(left); \
        op->right_op = &RPM_CONDITION_OP_NAME(right); \
    } while (0)


/* ====== data structure for generic logic ====== */
enum rpm_action {
    SUSPEND,
    RESUME,
    IDLE,
    AUTOSUSPEND,
};

int universal_disable_irq(struct universal_device *uni_dev);
int universal_enable_irq(struct universal_device *uni_dev);
int universal_pin_control(struct universal_device *uni_dev, 
        enum rpm_action action);

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
    unsigned long timeout;
};

struct universal_reg_entry {
    enum rpm_op reg_op;
    u32 reg_offset;
    int ctx_index;
    u32 write_augment;  /* this is a value to augment write operations */
    /* timing information for a register
     * Usually the reg op is write and timing check is specified, we need
     * to read the same register and compare against it to a pre-defined
     * value. Only when the value is stable (equal to the value) or a
     * timeout is triggered should we proceed. */
    struct reg_timeout timeout;
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
    enum rpm_op reg_op;
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
    enum rpm_device_call suspend_state;
    enum rpm_device_call resume_state;
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
};

struct universal_setup_wakeup {
};

struct universal_disable_clk {
};

struct universal_enable_clk {
};

#endif /* _LINUX_UNIVERSAL_RPM_H */
