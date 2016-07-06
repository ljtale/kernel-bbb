#ifndef _LINUX_UNIVERSAL_RPM_H
#define _LINUX_UNIVERSAL_RPM_H

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/of_device.h>
#include <linux/interrupt.h>
#include <linux/dmaengine.h>
#include <linux/omap-dmaengine.h>

#include <linux/i2c.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/platform_data/at24.h>

struct universal_driver;
struct universal_device;

struct universal_pm_ops {
    int (*suspend)(struct universal_device *uni_dev);
    int (*resume)(struct universal_device *uni_dev);
    int (*runtime_suspend)(struct universal_device *uni_dev);
    int (*runtime_resume)(struct universal_device *uni_dev);
};

/* operation type of each step of runtime pm doing */
enum rpm_op {
    RPM_START,
    RPM_STOP,
    RPM_REG_WRITE,
    RPM_REG_READ,
    RPM_PIN_STATE_SELECT,
    RPM_SPIN_LOCK,
    RPM_SPIN_UNLOCK,
    RPM_CONDITION,
    RPM_RETURN,
    /* TODO: more rpm ops */
};

enum rpm_pinctrl_state {
    RPM_PINCTRL_DEFAULT,
    RPM_PINCTRL_SLEEP,
    RPM_PINCTRL_IDEL,
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

struct rpm_pinctrl_node {
    enum rpm_pinctrl_state pinctrl_state;
};

struct rpm_spinlock_node {
    spinlock_t *lock;
    unsigned long flags;
};

struct rpm_node {
    enum rpm_op op;
    void *op_args;
};

#define RPM_REG_READ_NODE(name, addr, value); \
    static struct rpm_reg_node reg_node_##name = { \
        .reg_addr = addr,   \
        .reg_value = value, \
    }; \
    static struct rpm_node rpm_reg_read_node_##name = { \
        .op = RPM_REG_READ, \
        .op_args = &reg_node_##name, \
    };

#define RPM_REG_WRITE_NODE(name, addr, value); \
    static struct rpm_reg_node reg_node_##name = { \
        .reg_addr = addr,   \
        .reg_value = value, \
    }; \
    static struct rpm_node rpm_reg_read_node_##name = { \
        .op = RPM_REG_WRITE, \
        .op_args = &reg_node_##name, \
    };

#define RPM_PIN_STATE_SELECT_NODE(name, state); \
    static struct rpm_pinctrl_node rpm_pinctrl_##name = { \
        .pinctrl_state = state,   \
    }; \
    static struct rpm_node rpm_reg_read_node_##name = { \
        .op = RPM_PIN_STATE_SELECT, \
        .op_args = &rpm_pinctrl_##name, \
    };


enum rpm_condition_type {
    RPM_CONDITION_NONE,
    RPM_CONDITION_SIMPLE,
    RPM_CONDITION_LT,
    RPM_CONDITION_GT,
    RPM_CONDITION_EQ,
    RPM_CONDITION_BIT_AND,
    RPM_CONDITION_BIT_OR,
    RPM_CONDITION_AND,
    RPM_CONDITION_OR,
};

struct rpm_condition_value {
    unsigned value;
};
struct rpm_condition_op {
    enum rpm_condition_type type;
    /* for simple condition type, only left if valid */
    union {
        struct rpm_condition_value left_value;
        struct rpm_condition_op *left_op;
    };
    union {
        struct rpm_condition_value right_value;
        struct rpm_condition_op *right_op;
    };
};

struct rpm_condition_node {
    struct rpm_condition_op *op;
    struct rpm_node *true_path;
    struct rpm_node *false_path;
};

#endif /* _LINUX_UNIVERSAL_RPM_H */
