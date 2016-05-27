/*
 * universal driver core source file @ljtale
 * */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/pm_runtime.h>
#include <linux/pm_domain.h>
#include <linux/err.h>
#include <linux/mutex.h>

#include <linux/universal-drv.h>

void regacc_lock_mutex(void *__regacc)
{
    struct register_accessor *regacc = __regacc;
    mutex_lock(&regacc->mutex);
}

void regacc_unlock_mutex(void *__regacc)
{
    struct register_accessor *regacc = __regacc;
    mutex_unlock(&regacc->mutex);
}
