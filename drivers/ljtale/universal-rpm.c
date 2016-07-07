/*
 * universal driver runtime pm source file @ljtale
 * */

#include <linux/universal-drv.h>
#include <linux/universal-utils.h>
#include <linux/universal-rpm.h>


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
                break;
            case RPM_REG_WRITE:
                break;
            case RPM_PIN_STATE_SELECT:
                break;
            case RPM_CONDITION:
                break;
            case RPM_SPIN_LOCK:
                break;
            case RPM_SPIN_UNLOCK:
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
    struct rpm_node *rpm_suspend_graph;
    /* first find universal device */
    uni_dev = check_universal_driver(dev);
    if (!uni_dev) {
        LJTALE_MSG(KERN_ERR, "no universal driver for: %s\n", dev_name(dev));
        return 0;
    }
    LJTALE_LEVEL_DEBUG(3, "universal rpm suspend...%s\n", uni_dev->name);
    rpm_suspend_graph = uni_dev->rpm_suspend_graph;
    return universal_rpm_process_graph(uni_dev, rpm_suspend_graph);
}
