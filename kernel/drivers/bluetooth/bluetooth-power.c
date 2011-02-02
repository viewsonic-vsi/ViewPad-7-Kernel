/* Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
/*
 * Bluetooth Power Switch Module
 * controls power to external Bluetooth device
 * with interface to power management device
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>

/* FIH, JamesKCTung, 2009/06/16 { */
/* use rfkill to turn on off wifi */
#ifdef CONFIG_FIH_FXX
#define WIFI_CONTROL_MASK   0x10000000
static int  wifi_power_state;
struct rfkill   *g_WifiRfkill = NULL;

static int (*power_control)(int enable);
static DEFINE_SPINLOCK(bt_power_lock);
#endif
/* } FIH, JamesKCTung, 2009/06/16 */
static int bluetooth_toggle_radio(void *data, bool blocked)
{
	int ret;
	int (*power_control)(int enable);

	power_control = data;
	ret = (*power_control)(!blocked);
	return ret;
}
/* FIH, JamesKCTung, 2009/06/16 { */
/* use rfkill to turn on off wifi */
#ifdef CONFIG_FIH_FXX
//static int wifi_toggle_radio(void *data, enum rfkill_state state)
static int wifi_toggle_radio(void *data, bool blocked)
{
	int ret = 0;
    int state = 0;
    printk(KERN_ERR "wifi_toggle_radio() start!\n");
    printk(KERN_ERR "state = 0x%08x", state);
    state = WIFI_CONTROL_MASK | (!blocked);
	spin_lock(&bt_power_lock);
	//printk("wifi_toggle_radio - state=%d", state);
	//fih_printk(BT_debug_mask, FIH_DEBUG_ZONE_G0,"wifi_toggle_radio - state=%d\n", state);
	//ret = (*power_control)((state == RFKILL_STATE_UNBLOCKED) ? (1 | WIFI_CONTROL_MASK) : (0| WIFI_CONTROL_MASK) );
	ret = (*power_control)(state);
	spin_unlock(&bt_power_lock);
	printk(KERN_ERR "wifi_toggle_radio() finish!\n");
	return ret;
}

static const struct rfkill_ops wifi_power_rfkill_ops = {
	.set_block = wifi_toggle_radio,
};

static int wifi_rfkill_probe(struct platform_device *pdev)
{
	int ret = -ENOMEM;
	printk(KERN_ERR "wifi_rfkill_probe() start!\n");
	g_WifiRfkill = rfkill_alloc("wifi_ar6k", &pdev->dev, RFKILL_TYPE_WLAN,
			      &wifi_power_rfkill_ops,
			      pdev->dev.platform_data);
			      
	//g_WifiRfkill = rfkill_allocate(&pdev->dev, RFKILL_TYPE_WLAN);
	if (!g_WifiRfkill) {
		printk(KERN_ERR
			"%s: rfkill allocate failed\n", __func__);
		return -ENOMEM;
	}
	
	//ret = rfkill_set_default(RFKILL_TYPE_WLAN,
	//			RFKILL_STATE_SOFT_BLOCKED);
	rfkill_init_sw_state(g_WifiRfkill, 1);

	if (g_WifiRfkill) {
		//g_WifiRfkill->name = "wifi_ar6k";
		//g_WifiRfkill->toggle_radio = wifi_toggle_radio;
		//g_WifiRfkill->state = RFKILL_STATE_SOFT_BLOCKED;
		ret = rfkill_register(g_WifiRfkill);
		if (ret) {
			//printk(KERN_DEBUG
			//	"%s: wifi rfkill register failed=%d\n", __func__,
			//	ret);
			//fih_printk(BT_debug_mask, FIH_DEBUG_ZONE_G0,"%s: wifi rfkill register failed=%d\n", __func__,ret);
			rfkill_destroy(g_WifiRfkill);
			//rfkill_free(g_WifiRfkill);
            return -ENOMEM;
		}
	}

	/* force WIFI off during init to allow for user control */
	//rfkill_switch_all(RFKILL_TYPE_WLAN, RFKILL_STATE_SOFT_BLOCKED);
	platform_set_drvdata(pdev, g_WifiRfkill);
	printk(KERN_ERR "wifi_rfkill_probe() finish!\n");
	return ret;
}
#endif
/* } FIH, JamesKCTung, 2009/06/16 */

