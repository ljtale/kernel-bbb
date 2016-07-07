/*
 * universal driver runtime pm source file @ljtale
 * */

#include <linux/universal-drv.h>
#include <linux/universal-utils.h>
#include <linux/universal-rpm.h>


int universal_rpm_process_graph(struct rpm_node *graph) {
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
    rpm_suspend_graph = uni_dev->rpm_suspend_graph;
    if (!rpm_suspend_graph)
        return 0;
    else
        return universal_rpm_process_graph(rpm_suspend_graph);
}
