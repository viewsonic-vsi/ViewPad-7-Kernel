#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/completion.h>
#include <linux/spinlock.h>
#include <linux/miscdevice.h>
//#include <linux/elan_i2c.h>
#include <linux/focal_i2c.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <mach/gpio.h>
//Dynamic to load RES or CAP touch driver++
#include <mach/msm_iomap.h>
#include <mach/msm_smd.h>
//Dynamic to load RES or CAP touch driver--
#include <mach/vreg.h>  //Add for VREG_WLAN power in, 07/08
/* FIH, SimonSSChang, 2009/09/04 { */
/* [FXX_CR], change CAP touch suspend/resume function
to earlysuspend */
#include <linux/earlysuspend.h>
/* } FIH, SimonSSChang, 2009/09/04 */
//Added by Stanley for dump scheme++
#include <linux/unistd.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
//Added by Stanley for dump scheme--
#include <linux/cmdbgapi.h>  //Added for debug mask (2010/01/04)


/************************************************
 *  attribute marco
 ************************************************/
#define TOUCH_NAME		"ft5202"	/* device name */
#define ft5202_DEBUG	1			/* print out receive packet */
#define KEYCODE_BROWSER 192  //Modified for Home and AP key (2009/07/31)
#define KEYCODE_SEACHER 217  //Modified for Home and AP key (2009/07/31)

/************************************************
 *  function marco
 ************************************************/
