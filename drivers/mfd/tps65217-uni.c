/*
 * tps65217.c
 *
 * TPS65217 chip family multi-function driver
 *
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/regmap.h>
#include <linux/err.h>
#include <linux/input.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>

#include <linux/mfd/core.h>
#include <linux/mfd/tps65217.h>

/* ljtale starts */
#include <linux/universal-drv.h>
/* ljtale ends */

static const struct mfd_cell tps65217s[] = {
	{
		.name = "tps65217-pmic",
		.of_compatible = "ti,tps65217-pmic",
	},
	{
		.name = "tps65217-bl",
		.of_compatible = "ti,tps65217-bl",
	},
};

/**
 * tps65217_reg_read: Read a single tps65217 register.
 *
 * @tps: Device to read from.
 * @reg: Register to read.
 * @val: Contians the value
 */
int tps65217_reg_read(struct tps65217 *tps, unsigned int reg,
			unsigned int *val)
{
	return regmap_read(tps->regmap, reg, val);
}
EXPORT_SYMBOL_GPL(tps65217_reg_read);

/**
 * tps65217_reg_write: Write a single tps65217 register.
 *
 * @tps65217: Device to write to.
 * @reg: Register to write to.
 * @val: Value to write.
 * @level: Password protected level
 */
int tps65217_reg_write(struct tps65217 *tps, unsigned int reg,
			unsigned int val, unsigned int level)
{
	int ret;
	unsigned int xor_reg_val;

	switch (level) {
	case TPS65217_PROTECT_NONE:
		return regmap_write(tps->regmap, reg, val);
	case TPS65217_PROTECT_L1:
		xor_reg_val = reg ^ TPS65217_PASSWORD_REGS_UNLOCK;
		ret = regmap_write(tps->regmap, TPS65217_REG_PASSWORD,
							xor_reg_val);
		if (ret < 0)
			return ret;

		return regmap_write(tps->regmap, reg, val);
	case TPS65217_PROTECT_L2:
		xor_reg_val = reg ^ TPS65217_PASSWORD_REGS_UNLOCK;
		ret = regmap_write(tps->regmap, TPS65217_REG_PASSWORD,
							xor_reg_val);
		if (ret < 0)
			return ret;
		ret = regmap_write(tps->regmap, reg, val);
		if (ret < 0)
			return ret;
		ret = regmap_write(tps->regmap, TPS65217_REG_PASSWORD,
							xor_reg_val);
		if (ret < 0)
			return ret;
		return regmap_write(tps->regmap, reg, val);
	default:
		return -EINVAL;
	}
}
EXPORT_SYMBOL_GPL(tps65217_reg_write);

/**
 * tps65217_update_bits: Modify bits w.r.t mask, val and level.
 *
 * @tps65217: Device to write to.
 * @reg: Register to read-write to.
 * @mask: Mask.
 * @val: Value to write.
 * @level: Password protected level
 */
static int tps65217_update_bits(struct tps65217 *tps, unsigned int reg,
		unsigned int mask, unsigned int val, unsigned int level)
{
	int ret;
	unsigned int data;

	ret = tps65217_reg_read(tps, reg, &data);
	if (ret) {
		dev_err(tps->dev, "Read from reg 0x%x failed\n", reg);
		return ret;
	}

	data &= ~mask;
	data |= val & mask;

	ret = tps65217_reg_write(tps, reg, data, level);
	if (ret)
		dev_err(tps->dev, "Write for reg 0x%x failed\n", reg);

	return ret;
}

int tps65217_set_bits(struct tps65217 *tps, unsigned int reg,
		unsigned int mask, unsigned int val, unsigned int level)
{
	return tps65217_update_bits(tps, reg, mask, val, level);
}
EXPORT_SYMBOL_GPL(tps65217_set_bits);

int tps65217_clear_bits(struct tps65217 *tps, unsigned int reg,
		unsigned int mask, unsigned int level)
{
	return tps65217_update_bits(tps, reg, mask, 0, level);
}
EXPORT_SYMBOL_GPL(tps65217_clear_bits);