static const struct rfkill_ops bluetooth_power_rfkill_ops = {
	.set_block = bluetooth_toggle_radio,
};

static int bluetooth_power_rfkill_probe(struct platform_device *pdev)
{
	struct rfkill *rfkill;
	int ret;

	rfkill = rfkill_alloc("bt_power", &pdev->dev, RFKILL_TYPE_BLUETOOTH,
			      &bluetooth_power_rfkill_ops,
			      pdev->dev.platform_data);

	if (!rfkill) {
		printk(KERN_DEBUG
			"%s: rfkill allocate failed\n", __func__);
		return -ENOMEM;
	}

	/* force Bluetooth off during init to allow for user control */
	rfkill_init_sw_state(rfkill, 1);

	ret = rfkill_register(rfkill);
	if (ret) {
		printk(KERN_DEBUG
			"%s: rfkill register failed=%d\n", __func__,
			ret);
		rfkill_destroy(rfkill);
		return ret;
	}

	platform_set_drvdata(pdev, rfkill);

	return 0;
}

static void bluetooth_power_rfkill_remove(struct platform_device *pdev)
{
	struct rfkill *rfkill;

	rfkill = platform_get_drvdata(pdev);
	if (rfkill)
		rfkill_unregister(rfkill);
/* FIH, JamesKCTung, 2009/06/16 { */
/* Remove WIFI RFKILL */
#ifdef CONFIG_FIH_FXX
	if (g_WifiRfkill)
		rfkill_unregister(g_WifiRfkill);
#endif
/* } FIH, JamesKCTung, 2009/06/16 */
	rfkill_destroy(rfkill);
	platform_set_drvdata(pdev, NULL);
}

static int __init bt_power_probe(struct platform_device *pdev)
{
	int ret = 0;

	printk(KERN_DEBUG "%s\n", __func__);

	if (!pdev->dev.platform_data) {
		printk(KERN_ERR "%s: platform data not initialized\n",
				__func__);
		return -ENOSYS;
	}

/* FIH, JamesKCTung, 2009/06/03 { */
#ifdef CONFIG_FIH_FXX
	wifi_power_state = 0;
	power_control = pdev->dev.platform_data;
#endif
/* } FIH, JamesKCTung, 2009/06/03 */
/* FIH, JamesKCTung, 2009/06/16 { */
#ifndef CONFIG_FIH_FXX
	ret = (*power_control)(bluetooth_power_state);
#endif
/* } FIH, JamesKCTung, 2009/06/16 */
	ret = bluetooth_power_rfkill_probe(pdev);

/* FIH, JamesKCTung, 2009/06/16 { */
/* use rfkill to turn on off wifi */
#ifdef CONFIG_FIH_FXX
	wifi_rfkill_probe(pdev);
#endif
/* } FIH, JamesKCTung, 2009/06/16 */
	return ret;
}

static int __devexit bt_power_remove(struct platform_device *pdev)
{
	printk(KERN_DEBUG "%s\n", __func__);

	bluetooth_power_rfkill_remove(pdev);

/* FIH, JamesKCTung, 2009/06/03 { */
#ifdef CONFIG_FIH_FXX
	wifi_power_state=0;
	(*power_control)(wifi_power_state | WIFI_CONTROL_MASK );
#endif
/* } FIH, JamesKCTung, 2009/06/03 */
	return 0;
}

static struct platform_driver bt_power_driver = {
	.probe = bt_power_probe,
	.remove = __devexit_p(bt_power_remove),
	.driver = {
		.name = "bt_power",
		.owner = THIS_MODULE,
	},
};

static int __init bluetooth_power_init(void)
{
	int ret;

	printk(KERN_DEBUG "%s\n", __func__);
	ret = platform_driver_register(&bt_power_driver);
	return ret;
}

static void __exit bluetooth_power_exit(void)
{
	printk(KERN_DEBUG "%s\n", __func__);
	platform_driver_unregister(&bt_power_driver);
}

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MSM Bluetooth power control driver");
MODULE_VERSION("1.30");

module_init(bluetooth_power_init);
module_exit(bluetooth_power_exit);
