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
    RPM_NONE = 0,
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

struct rpm_reg_read_node {
    unsigned int reg_addr;
    u32 *reg_value;
};

struct rpm_reg_write_node {
    unsigned int reg_addr;
    u32 reg_value;
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
    struct rpm_node *next;
};

#define RPM_REG_READ_NODE(name, addr, value); \
    static struct rpm_reg_node reg_node_##name = { \
        .reg_addr = addr,   \
        .reg_value = (u32 *)value, \
    }; \
    static struct rpm_node rpm_node_##name = { \
        .op = RPM_REG_READ, \
        .op_args = &reg_node_##name, \
    };

#define RPM_DECLARE_REG_NODE(name); \
    struct rpm_reg_node *reg_node_##name; \
    struct rpm_node *rpm_node_##name;

#define RPM_ALLOCATE_REG_READ_NODE(dev, name, addr, value)  \
    do {                                                            \
        reg_node_##name = devm_kzalloc(dev,    \
                sizeof(struct rpm_reg_node), GFP_KERNEL);           \
        if (reg_node_##name) {                                     \
            reg_node_##name->reg_addr = addr;                           \
            reg_node_##name->reg_value = (u32 *)value;                  \
        } else                                                        \
            BUG_ON(1); \
        rpm_node_##name = devm_kzalloc(dev,        \
               sizeof(struct rpm_node), GFP_KERNEL);                \
       if (rpm_node_##name) {                                      \
           rpm_node_##name->op = RPM_REG_READ;                  \
           rpm_node_##name->op_args = reg_node_##name;              \
       } else                                                       \
           BUG_ON(1);                                               \
    } while(0)

#define RPM_ALLOCATE_REG_WRITE_NODE(dev, name, addr, value)  \
    do {                                                            \
        reg_node_##name = devm_kzalloc(dev,    \
                sizeof(struct rpm_reg_node), GFP_KERNEL);           \
        if (reg_node_##name) {                                     \
            reg_node_##name->reg_addr = addr;                           \
            reg_node_##name->reg_value = (u32 *)value;                  \
        } else                                                        \
            BUG_ON(1); \
        rpm_node_##name = devm_kzalloc(dev,        \
               sizeof(struct rpm_node), GFP_KERNEL);                \
       if (rpm_node_##name) {                                      \
           rpm_node_##name->op = RPM_REG_WRITE;                     \
           rpm_node_##name->op_args = reg_node_##name;              \
       } else                                                       \
           BUG_ON(1);                                               \
    } while(0)


#define RPM_REG_WRITE_NODE(name, addr, value); \
    static struct rpm_reg_node reg_node_##name = { \
        .reg_addr = addr,   \
        .reg_value = (u32 *)value, \
    }; \
    static struct rpm_node rpm_node_##name = { \
        .op = RPM_REG_WRITE, \
        .op_args = &reg_node_##name, \
    };

#define RPM_DECLARE_PIN_STATE_SELECT_NODE(name); \
    struct rpm_pinctrl_node *rpm_pinctrl_##name; \
    struct rpm_node *rpm_node_##name;

#define RPM_ALLOCATE_PIN_STATE_SELECT_NODE(dev, name, state)  \
    do {                                                            \
        rpm_pinctrl_##name = devm_kzalloc(dev,    \
                sizeof(struct rpm_pinctrl_node), GFP_KERNEL);           \
        if (rpm_pinctrl_##name)                                     \
            rpm_pinctrl_##name->pinctrl_state = state;                  \
        else  \
            BUG_ON(1); \
        rpm_node_##name = devm_kzalloc(dev,        \
               sizeof(struct rpm_node), GFP_KERNEL);                \
       if (rpm_node_##name) {                                      \
           rpm_node_##name->op = RPM_PIN_STATE_SELECT;  \
           rpm_node_##name->op_args = rpm_pinctrl_##name;              \
       } else                                                       \
           BUG_ON(1);                                               \
    } while(0)


#define RPM_PIN_STATE_SELECT_NODE(name, state); \
    static struct rpm_pinctrl_node rpm_pinctrl_##name = { \
        .pinctrl_state = state,   \
    }; \
    static struct rpm_node rpm_node_##name = { \
        .op = RPM_PIN_STATE_SELECT, \
        .op_args = &rpm_pinctrl_##name, \
    };

#define RPM_NODE_NAME(name) rpm_node_##name
#define RPM_REG_NODE_NAME(name) reg_node_##name

enum rpm_condition_type {
    RPM_CONDITION_SIMPLE,
    RPM_CONDITION_LT,
    RPM_CONDITION_GT,
    RPM_CONDITION_LTEQ,
    RPM_CONDITION_GTEQ,
    RPM_CONDITION_EQ,
    RPM_CONDITION_BIT_AND,
    RPM_CONDITION_BIT_OR,
    RPM_CONDITION_AND,
    RPM_CONDITION_OR,
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


#define RPM_DECLARE_CONDITION_OP(name); \
    struct rpm_condition_op *rpm_con_op_##name;

#define RPM_ALLOCATE_CONDITION_OP(dev, name, con_type)  \
do {                                                            \
    rpm_con_op_##name = devm_kzalloc(dev,    \
            sizeof(struct rpm_condition_op), GFP_KERNEL);           \
    if (rpm_con_op_##name)                                     \
        rpm_con_op_##name->type = con_type;                  \
    else  \
        BUG_ON(1); \
} while(0)

#define RPM_DECLARE_CONDITION_NODE(name); \
    struct rpm_condition_node *rpm_condition_##name; \
    struct rpm_node *rpm_node_##name;

#define RPM_ALLOCATE_CONDITION_NODE(dev, name, op_name)  \
do {                                                            \
    rpm_condition_##name = devm_kzalloc(dev,    \
            sizeof(struct rpm_condition_node), GFP_KERNEL);           \
    if (rpm_condition_##name)                                     \
        rpm_condition_##name->op = rpm_con_op_##op_name;                  \
    else  \
        BUG_ON(1); \
    rpm_node_##name = devm_kzalloc(dev,        \
           sizeof(struct rpm_node), GFP_KERNEL);                \
   if (rpm_node_##name) {                                      \
       rpm_node_##name->op = RPM_CONDITION;  \
       rpm_node_##name->op_args = rpm_condition_##name;              \
   } else                                                       \
       BUG_ON(1);                                               \
} while(0)



#define RPM_NODE_CONTROL(name1, name2) \
    do { \
        rpm_node_##name1->next = rpm_node_##name2; \
    } while (0)

/* the following macros are used only when all the arguments are valid */
#define RPM_CONDITION_OP_CONTROL(op_name, left, right) \
    do { \
        rpm_con_op_##op_name->left_op = rpm_con_op_left; \
        rpm_con_op_##op_name->right_op = rpm_con_op_right; \
    } while(0)

#define RPM_CONDITION_CONTROL(condition_node, true_node, false_node) \
    do { \
        struct rpm_condition_node *node = rpm_node_##condition_node; \
        node->true_path = RPM_NODE_NAME(true_node); \
        node->false_path = RPM_NODE_NAME(false_node); \
    } while(0)

#endif /* _LINUX_UNIVERSAL_RPM_H */