static const struct regmap_config tps65217_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = TPS65217_REG_MAX,
};

static const struct of_device_id tps65217_of_match[] = {
	{ .compatible = "ti,tps65217", .data = (void *)TPS65217 },
	{ /* sentinel */ },
};


/* ljtale starts */
static struct universal_regmap_type tps65217_universal_regmap = {
    .regmap_config = &tps65217_regmap_config,
};

static struct universal_devm_alloc_type tps65217_universal_devm_alloc = {
};

static struct universal_of_node_match_type tps65217_universal_of_node_match = {
    .matches = tps65217_of_match,
};

static struct universal_request tps65217_universal_requests[] = {
    {
        .type = REGMAP_INIT,
        .data = &tps65217_universal_regmap,
    },
    {
        .type = DEVM_ALLOCATE,
        .data = &tps65217_universal_devm_alloc,
    },
    {
        .type = OF_NODE_MATCH,
        .data = &tps65217_universal_of_node_match,
    },
};

static const char *tps65217_universal_driver_name = "tps65217-universal";

struct tps65217_universal_local {
    struct tps65217 *tps;
    unsigned long chip_id;
    int irq;
    int irq_gpio;
};

static struct tps65217_universal_local tps65217_local = {
    .irq = -1,
    .irq_gpio = -1,
};

static struct universal_drv tps65217_universal_driver;

/* ljtale ends */

static irqreturn_t tps65217_irq(int irq, void *irq_data)
{
	struct tps65217 *tps = irq_data;
	unsigned int int_reg = 0, status_reg = 0;

	tps65217_reg_read(tps, TPS65217_REG_INT, &int_reg);
	tps65217_reg_read(tps, TPS65217_REG_STATUS, &status_reg);
	if (status_reg)
		dev_dbg(tps->dev, "status now: 0x%X\n", status_reg);

	if (!int_reg)
		return IRQ_NONE;

	if (int_reg & TPS65217_INT_PBI) {
		/* Handle push button */
		dev_dbg(tps->dev, "power button status change\n");
		input_report_key(tps->pwr_but, KEY_POWER,
				status_reg & TPS65217_STATUS_PB);
		input_sync(tps->pwr_but);
	}
	if (int_reg & TPS65217_INT_ACI) {
		/* Handle AC power status change */
		dev_dbg(tps->dev, "AC power status change\n");
		/* Press KEY_POWER when AC not present */
		input_report_key(tps->pwr_but, KEY_POWER,
				~status_reg & TPS65217_STATUS_ACPWR);
		input_sync(tps->pwr_but);
	}
	if (int_reg & TPS65217_INT_USBI) {
		/* Handle USB power status change */
		dev_dbg(tps->dev, "USB power status change\n");
	}

	return IRQ_HANDLED;
}

static int tps65217_probe_pwr_but(struct tps65217 *tps)
{
	int ret;
	unsigned int int_reg;

	tps->pwr_but = devm_input_allocate_device(tps->dev);
	if (!tps->pwr_but) {
		dev_err(tps->dev,
			"Failed to allocated pwr_but input device\n");
		return -ENOMEM;
	}

	tps->pwr_but->evbit[0] = BIT_MASK(EV_KEY);
	tps->pwr_but->keybit[BIT_WORD(KEY_POWER)] = BIT_MASK(KEY_POWER);
	tps->pwr_but->name = "tps65217_pwr_but";
    /* ljtale: register the input device to the input subsystem */
	ret = input_register_device(tps->pwr_but);
	if (ret) {
		/* NOTE: devm managed device */
		dev_err(tps->dev, "Failed to register button device\n");
		return ret;
	}
	ret = devm_request_threaded_irq(tps->dev,
		tps->irq, NULL, tps65217_irq, IRQF_TRIGGER_LOW | IRQF_ONESHOT,
		"tps65217", tps);
	if (ret != 0) {
		dev_err(tps->dev, "Failed to request IRQ %d\n", tps->irq);
		return ret;
	}

	/* enable the power button interrupt */
	ret = tps65217_reg_read(tps, TPS65217_REG_INT, &int_reg);
	if (ret < 0) {
		dev_err(tps->dev, "Failed to read INT reg\n");
		return ret;
	}
    /* ljtale: power button monitor bit in the interrupt register */
	int_reg &= ~TPS65217_INT_PBM;
	tps65217_reg_write(tps, TPS65217_REG_INT, int_reg, TPS65217_PROTECT_NONE);
	return 0;
}