#define ft5202_msg(lv, fmt, arg...)	\
	printk(KERN_ ##lv"[%s@%s@%d] "fmt"\n",__FILE__,__func__,__LINE__,##arg)

static struct proc_dir_entry *msm_touch_proc_file = NULL;  //Added by Stanley for dump scheme++
/************************************************
 *  Global variable
 ************************************************/
u8 FT5202_buffer[26];
bool bFT5202_Soft1CapKeyPressed = 0;  //Modified for new CAP sample by Stanley (2009/05/25)	
bool bFT5202_Soft2CapKeyPressed = 0;  //Modified for new CAP sample by Stanley (2009/05/25)
bool bFT5202_HomeCapKeyPressed = 0;  //Modified for Home and AP key (2009/07/31)
bool bFT5202_ApCapKeyPressed = 0;  //Modified for Home and AP key (2009/07/31)
bool bFT5202_CenterKeyPressed = 0;  //Added for FST
bool bFT5202_IsF913 = 0;  //Modified for Home and AP key (2009/07/31)
bool bFT5202_IsFST = 0;  //Added for FST
bool bFT5202_IsFxxPR2 = 0;  //Added for PR2
bool bFT5202_IsPenUp = 1;  //Added for touch behavior (2009/08/14)
bool bFT5202_IsKeyLock = 0;  //Added for new behavior (2009/09/27)
bool bFT5202_IsNewFWVer = 0;  //Add for detect firmware version
bool bFT5202_PrintPenDown = 0;  //Added log for debugging
/* FIH, Henry Juang, 2009/11/20 ++*/
/* [FXX_CR], Add for proximity driver to turn on/off BL and TP. */
bool bFT5202_IsNeedSkipTouchEvent = 0;  
/* FIH, Henry Juang, 2009/11/20 --*/

//Added for debug mask definitions++
/************************************************
 * Debug Definitions
 ************************************************/
#if 1
enum {
	MSM_PM_DEBUG_COORDINATES = 1U << 0,
};

static uint32_t msm_cap_touch_debug_mask_ft5202;
module_param_named(
    debug_mask, msm_cap_touch_debug_mask_ft5202, uint, S_IRUGO | S_IWUSR | S_IWGRP
);  //Added for debug mask (2010/01/04)

static int msm_cap_touch_gpio_pull_fail;
module_param_named(
    gpio_pull, msm_cap_touch_gpio_pull_fail, int, S_IRUGO | S_IWUSR | S_IWGRP
);

//Added for show FW version on FQC++
static int msm_cap_touch_fw_version;
module_param_named(
    fw_version, msm_cap_touch_fw_version, int, S_IRUGO | S_IWUSR | S_IWGRP
);
//Added for show FW version on FQC--
//Added for debug mask definitions--
#endif
/************************************************
 * Control structure
 ************************************************/
struct ft5202_innolux {
	struct i2c_client *client;
	struct input_dev  *input;
	struct work_struct wqueue;
	struct completion data_ready;
	struct completion data_complete;
/* FIH, SimonSSChang, 2009/09/04 { */
/* [FXX_CR], change CAP touch suspend/resume function
to earlysuspend */
#ifdef CONFIG_FIH_FXX
#ifdef CONFIG_HAS_EARLYSUSPEND
    struct early_suspend ft5202_early_suspend_desc;
#endif
#endif
/* } FIH, SimonSSChang, 2009/09/04 */    
} *ft5202;

static int ft5202_recv(u8 *buf, u32 count)
{
	struct i2c_adapter *adap = ft5202->client->adapter;
    struct i2c_msg msgs[]= {
		[0] = {
			.addr = ft5202->client->addr,
            .flags = I2C_M_RD,
            .len = count,
            .buf = buf,
		},
	};

	return (i2c_transfer(adap, msgs, 1) < 0) ? -EIO : 0;
}

static int ft5202_send(u8 *buf, u32 count)
{
	struct i2c_adapter *adap = ft5202->client->adapter;
	struct i2c_msg msgs[]= {
		[0] = {
            .addr = ft5202->client->addr,
			.flags = 0,
			.len = count,
			.buf = buf,
        },
    };

	return (i2c_transfer(adap, msgs, 1) < 0) ? -EIO : 0;
}

static int ft5202_get_fw_version(int *version)
{
    char cmd[2] = { 0xFC, 0x7B };

	ft5202_send(cmd, ARRAY_SIZE(cmd));
	//wait_for_completion(&ft5202->data_ready);
	if (ft5202_recv(FT5202_buffer, ARRAY_SIZE(FT5202_buffer)) < 0)
	{
        ft5202_msg(ERR, "[FT5202]ft5202_get_fw_version failed");
	    return -1;
	}

	*version = FT5202_buffer[0];
	
	return 0;
}

static int ft5202_get_vendor_id(int *id)
{
	char cmd[2] = { 0xFC, 0x7D };

	ft5202_send(cmd, ARRAY_SIZE(cmd));
	//wait_for_completion(&ft5202->data_ready);
	if (ft5202_recv(FT5202_buffer, ARRAY_SIZE(FT5202_buffer)) < 0)
	{
        ft5202_msg(ERR, "[FT5202]ft5202_get_vendor_id failed");
	    return -1;
	}
	*id = FT5202_buffer[0];

	return 0;
}

//Added by Stanley++
static int ft5202_set_starttch(void)
{
	char cmd[1] = { 0xF9 };

	if (ft5202_send(cmd, ARRAY_SIZE(cmd)) < 0) {
		ft5202_msg(ERR, "[FT5202]Send STARTTCH failed");
		return -1;
	}

	return 0;
}
//Added by Stanley--

/* FIH, Henry Juang, 2009/11/20 ++*/
/* [FXX_CR], Add for proximity driver to turn on/off BL and TP. */
int notify_from_proximity_1(bool bFlag)
{
	bFT5202_IsNeedSkipTouchEvent = bFlag;
    	ft5202_msg(ERR, "[TOUCH]notify_from_proximity = %d\r\n", bFT5202_IsNeedSkipTouchEvent);
    	printk("[TOUCH]notify_from_proximity = %d\r\n", bFT5202_IsNeedSkipTouchEvent);
    
	return 1;
}
EXPORT_SYMBOL(notify_from_proximity_1);
/* FIH, Henry Juang, 2009/11/20 --*/

#define XCORD1(x) (((int)((x)[5]) << 8) + ((int)((x)[6])))
#define YCORD1(y) (((int)((y)[7]) << 8) + ((int)((y)[8])))
#define XCORD2(x) (((int)((x)[9]) << 8) + ((int)((x)[10])))
#define YCORD2(y) (((int)((y)[11]) << 8) + ((int)((y)[12])))
#define XCORD3(x) (((int)((x)[13]) << 8) + ((int)((x)[14])))
#define YCORD3(y) (((int)((y)[15]) << 8) + ((int)((y)[16])))
#define XCORD4(x) (((int)((x)[17]) << 8) + ((int)((x)[18])))
#define YCORD4(y) (((int)((y)[19]) << 8) + ((int)((y)[20])))
#define XCORD5(x) (((int)((x)[21]) << 8) + ((int)((x)[22])))
#define YCORD5(y) (((int)((y)[23]) << 8) + ((int)((y)[24])))
static void ft5202_isr_workqueue(struct work_struct *work)
{
	struct input_dev *input = ft5202->input;
	int cnt, virtual_button;  //Modified for new CAP sample by Stanley (2009/05/25)
	//int i;
	
    disable_irq(ft5202->client->irq);
	ft5202_set_starttch();
	ft5202_recv(FT5202_buffer, ARRAY_SIZE(FT5202_buffer));
		
    #if 1
	if (FT5202_buffer[0] == 0xAA && FT5202_buffer[1] == 0xAA) {
		cnt = FT5202_buffer[3];
		virtual_button = (FT5202_buffer[4]);
        //ft5202_msg(INFO, "[TOUCH-CAP]cnt = %d\r\n", cnt);
        //ft5202_msg(INFO, "[TOUCH-CAP]virtual button = %d\r\n", virtual_button);
        fih_printk(msm_cap_touch_debug_mask_ft5202, FIH_DEBUG_ZONE_G1, "[TOUCH-CAP]cnt = %d\r\n", cnt);
        fih_printk(msm_cap_touch_debug_mask_ft5202, FIH_DEBUG_ZONE_G1, "[TOUCH-CAP]virtual button = %d\r\n", virtual_button);
        //Modified for new CAP sample by Stanley ++(2010/03/15)
        if((virtual_button == 0) && !bFT5202_Soft1CapKeyPressed && !bFT5202_Soft2CapKeyPressed && !bFT5202_HomeCapKeyPressed && !bFT5202_ApCapKeyPressed && !bFT5202_CenterKeyPressed)
        {
		    input_report_key(input, BTN_TOUCH, cnt > 0);
		    input_report_key(input, BTN_2, cnt == 2);
		    input_report_key(input, BTN_3, cnt == 3);
#if 0			
		    if (cnt) {
			    input_report_abs(input, ABS_X, XCORD1(FT5202_buffer));  //Add for protect origin point
			    input_report_abs(input, ABS_Y, YCORD1(FT5202_buffer));
			    //ft5202_msg(INFO, "[TOUCH-CAP]x1 = %d, y1 = %d\r\n", XCORD1(FT5202_buffer), YCORD1(FT5202_buffer));
		    }
		    if (cnt > 1) {
                input_report_abs(input, ABS_HAT0X, XCORD2(FT5202_buffer));  //Add for protect origin point
                input_report_abs(input, ABS_HAT0Y, YCORD2(FT5202_buffer));
                //ft5202_msg(INFO, "[TOUCH-CAP]x2 = %d, y2 = %d\r\n", XCORD2(FT5202_buffer), YCORD2(FT5202_buffer));
            }
#else
		    if (cnt) {			    
   				input_report_abs(input, ABS_MT_TOUCH_MAJOR, 255);
				input_report_abs(input, ABS_MT_POSITION_X, XCORD1(FT5202_buffer) );
				input_report_abs(input, ABS_MT_POSITION_Y, YCORD1(FT5202_buffer) );
				input_mt_sync(input);
			    if (cnt > 1) {
	   				input_report_abs(input, ABS_MT_TOUCH_MAJOR, 255);
					input_report_abs(input, ABS_MT_POSITION_X, XCORD2(FT5202_buffer) );
					input_report_abs(input, ABS_MT_POSITION_Y, YCORD2(FT5202_buffer) );
					input_mt_sync(input);
	            }
		    }else{
   				input_report_abs(input, ABS_MT_TOUCH_MAJOR, 0);
				input_report_abs(input, ABS_MT_POSITION_X, XCORD1(FT5202_buffer) );
				input_report_abs(input, ABS_MT_POSITION_Y, YCORD1(FT5202_buffer) );
				input_mt_sync(input);
				
   				input_report_abs(input, ABS_MT_TOUCH_MAJOR, 0);
				input_report_abs(input, ABS_MT_POSITION_X, XCORD2(FT5202_buffer) );
				input_report_abs(input, ABS_MT_POSITION_Y, YCORD2(FT5202_buffer) );
				input_mt_sync(input);

			}

#endif
            //Added for debug mask (2010/03/31)++
            if (cnt && bFT5202_PrintPenDown)
            {
                fih_printk(msm_cap_touch_debug_mask_ft5202, FIH_DEBUG_ZONE_G1, "[TOUCH-CAP]Pen down x = %d, y = %d\r\n", XCORD1(FT5202_buffer), YCORD1(FT5202_buffer));  //Added for debug mask (2010/01/04)
                bFT5202_PrintPenDown = 0;
            }
            else if (!cnt)
            {
                fih_printk(msm_cap_touch_debug_mask_ft5202, FIH_DEBUG_ZONE_G1, "[TOUCH-CAP]Pen up x = %d, y = %d\r\n", XCORD2(FT5202_buffer), YCORD2(FT5202_buffer));  //Added for debug mask (2010/01/04)
                bFT5202_PrintPenDown = 1;
            }
            //Added for debug mask (2010/03/31)--

            //Added for touch behavior (2009/08/14)++
            if(!cnt)
                bFT5202_IsPenUp = 1;
            else
                bFT5202_IsPenUp = 0;
            //Added for touch behavior (2009/08/14)--
        }
        else if ((virtual_button == 0) && (bFT5202_Soft1CapKeyPressed || bFT5202_Soft2CapKeyPressed || bFT5202_HomeCapKeyPressed || bFT5202_ApCapKeyPressed || bFT5202_CenterKeyPressed) && !bFT5202_IsKeyLock)  //Button up  //Modified for Home and AP key (2009/07/31)
        {
            if(bFT5202_Soft1CapKeyPressed)
            {
                //input_report_key(input, KEY_KBDILLUMDOWN, 0);
                //Added for FST++ 
                if(bFT5202_IsFST)
                    input_report_key(input, KEY_SEND, 0);  //FST
                else
                    input_report_key(input, KEY_KBDILLUMDOWN, 0);
                //Added for FST--
                ft5202_msg(INFO, "[TOUCH-CAP]virtual button SOFT1 - up!\r\n");
                bFT5202_Soft1CapKeyPressed = 0;
                bFT5202_Soft2CapKeyPressed = 0;
                bFT5202_HomeCapKeyPressed = 0;  //Modified for Home and AP key (2009/07/31)
                bFT5202_ApCapKeyPressed = 0;  //Modified for Home and AP key (2009/07/31)
                bFT5202_CenterKeyPressed = 0;  //Added for FST
            }
            else if(bFT5202_Soft2CapKeyPressed)
            {
                //input_report_key(input, KEY_BACK, 0);
                //Added for FST++
                if(bFT5202_IsFST)
                    input_report_key(input, KEY_END, 0);  //FST
                else
                    input_report_key(input, KEY_BACK, 0);
                //Added for FST--
                ft5202_msg(INFO, "[TOUCH-CAP]virtual button SOFT2 - up!\r\n");
                bFT5202_Soft2CapKeyPressed = 0;
                bFT5202_Soft1CapKeyPressed = 0;
                bFT5202_HomeCapKeyPressed = 0;  //Modified for Home and AP key (2009/07/31)
                bFT5202_ApCapKeyPressed = 0;  //Modified for Home and AP key (2009/07/31)
                bFT5202_CenterKeyPressed = 0;  //Added for FST
            }
            //Modified for Home and AP key (2009/07/31)++
            else if(bFT5202_HomeCapKeyPressed)
            {
                //if(bFT5202_IsF913)
                    //input_report_key(input, KEYCODE_SEACHER, 0);  //F913
                //else
                    //input_report_key(input, KEY_HOME, 0);  //F905 or other
                //Added for FST++
                if(bFT5202_IsF913 && !bFT5202_IsFST)
                    input_report_key(input, KEYCODE_SEACHER, 0);  //F913
                else if(bFT5202_IsFST)
                    input_report_key(input, KEY_KBDILLUMDOWN, 0);  //FST
                else
                    input_report_key(input, KEY_HOME, 0);  //F905 or other
                //Added for FST--
                ft5202_msg(INFO, "[TOUCH-CAP]virtual button HOME key - up!\r\n");
                bFT5202_Soft2CapKeyPressed = 0;
                bFT5202_Soft1CapKeyPressed = 0;
                bFT5202_HomeCapKeyPressed = 0;  //Modified for Home and AP key (2009/07/31)
                bFT5202_ApCapKeyPressed = 0;  //Modified for Home and AP key (2009/07/31)
                bFT5202_CenterKeyPressed = 0;  //Added for FST
            }
            else if(bFT5202_ApCapKeyPressed)
            {
                //input_report_key(input, KEYCODE_BROWSER, 0);
                //Added for FST++
                if(bFT5202_IsFST)
                    input_report_key(input, KEY_BACK, 0);  //FST
                else
                    input_report_key(input, KEYCODE_BROWSER, 0);
                //Added for FST--
                ft5202_msg(INFO, "[TOUCH-CAP]virtual button AP key - up!\r\n");
                bFT5202_Soft2CapKeyPressed = 0;
                bFT5202_Soft1CapKeyPressed = 0;
                bFT5202_HomeCapKeyPressed = 0;  //Modified for Home and AP key (2009/07/31)
                bFT5202_ApCapKeyPressed = 0;  //Modified for Home and AP key (2009/07/31)
                bFT5202_CenterKeyPressed = 0;  //Added for FST
            }
            //Added for FST++
            else if(bFT5202_CenterKeyPressed)
            {
                input_report_key(input, KEYCODE_SEACHER, 0);
                ft5202_msg(INFO, "[TOUCH-CAP]virtual button Center key - up!\r\n");
                bFT5202_Soft2CapKeyPressed = 0;
                bFT5202_Soft1CapKeyPressed = 0;
                bFT5202_HomeCapKeyPressed = 0;  //Modified for Home and AP key (2009/07/31)
                bFT5202_ApCapKeyPressed = 0;  //Modified for Home and AP key (2009/07/31)
                bFT5202_CenterKeyPressed = 0;  //Added for FST
            }
            //Added for FST--
            //Modified for Home and AP key (2009/07/31)--
        }
        else if(virtual_button == 243 && !bFT5202_Soft2CapKeyPressed && !bFT5202_IsNeedSkipTouchEvent)  //Button 4 //Modified for Home and AP key (2009/07/31)  /* FIH, Henry Juang, 2009/11/20 ++*/ /* [FXX_CR], Add for proximity driver to turn on/off BL and TP. */
        {
            //Added for touch behavior (2009/08/14)++
            if(!bFT5202_IsPenUp)
            {
                input_report_key(input, BTN_TOUCH, 0);
                bFT5202_IsPenUp = 1;
                ft5202_msg(INFO, "[TOUCH-CAP]Send BTN touch - up!\r\n");
                bFT5202_IsKeyLock = 1;  //Added for new behavior (2009/09/27)
            }
            //Added for touch behavior (2009/08/14)--
            //Added for FST++
            if(!bFT5202_IsKeyLock)  //Added for new behavior (2009/09/27)
            {
                if(bFT5202_IsFST)
                    input_report_key(input, KEY_END, 1);  //FST
                else
                    input_report_key(input, KEY_BACK, 1);
            }
            //Added for FST--
            bFT5202_IsKeyLock = 0;  //Added for new behavior (2009/09/27)
            ft5202_msg(INFO, "[TOUCH-CAP]virtual button SOFT2 - down!\r\n");
            bFT5202_Soft2CapKeyPressed = 1;
        }
        //Modified for Home and AP key (2009/07/31)++
        else if(virtual_button == 242 && !bFT5202_ApCapKeyPressed && !bFT5202_IsNeedSkipTouchEvent)  //Button 3 //Modified for Home and AP key (2009/07/31)  /* FIH, Henry Juang, 2009/11/20 ++*/ /* [FXX_CR], Add for proximity driver to turn on/off BL and TP. */
        {
            //Added for touch behavior (2009/08/14)++
            if(!bFT5202_IsPenUp)
            {
                input_report_key(input, BTN_TOUCH, 0);
                bFT5202_IsPenUp = 1;
                ft5202_msg(INFO, "[TOUCH-CAP]Send BTN touch - up!\r\n");
                bFT5202_IsKeyLock = 1;  //Added for new behavior (2009/09/27)
            }
            //Added for touch behavior (2009/08/14)--
            //Added for FST++
            if(!bFT5202_IsKeyLock)  //Added for new behavior (2009/09/27)
            {
                if(bFT5202_IsFST)
                    input_report_key(input, KEY_BACK, 1);  //FST
                else
                    input_report_key(input, KEYCODE_BROWSER, 1);
            }
            //Added for FST--
            bFT5202_IsKeyLock = 0;  //Added for new behavior (2009/09/27)
            ft5202_msg(INFO, "[TOUCH-CAP]virtual button AP key - down!\r\n");
            bFT5202_ApCapKeyPressed = 1;
        }
        else if(virtual_button == 241 && !bFT5202_HomeCapKeyPressed && !bFT5202_IsNeedSkipTouchEvent)  //Button 2 //Modified for Home and AP key (2009/07/31)  /* FIH, Henry Juang, 2009/11/20 ++*/ /* [FXX_CR], Add for proximity driver to turn on/off BL and TP. */
        {
            //Added for touch behavior (2009/08/14)++
            if(!bFT5202_IsPenUp)
            {
                input_report_key(input, BTN_TOUCH, 0);
                bFT5202_IsPenUp = 1;
                ft5202_msg(INFO, "[TOUCH-CAP]Send BTN touch - up!\r\n");
                bFT5202_IsKeyLock = 1;  //Added for new behavior (2009/09/27)
            }
            //Added for touch behavior (2009/08/14)--
            if(!bFT5202_IsKeyLock)  //Added for new behavior (2009/09/27)
            {
                if(bFT5202_IsF913 && !bFT5202_IsFST)  //Added for FST
                    input_report_key(input, KEYCODE_SEACHER, 1);  //F913
                //Added for FST++
                else if(bFT5202_IsFST)
                    input_report_key(input, KEY_KBDILLUMDOWN, 1);  //FST
                //Added for FST--
                else
                    input_report_key(input, KEY_HOME, 1);  //F905 or other
            }
            bFT5202_IsKeyLock = 0;  //Added for new behavior (2009/09/27)
            ft5202_msg(INFO, "[TOUCH-CAP]virtual button HOME key - down!\r\n");
            bFT5202_HomeCapKeyPressed = 1;
        }
        //Modified for Home and AP key (2009/07/31)--
        else if(virtual_button == 240 && !bFT5202_Soft1CapKeyPressed && !bFT5202_IsNeedSkipTouchEvent)  //Button 1 //Modified for Home and AP key (2009/07/31)  /* FIH, Henry Juang, 2009/11/20 ++*/ /* [FXX_CR], Add for proximity driver to turn on/off BL and TP. */
        {
            //Added for touch behavior (2009/08/14)++
            if(!bFT5202_IsPenUp)
            {
                input_report_key(input, BTN_TOUCH, 0);
                bFT5202_IsPenUp = 1;
                ft5202_msg(INFO, "[TOUCH-CAP]Send BTN touch - up!\r\n");
                bFT5202_IsKeyLock = 1;  //Added for new behavior (2009/09/27)
            }
            //Added for touch behavior (2009/08/14)--
            //Added for FST++ 
            if(!bFT5202_IsKeyLock)  //Added for new behavior (2009/09/27)
            {
                if(bFT5202_IsFST)
                    input_report_key(input, KEY_SEND, 1);  //FST
                else{
                    input_report_key(input, KEY_KBDILLUMDOWN, 1);
                    //ft5202_msg(INFO, "[TOUCH-CAP]virtual button KEY_KBDILLUMDOWN1 - down!\r\n");
                }
            }
            //Added for FST--
            bFT5202_IsKeyLock = 0;  //Added for new behavior (2009/09/27)
            ft5202_msg(INFO, "[TOUCH-CAP]virtual button SOFT1 - down!\r\n");
            bFT5202_Soft1CapKeyPressed = 1;
        }
        //Added for FST++
        else if(virtual_button == 244 && !bFT5202_CenterKeyPressed && !bFT5202_IsNeedSkipTouchEvent)  //Added for FST  //Button 5  /* FIH, Henry Juang, 2009/11/20 ++*/ /* [FXX_CR], Add for proximity driver to turn on/off BL and TP. */
        {
            //Added for touch behavior (2009/08/14)++
            if(!bFT5202_IsPenUp)
            {
                input_report_key(input, BTN_TOUCH, 0);
                bFT5202_IsPenUp = 1;
                ft5202_msg(INFO, "[TOUCH-CAP]Send BTN touch - up!\r\n");
                bFT5202_IsKeyLock = 1;  //Added for new behavior (2009/09/27)
            }
            //Added for touch behavior (2009/08/14)--
            if(!bFT5202_IsKeyLock)
                input_report_key(input, KEYCODE_SEACHER, 1);
            bFT5202_IsKeyLock = 0;  //Added for new behavior (2009/09/27)
            ft5202_msg(INFO, "[TOUCH-CAP]virtual button Center - down!\r\n");
            bFT5202_CenterKeyPressed = 1;  //Added for FST
        }
        //Added for FST--
        //Modified for new CAP sample by Stanley --(2009/05/25)
		input_sync(input);
	} 
	#endif
	gpio_clear_detect_status(ft5202->client->irq);  
	enable_irq(ft5202->client->irq);
}

static irqreturn_t ft5202_isr(int irq, void * dev_id)
{
	//disable_irq(irq);
	schedule_work(&ft5202->wqueue);

	return IRQ_HANDLED;
}

static int input_open(struct input_dev * idev)
{
	struct i2c_client *client = ft5202->client;

    if (request_irq(client->irq, ft5202_isr, 2, TOUCH_NAME, ft5202)) {
        ft5202_msg(ERR, "can not register irq %d", client->irq);
		return -1;
    }

	return 0;
}

static void input_close(struct input_dev *idev)
{
	struct i2c_client *client = ft5202->client;
	
	free_irq(client->irq, ft5202);
}

static int ft5202_misc_open(struct inode *inode, struct file *file)
{
    if ((file->f_flags & O_ACCMODE) == O_WRONLY) {
		ft5202_msg(INFO, "device node is readonly");
        return -1;
    }

	return 0;
}

static int ft5202_misc_release(struct inode *inode, struct file *file)
{
    return 0;
}

static int ft5202_misc_ioctl(struct inode *inode, struct file *file,
									unsigned cmd, unsigned long arg)
{
	int value, ret = 0;
	//struct focal_i2c_resolution res;
	//struct focal_i2c_sensitivity sen;

	if (_IOC_TYPE(cmd) != FT5202_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > FT5202_IOC_MAXNR) return -ENOTTY;

	switch(cmd) {
	case FT5202_IOC_GFWVERSION:
		if (ft5202_get_fw_version(&value) < 0)
			return -EIO;
		ret = put_user(value, (int __user *)arg);
		break;
	
	case FT5202_IOC_GFWID:
		if (ft5202_get_vendor_id(&value) < 0)
			return -EIO;
		ret = put_user(value, (int __user *)arg);
        break;

	default:
		return -ENOTTY;
	}

	return ret;
}

static struct file_operations ft5202_misc_fops = {
    .open	= ft5202_misc_open,
    .ioctl	= ft5202_misc_ioctl,
    .release= ft5202_misc_release,
};

static struct miscdevice ft5202_misc_dev = {
    .minor= MISC_DYNAMIC_MINOR,
    .name = TOUCH_NAME,
	.fops = &ft5202_misc_fops,
};

#ifdef CONFIG_PM
static int ft5202_suspend(struct i2c_client *client, pm_message_t state)
{
    struct vreg *vreg_wlan;  //Add for VREG_WLAN power in, 07/08
    struct focal_i2c_platform_data *pdata = client->dev.platform_data;  //Setting the configuration of GPIO 89
    int ret, ret_gpio = 0;  //Add for VREG_WLAN power in, 07/08
    
	cancel_work_sync(&ft5202->wqueue);
    printk(KERN_INFO "ft5202_suspend() disable IRQ: %d\n", client->irq);
    disable_irq(client->irq);

    //Add for VREG_WLAN power in++
    if((FIH_READ_HWID_FROM_SMEM() != CMCS_7627_PR1) && (FIH_READ_HWID_FROM_SMEM() != CMCS_F913_PR1))  //Don't apply VREG_WLAN power in on PR1++
    {
        vreg_wlan = vreg_get(0, "wlan");

	    if (!vreg_wlan) {
		    printk(KERN_ERR "%s: vreg WLAN get failed\n", __func__);
		return -EIO;
	    }

	    ret = vreg_disable(vreg_wlan);
	    if (ret) {
		    printk(KERN_ERR "%s: vreg WLAN disable failed (%d)\n",
		        __func__, ret);
		return -EIO;
	    }

	    printk(KERN_INFO "%s: vote vreg WLAN to be closed\n", __func__);
	}
	//Add for VREG_WLAN power in--
	//return ft5202_set_pw_state(0);
	//Setting the configuration of GPIO 89++
	ret_gpio = gpio_tlmm_config(GPIO_CFG(pdata->intr_gpio, 0, GPIO_INPUT,
						GPIO_PULL_DOWN, GPIO_2MA), GPIO_ENABLE);
	if(ret_gpio < 0)
	{
	    msm_cap_touch_gpio_pull_fail = 1;
	    printk(KERN_INFO "ft5202_suspend(): GPIO89_PULL_DOWN failed!\n");
	}
	printk(KERN_INFO "ft5202_suspend(): GPIO89_PULL_DOWN\n");
	//Setting the configuration of GPIO 89--
	return 0; //Remove to set power state by Stanley
}

static int ft5202_resume(struct i2c_client *client)
{
    struct vreg *vreg_wlan;  //Add for VREG_WLAN power in, 07/08
    //struct focal_i2c_sensitivity sen;  //Added for modify sensitivity, 0729
    struct focal_i2c_platform_data *pdata = client->dev.platform_data;  //Setting the configuration of GPIO 89
    int ret, ret_gpio = 0;  //Added for modify sensitivity, 0729
        
//Remove to set power state by Stanley++
#if 0
	int state = 0,
		retry = 10;

	ft5202_set_pw_state(1);
	do {
		ft5202_get_pw_state(&state);
		if (--retry == 0) {
			ft5202_msg(ERR, "can not wake device up");
			return -1;
		}
	} while (!state);
#endif
//Remove to set power state by Stanley--
    //printk(KERN_INFO "ft5202_resume() enable IRQ: %d\n", client->irq);
	//enable_irq(client->irq);

    //Add for VREG_WLAN power in++
    if((FIH_READ_HWID_FROM_SMEM() != CMCS_7627_PR1) && (FIH_READ_HWID_FROM_SMEM() != CMCS_F913_PR1))  //Don't apply VREG_WLAN power in on PR1++
    {
        vreg_wlan = vreg_get(0, "wlan");

	    if (!vreg_wlan) {
		    printk(KERN_ERR "%s: vreg WLAN get failed\n", __func__);
		return -EIO;
	    }

	    ret = vreg_enable(vreg_wlan);
	    if (ret) {
		    printk(KERN_ERR "%s: vreg WLAN enable failed (%d)\n",
		        __func__, ret);
		return -EIO;
	    }
	}
	//Add for VREG_WLAN power in--
	//Setting the configuration of GPIO 89++
	ret_gpio = gpio_tlmm_config(GPIO_CFG(pdata->intr_gpio, 0, GPIO_INPUT,
						GPIO_PULL_UP, GPIO_2MA), GPIO_ENABLE);
	if(ret_gpio < 0)
	{
	    msm_cap_touch_gpio_pull_fail = 1;
	    printk(KERN_INFO "ft5202_suspend(): GPIO89_PULL_UP failed!\n");
	}
	//Setting the configuration of GPIO 89--
	//Modify the scheme for receive hello packet++
	printk(KERN_INFO "bi8232_resume() enable IRQ: %d and GPIO89_PULL_UP\n", client->irq);
	enable_irq(client->irq);
	//Modify the scheme for receive hello packet--
	
	return 0;
}
#else
#define ft5202_suspend	NULL
#define ft5202_resume	NULL
#endif

/* FIH, SimonSSChang, 2009/09/04 { */
/* [FXX_CR], change CAP touch suspend/resume function
to earlysuspend */
#ifdef CONFIG_FIH_FXX
#ifdef CONFIG_HAS_EARLYSUSPEND
void ft5202_early_suspend(struct early_suspend *h)
{
    struct ft5202_innolux *pft5202;
	pft5202 = container_of(h, struct ft5202_innolux, ft5202_early_suspend_desc);

    printk(KERN_INFO "ft5202_early_suspend()\n");
    //ft5202_suspend(pft5202->client, PMSG_SUSPEND);
    printk(KERN_INFO "ft5202_suspend() disable IRQ: %d\n", pft5202->client->irq);
    disable_irq(pft5202->client->irq);
    printk(KERN_INFO "ft5202_early_suspend() exit!\n");
}
void ft5202_late_resume(struct early_suspend *h)
{
    struct ft5202_innolux *pft5202;
	pft5202 = container_of(h, struct ft5202_innolux, ft5202_early_suspend_desc);

    printk(KERN_INFO "ft5202_late_resume()\n");
    //ft5202_resume(pft5202->client);
    printk(KERN_INFO "ft5202_resume() enable IRQ: %d\n", pft5202->client->irq);
	enable_irq(pft5202->client->irq);
	printk(KERN_INFO "ft5202_late_resume() exit!\n");
}
#endif	
#endif
/* } FIH, SimonSSChang, 2009/09/04 */

//Added by Stanley for dump scheme++
static int msm_seq_open(struct inode *inode, struct file *file)
{
	//printk(KERN_INFO "msm_open\n");
  	return single_open(file, NULL, NULL);
}

static ssize_t msm_seq_write(struct file *file, const char *buff, size_t len, loff_t *off)
{
	char str[64];
	int param = -1;
	int param2 = -1;
	int param3 = -1;
	char cmd[32];
	u32 sen_x = 2, sen_y = 3;

	//struct focal_i2c_sensitivity sen;

	printk(KERN_INFO "MSM_TOUCH_Write ~~\n");
	if(copy_from_user(str, buff, sizeof(str)))
		return -EFAULT;	

  	if(sscanf(str, "%s %d %d %d", cmd, &param, &param2, &param3) == -1)
	{
	  	printk("parameter format: <type> <value>\n");

 		return -EINVAL;
	}	  	

	if(!strnicmp(cmd, "sen", 3))
	{	
		sen_x = param;
		sen_y = param2;
		printk(KERN_INFO "sen param = %d\n",sen_x);
	}
	else
	{
		printk(KERN_INFO "Parameter error!\n");
	}

    //sen.x = sen_x;
    //sen.y = sen_y;
	//if (ft5202_set_sensitivity(&sen) < 0)
	    //{
            //ft5202_msg(INFO, "[TOUCH-CAP]ft5202_set_sensitivity failed!\r\n");
            //msleep(100);
        //}
	
	return len;
}

static ssize_t msm_seq_read(struct file *file, char __user *data, size_t count, loff_t *ppos)
{
    //int bytes, reg;
    int bytes;
    char cmd[4] = { 0x53, 0x20, 0x00, 0x01 };
        
    if (*ppos != 0)
        return 0;
    
#if 0
    reg = readl(TSSC_REG(CTL));
    bytes = sprintf(data, "[TOUCH]TSSC_CTL : 0x%x\r\n", reg);
    *ppos += bytes;
    data += bytes;

    reg = readl(TSSC_REG(OPN));
    bytes = sprintf(data, "[TOUCH]TSSC_OPN : 0x%x\r\n", reg);
    *ppos += bytes;
    data += bytes;

    reg = readl(TSSC_REG(SI));
    bytes = sprintf(data, "[TOUCH]TSSC_SAMPLING_INT : 0x%x\r\n", reg);
    *ppos += bytes;
    data += bytes;

    reg = readl(TSSC_REG(STATUS));
    bytes = sprintf(data, "[TOUCH]TSSC_STATUS : 0x%x\r\n", reg);
    *ppos += bytes;
    data += bytes;

    reg = readl(TSSC_REG(AVG12));
    bytes = sprintf(data, "[TOUCH]TSSC_AVG_12 : 0x%x\r\n", reg);
    *ppos += bytes;
    data += bytes;
#endif
    ft5202_send(cmd, ARRAY_SIZE(cmd));
	ft5202_recv(FT5202_buffer, ARRAY_SIZE(FT5202_buffer));
	bytes = sprintf(data, "[TOUCH]X-axis absolute : 0x%x, 0x%x, 0x%x, 0x%x\r\n", FT5202_buffer[0], FT5202_buffer[1], FT5202_buffer[2], FT5202_buffer[3]);
    *ppos += bytes;
    data += bytes;

    return *ppos;
}

static struct file_operations msm_touch_seq_fops =
{
  	.owner 		= THIS_MODULE,
	.open  		= msm_seq_open,
	.write 		= msm_seq_write,
	.read		= msm_seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
//Added by Stanley for dump scheme--

static int ft5202_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct input_dev *input;
	struct focal_i2c_platform_data *pdata;
	struct vreg *vreg_wlan;  //Add for VREG_WLAN power in, 09/10
	//struct focal_i2c_sensitivity sen;  //Added for modify sensitivity, 0729
	int iValue = 0, i, ret;
	//int retry = 50,			/* retry count of device detecting */	    
		//pkt;				/* packet FT5202_buffer */
	
	ft5202 = kzalloc(sizeof(struct ft5202_innolux), GFP_KERNEL);
	if (ft5202 == NULL) {
		ft5202_msg(ERR, "can not allocate memory for ft5202");
		return -ENOMEM;
	}

/* FIH, SimonSSChang, 2009/09/04 { */
/* [FXX_CR], change CAP touch suspend/resume function
to earlysuspend */
#ifdef CONFIG_FIH_FXX
#ifdef CONFIG_HAS_EARLYSUSPEND
    ft5202->ft5202_early_suspend_desc.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN - 11;
	ft5202->ft5202_early_suspend_desc.suspend = ft5202_early_suspend;
	ft5202->ft5202_early_suspend_desc.resume = ft5202_late_resume;
    printk(KERN_INFO "CAP_Touch register_early_suspend()\n");
	register_early_suspend(&ft5202->ft5202_early_suspend_desc);
#endif
#endif
/* } FIH, SimonSSChang, 2009/09/04 */
	
	ft5202->client = client;
	dev_set_drvdata(&client->dev, ft5202);

    //device_init_wakeup(&client->dev, 1);  //Added by Stanley (2010/02/25)
    #if 1
	for(i = 0 ; i < 5 ; i++)
	{
	    if (ft5202_get_vendor_id(&iValue) >= 0)
	    {
            ft5202_msg(INFO, "[TOUCH-CAP]ft5202 vendor id = 0x%x", iValue);        
  	    }
  	    else
  	    {
            ft5202_msg(INFO, "[TOUCH-CAP]Get ft5202 vendor failed!\r\n");
  	        if(i == 4)
            {
                ft5202_msg(INFO, "%s: return -ENODEV!\r\n", __func__);
                dev_set_drvdata(&client->dev, 0);
                unregister_early_suspend(&ft5202->ft5202_early_suspend_desc);
                kfree(ft5202);
                //ft5202_exit();
                
                return -ENODEV;
            }
  	    }
  	}
  	if (ft5202_get_fw_version(&iValue) >= 0)
	{
        ft5202_msg(INFO, "[TOUCH-CAP]ft5202 fw version = %d", iValue);
        msm_cap_touch_fw_version = iValue;
  	}
	#endif
    
	init_completion(&ft5202->data_ready);
	init_completion(&ft5202->data_complete);
	INIT_WORK(&ft5202->wqueue, ft5202_isr_workqueue);

	pdata = client->dev.platform_data;
	if (pdata == NULL) {
		ft5202_msg(ERR, "can not get platform data");
		goto err1;
	}

    input = input_allocate_device();
    if (input == NULL) {
		ft5202_msg(ERR, "can not allocate memory for input device");
        goto err1;
    }

    input->name = "Focal FT5202 Touchscreen";
    input->phys = "ft5202/input0";
    input->open = input_open;
    input->close= input_close;
	
    set_bit(EV_KEY, input->evbit);
    set_bit(EV_ABS, input->evbit);
    set_bit(EV_SYN, input->evbit);
    set_bit(BTN_TOUCH, input->keybit);
    set_bit(BTN_2, input->keybit);  //Added for Multi-touch
    set_bit(KEY_BACK, input->keybit);  //Modified for new CAP sample by Stanley (2009/05/25)
    set_bit(KEY_KBDILLUMDOWN, input->keybit);  //Modified for new CAP sample by Stanley (2009/05/25)
    //Modified for Home and AP key (2009/07/31)++
    if((FIH_READ_HWID_FROM_SMEM() >= CMCS_CTP_PR2) && (FIH_READ_HWID_FROM_SMEM() != CMCS_7627_PR1))
    {
        set_bit(KEY_HOME, input->keybit);
        set_bit(KEYCODE_BROWSER, input->keybit);
        set_bit(KEYCODE_SEACHER, input->keybit);
        set_bit(KEY_SEND, input->keybit);  //Added for FST
        set_bit(KEY_END, input->keybit);  //Added for FST
        bFT5202_IsFxxPR2 = 1; //Added for PR2
    }
    if((FIH_READ_HWID_FROM_SMEM() >= CMCS_F913_PR1) && (FIH_READ_HWID_FROM_SMEM() <= CMCS_F913_MP1))
        bFT5202_IsF913 = 1;
    else
        bFT5202_IsF913 = 0;
    //Modified for Home and AP key (2009/07/31)--

    //Added for FST++
    if((FIH_READ_ORIG_HWID_FROM_SMEM() >= CMCS_125_FST_PR1) && (FIH_READ_ORIG_HWID_FROM_SMEM() <= CMCS_128_FST_MP1))
        bFT5202_IsFST = 1;
    else
        bFT5202_IsFST = 0;
    //Added for FST--

    input_set_abs_params(input, ABS_X, pdata->abs_x_min,
								pdata->abs_x_max, 0, 0);
    input_set_abs_params(input, ABS_Y, pdata->abs_y_min,
								pdata->abs_y_max, 0, 0);
    input_set_abs_params(input, ABS_HAT0X, pdata->abs_x_min,
								pdata->abs_x_max, 0, 0);
    input_set_abs_params(input, ABS_HAT0Y, pdata->abs_y_min,
								pdata->abs_y_max, 0, 0);
	
    //Added the MT protocol for Eclair by Stanley (2010/03/23)++
    input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0,
								255, 0, 0);
    input_set_abs_params(input, ABS_MT_POSITION_X, pdata->abs_x_min,
								pdata->abs_x_max, 0, 0);
    input_set_abs_params(input, ABS_MT_POSITION_Y, pdata->abs_y_min,
								pdata->abs_y_max, 0, 0);
	//Added the MT protocol for Eclair by Stanley (2010/03/23)--

	ft5202->input = input;
    if (input_register_device(ft5202->input)) {
		ft5202_msg(ERR, "can not register input device");
        goto err2;
	}

	if (misc_register(&ft5202_misc_dev)) {
		ft5202_msg(ERR, "can not add misc device");
		goto err3;
    }

	if (MSM_GPIO_TO_INT(pdata->intr_gpio) != client->irq) {
		ft5202_msg(ERR, "irq not match");
		goto err4;
	}
	gpio_tlmm_config(GPIO_CFG(pdata->intr_gpio, 0, GPIO_INPUT,
						GPIO_PULL_UP, GPIO_2MA), GPIO_ENABLE);

    //Stanley: Add for increasing VREG_WLAN refcnt and let VREG_WLAN can be closed at first suspend +++
    if((FIH_READ_HWID_FROM_SMEM() != CMCS_7627_PR1) && (FIH_READ_HWID_FROM_SMEM() != CMCS_F913_PR1))  //Don't apply VREG_WLAN power in on PR1++
    {
        vreg_wlan = vreg_get(0, "wlan");

        if (!vreg_wlan) {
	        printk(KERN_ERR "%s: init vreg WLAN get failed\n", __func__);
        }

        ret = vreg_enable(vreg_wlan);
        if (ret) {
	        printk(KERN_ERR "%s: init vreg WLAN enable failed (%d)\n", __func__, ret);
        }
    }
    //Stanley: Add for increasing VREG_WLAN refcnt and let VREG_WLAN can be closed at first suspend ---	
	ft5202_msg(INFO, "[TOUCH-CAP]ft5202_probe init ok!\r\n");
	//Added by Stanley for dump scheme++
  	msm_touch_proc_file = create_proc_entry("driver/cap_touch", 0666, NULL);
	
	if(!msm_touch_proc_file){
	  	printk(KERN_INFO "create proc file for Msm_touch failed\n");
		return -ENOMEM;
	}

	printk(KERN_INFO "Msm_touch proc ok\n");
	msm_touch_proc_file->proc_fops = &msm_touch_seq_fops;
	//Added by Stanley for dump scheme--
    return 0;

err4:
	misc_deregister(&ft5202_misc_dev);
err3:
	input_unregister_device(ft5202->input);
err2:
	input_free_device(input);
err1:
	dev_set_drvdata(&client->dev, 0);
	kfree(ft5202);
	return -1;
}

