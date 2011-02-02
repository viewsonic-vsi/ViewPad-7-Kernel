
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/input.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/delay.h>

#include <linux/proc_fs.h>
#include "msm_fb.h"
#include <asm/gpio.h>

#include "mddihosti.h"

/*
    Macro for easily apply LCM vendor's init code
*/

#define Delayms(n) mdelay(n)

#define ARRAY_AND_SIZE(x)	(x), ARRAY_SIZE(x)


void fih_lcdc_enter_sleep(void)
{    
    printk(KERN_INFO "lcm_innolux_7d: fih_lcdc_enter_sleep()\n");
   
    gpio_set_value(16, 1);    //LCM LS disable 

    gpio_set_value(78, 0);    //LCM_Bias_EN disable

    gpio_set_value(1, 0);   //LCM_DVDD_EN disable

    gpio_set_value(131, 0);   

    gpio_set_value(132, 0);

}

void fih_lcdc_exit_sleep(void)
{
    printk(KERN_INFO "lcm_innolux_7d: fih_lcdc_exit_sleep()\n");
    
    gpio_set_value(1, 1);   //LCM_DVDD_EN

    gpio_set_value(131, 0);    //LCM UD set low
    gpio_set_value(132, 1);    //LCM LR set high

    gpio_set_value(78, 1);    //LCM_Bias_EN
        
    gpio_set_value(16, 0);    //LCM LS enable   

}

EXPORT_SYMBOL(fih_lcdc_enter_sleep);
EXPORT_SYMBOL(fih_lcdc_exit_sleep);


extern int fih_lcm_is_mddi_type(void);

static int __init lcdc_innolux_init(void)
{
    int ret;
    struct msm_panel_info pinfo;

    printk(KERN_INFO "lcm_innolux_7d: +lcdc_innolux_init()\n");


    if (mddi_get_client_id() != 0)
    {
        printk(KERN_INFO "lcm_innolux_7d: found MDDI panel()\n");    
        return 0;
    }
    else
    {
        printk(KERN_INFO "lcm_innolux_7d: Not found MDDI panel()\n");
    }

    pinfo.xres = 800;
    pinfo.yres = 480;
    pinfo.type = LCDC_PANEL;
    pinfo.pdest = DISPLAY_1;
    pinfo.wait_cycle = 0;
    pinfo.bpp = 24;
    pinfo.fb_num = 2;

    pinfo.clk_rate = 36000000;
    printk(KERN_INFO "lcm_innolux_7d: +lcdc_innolux_init(): pinfo.clk_rate(%d)\n", pinfo.clk_rate);

    pinfo.bl_max=100;
    pinfo.bl_min=0;

    pinfo.lcdc.h_back_porch = 6;
    pinfo.lcdc.h_front_porch = 210;
    pinfo.lcdc.h_pulse_width = 40;

    pinfo.lcdc.v_back_porch = 3;
    pinfo.lcdc.v_front_porch = 22;
    pinfo.lcdc.v_pulse_width = 20;

    pinfo.lcdc.border_clr = 0;	/* blk */
    pinfo.lcdc.underflow_clr = 0x0;//ff;	workaround for DUT shows the blue screen when booting/* blue */
    pinfo.lcdc.hsync_skew = 0;	

    ret = lcdc_device_register(&pinfo);
    
    if (ret)
        printk(KERN_ERR "%s: failed to register device!\n", __func__);

    return ret;
}

static void __exit lcdc_innolux_exit(void)
{

}

module_init(lcdc_innolux_init);
module_exit(lcdc_innolux_exit);