/* ljtale starts */
static int tps65217_devm_alloc_populate(
        struct universal_devm_alloc_type *ptr) {
    /* we assume the ptr is valid because this function is called from
     * universal driver after allocating ptr, only the individual driver knows
     * what the type of the ptr is */
    int ret;
    struct tps65217_universal_local *local;
    struct tps65217 *tps;
    local = 
        (struct tps65217_universal_local *)tps65217_universal_driver.local_data;
    local->tps = (struct tps65217 *)(ptr->ret_addr);
    tps = local->tps;
    tps->regmap = tps65217_universal_regmap.regmap;
    tps->dev = ptr->dev; 
    tps->irq = local->irq;
    tps->irq_gpio = local->irq_gpio;
    /* we got an irq, request it */
    /* TODO: irq request should be extracted out as well */
    if (tps->irq >= 0) {
        ret = tps65217_probe_pwr_but(tps);
        if (ret < 0) {
            dev_err(tps->dev, "Failed to probe pwr_but\n");
            return ret;
        }
    }
    return 0;
}
/* ljtale ends */

static int tps65217_probe(struct i2c_client *client,
				const struct i2c_device_id *ids)
{
	struct tps65217 *tps;
	unsigned int version;
	unsigned long chip_id = ids->driver_data;
	const struct of_device_id *match;
	struct device_node *node;
	bool status_off = false;
	int irq = -1, irq_gpio = -1;
	int ret;

    /* ljtale starts */
    struct regmap_bus *regmap_bus;
    /* ljtale ends */

    /* ljtale starts */
    tps65217_universal_driver.name = tps65217_universal_driver_name;
    tps65217_universal_driver.dev = &client->dev;
    tps65217_universal_driver.requests = tps65217_universal_requests;
    tps65217_universal_driver.request_size =
        ARRAY_SIZE(tps65217_universal_requests);
    tps65217_universal_driver.local_data = &tps65217_local;
    ret = universal_drv_register(&tps65217_universal_driver);
    if (ret < 0) {
        LJTALE_MSG(KERN_ERR, "universal driver registration failed: %d\n", ret);
        return ret;
    }
    /* ljtale ends */

    /* ljtale starts */
    /* TODO: I leave the of node match untounched at this moment */
    tps65217_universal_of_node_match.dev = &client->dev;
    /* ljtale ends */
	node = client->dev.of_node;
	if (node) {
        /* ljtale: find a device node matched with client->dev to get
         * information from that device node, such as irq and memory range */
		match = of_match_device(tps65217_of_match, &client->dev);
		if (!match) {
			dev_err(&client->dev,
				"Failed to find matching dt id\n");
			return -EINVAL;
		}
		chip_id = (unsigned long)match->data;
		status_off = of_property_read_bool(node,
					"ti,pmic-shutdown-controller");

		/* at first try to get irq via OF method */
		irq = irq_of_parse_and_map(node, 0);
		if (irq <= 0) {
			irq = -1;
			irq_gpio = of_get_named_gpio(node, "irq-gpio", 0);
			if (irq_gpio >= 0) {
				/* valid gpio; convert to irq */
				ret = devm_gpio_request_one(&client->dev,
					irq_gpio, GPIOF_DIR_IN,
					"tps65217-gpio-irq");
				if (ret != 0)
					dev_warn(&client->dev, "Failed to "
						"request gpio #%d\n", irq_gpio);
				irq = gpio_to_irq(irq_gpio);
				if (irq <= 0) {
					dev_warn(&client->dev, "Failed to "
						"convert gpio #%d to irq\n",
						irq_gpio);
					irq = -1;
				}
			}
		}
	}

	if (!chip_id) {
		dev_err(&client->dev, "id is null.\n");
		return -ENODEV;
	}

    /* ljtale starts */
    /* initialize the universal driver data according to the request order, */
    /* either statically or dynamically */
    /* regmap init */
    tps65217_universal_regmap.regmap_bus_context = &client->dev; 
    regmap_bus = regmap_get_i2c_bus_pub(client, &tps65217_regmap_config);
    if (IS_ERR(regmap_bus)) {
        return PTR_ERR(regmap_bus);
    }
    tps65217_universal_regmap.regmap_bus = regmap_bus;
    /* devm allocation */
    tps65217_universal_devm_alloc.dev = &client->dev;
    tps65217_universal_devm_alloc.size = sizeof(struct tps65217);
    tps65217_universal_devm_alloc.gfp = GFP_KERNEL;
    tps65217_universal_devm_alloc.populate = tps65217_devm_alloc_populate;
    /* ljtale ends */

    /* ljtale starts */
    /* populate local data structure, the values are likely to be computed
     * in the probe function or locally in this file */
    tps65217_local.chip_id = chip_id;
    tps65217_local.irq = irq;
    tps65217_local.irq_gpio = irq_gpio;
    /* ljtale ends */

    /* ljtale starts */
    /*
     * FIXME: call universal driver init only after all the request fields 
     * are populated. Refer to the google doc for conerns and explanation
     */
    universal_drv_init(&tps65217_universal_driver);
    if (!tps65217_local.tps) {
        LJTALE_MSG(KERN_ERR, "universal driver allocation failed\n");
        return -EINVAL;
    }
    tps = tps65217_local.tps;
    /* ljtale ends */

    /* ljtale: the following code are essentially using the tps data structure,
     * TODO: we should think about creating other data structures to abstract
     * them out into universal driver 
     */
	ret = mfd_add_devices(tps->dev, -1, tps65217s,
			      ARRAY_SIZE(tps65217s), NULL, 0, NULL);
	if (ret < 0) {
		dev_err(tps->dev, "mfd_add_devices failed: %d\n", ret);
		return ret;
	}

	ret = tps65217_reg_read(tps, TPS65217_REG_CHIPID, &version);
	if (ret < 0) {
		dev_err(tps->dev, "Failed to read revision register: %d\n",
			ret);
		return ret;
	}

	/* Set the PMIC to shutdown on PWR_EN toggle */
	if (status_off) {
		ret = tps65217_set_bits(tps, TPS65217_REG_STATUS,
				TPS65217_STATUS_OFF, TPS65217_STATUS_OFF,
				TPS65217_PROTECT_NONE);
		if (ret)
			dev_warn(tps->dev, "unable to set the status OFF\n");
	}

    /* ljtale: dev_info defined in include/linux/device.h */
	dev_info(tps->dev, "TPS65217 ID %#x version 1.%d\n",
			(version & TPS65217_CHIPID_CHIP_MASK) >> 4,
			version & TPS65217_CHIPID_REV_MASK);

	return 0;
}

static int tps65217_remove(struct i2c_client *client)
{
	struct tps65217 *tps = i2c_get_clientdata(client);

	mfd_remove_devices(tps->dev);

	return 0;
}

static const struct i2c_device_id tps65217_id_table[] = {
	{"tps65217", TPS65217},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(i2c, tps65217_id_table);

static struct i2c_driver tps65217_driver = {
	.driver		= {
		.name	= "tps65217",
		.owner	= THIS_MODULE,
		.of_match_table = tps65217_of_match,
	},
	.id_table	= tps65217_id_table,
	.probe		= tps65217_probe,
	.remove		= tps65217_remove,
};

static int __init tps65217_init(void)
{
	return i2c_add_driver(&tps65217_driver);
}
subsys_initcall(tps65217_init);

static void __exit tps65217_exit(void)
{
	i2c_del_driver(&tps65217_driver);
}
module_exit(tps65217_exit);

MODULE_AUTHOR("AnilKumar Ch <anilkumar@ti.com>");
MODULE_DESCRIPTION("TPS65217 chip family multi-function driver");
MODULE_LICENSE("GPL v2");