static int ft5202_remove(struct i2c_client * client)
{
/* FIH, SimonSSChang, 2009/09/04 { */
/* [FXX_CR], change CAP touch suspend/resume function
to earlysuspend */
#ifdef CONFIG_FIH_FXX
#ifdef CONFIG_HAS_EARLYSUSPEND
    printk(KERN_INFO "CAP_Touch unregister_early_suspend()\n");
	unregister_early_suspend(&ft5202->ft5202_early_suspend_desc);
#endif
#endif
/* } FIH, SimonSSChang, 2009/09/04 */

	misc_deregister(&ft5202_misc_dev);
	input_unregister_device(ft5202->input);
    dev_set_drvdata(&client->dev, 0);
    kfree(ft5202);

	return 0;
}

static const struct i2c_device_id ft5202_id[] = {
    { TOUCH_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, ft5202);

static struct i2c_driver ft5202_i2c_driver = {
	.driver = {
		.name	= TOUCH_NAME,
		.owner	= THIS_MODULE,
	},
	.id_table   = ft5202_id,
	.probe  	= ft5202_probe,
	.remove 	= ft5202_remove,
/* FIH, SimonSSChang, 2009/09/04 { */
/* [FXX_CR], change CAP touch suspend/resume function
to earlysuspend */
#ifdef CONFIG_FIH_FXX
	.suspend	= ft5202_suspend,
    .resume		= ft5202_resume,
#else
	.suspend	= ft5202_suspend,
    .resume		= ft5202_resume,
#endif	
/* } FIH, SimonSSChang, 2009/09/04 */
};

static int __init ft5202_init( void )
{
    //struct vreg *vreg_wlan;  //Add for VREG_WLAN power in, 09/10
    //int ret;
	
    //Dynamic to load RES or CAP touch driver++
    if(FIH_READ_HWID_FROM_SMEM() >= CMCS_CTP_PR1)
    {
         msm_cap_touch_debug_mask_ft5202 = *(uint32_t *)TOUCH_DEBUG_MASK_OFFSET;  //Added for debug mask (2010/01/04)
         //Neo: Add for increasing VREG_WLAN refcnt and let VREG_WLAN can be closed at first suspend +++
         #if 0
         if((FIH_READ_HWID_FROM_SMEM() != CMCS_7627_PR1) && (FIH_READ_HWID_FROM_SMEM() != CMCS_F913_PR1))  //Don't apply VREG_WLAN power in on PR1++
         {
            vreg_wlan = vreg_get(0, "wlan");

            if (!vreg_wlan) {
	         printk(KERN_ERR "%s: init vreg WLAN get failed\n", __func__);
            }

            ret = vreg_enable(vreg_wlan);
            if (ret) {
	          printk(KERN_ERR "%s: init vreg WLAN enable failed (%d)\n", __func__, ret);
            }
         }
         #endif
         //Neo: Add for increasing VREG_WLAN refcnt and let VREG_WLAN can be closed at first suspend ---
		
	  return i2c_add_driver(&ft5202_i2c_driver);
    }
	else
	    return -ENODEV;
	//Dynamic to load RES or CAP touch driver--
}

static void __exit ft5202_exit( void )
{
	i2c_del_driver(&ft5202_i2c_driver);
}

module_init(ft5202_init);
module_exit(ft5202_exit);

MODULE_DESCRIPTION("Focaltech FT5202 Touchscreen driver");
MODULE_AUTHOR("Stanley Cheng <stanleycheng@fihtdc.com>");
MODULE_VERSION("0:1.4");
MODULE_LICENSE("GPL");
