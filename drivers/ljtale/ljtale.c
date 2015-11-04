#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/of.h>


static const struct of_device_id ljtale_device_match[] = {
    {.compatible = "ljtale,device",},
    {}
};

MODULE_DEVICE_TABLE(of, ljtale_device_match);

static int ljtale_device_dt_probe(struct platform_device *pdev) {
    const struct of_device_id *of_id;
    of_id = of_match_node(ljtale_device_match, pdev->dev.of_node);
    printk("ljtale driver probed: %s %s\n", of_id->name, of_id->compatible);
    return 0;
}

static int ljtale_device_dt_remove(struct platform_device *pdev) {
    const struct of_device_id *of_id;
    of_id = of_match_node(ljtale_device_match, pdev->dev.of_node);
    printk("ljtale driver removed: %s %s\n", of_id->name, of_id->compatible);
    return 0;
}

static struct platform_driver ljtale_device_driver = {
    .driver = {
        .name = "ljtale-device-name",
        // .owner = THIS_MODULE;
        .of_match_table = of_match_ptr(ljtale_device_match),
    },
    .probe = ljtale_device_dt_probe,
    .remove = ljtale_device_dt_remove,
};

module_platform_driver(ljtale_device_driver);
