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
#include <linux/panjit_i2c.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <mach/gpio.h>
#include <linux/earlysuspend.h>

//Dynamic to load RES or CAP touch driver++
#include <mach/msm_iomap.h>
#include <mach/msm_smd.h>
//Dynamic to load RES or CAP touch driver--
#include <linux/cmdbgapi.h>  //Added for debug mask (2010/01/04)
#include <linux/pm.h>  //Added for new kernel suspend scheme (2010.08.21)

/************************************************
 *  attribute marco
 ************************************************/
#define TOUCH_NAME		"panjit"	/* device name */
#define panjit_DEBUG	1			/* print out receive packet */

/************************************************
 *  function marco
 ************************************************/
#define panjit_msg(lv, fmt, arg...)	\
	printk(KERN_ ##lv"[%s@%s@%d] "fmt"\n",__FILE__,__func__,__LINE__,##arg)

/************************************************
 *  Global variable
 ************************************************/
u8 buffer[9];
bool bSoft1CapKeyPressed = 0;  
bool bSoft2CapKeyPressed = 0;  
bool bSoft3CapKeyPressed = 0; 
bool bSoft4CapKeyPressed = 0; 

bool g_bTP_ready = 0;
bool g_bSK_ready = 0;

static uint32_t msm_cap_touch_debug_mask;
module_param_named(
    debug_mask, msm_cap_touch_debug_mask, uint, S_IRUGO | S_IWUSR | S_IWGRP
);  //Added for debug mask (2010/01/04)

static int msm_cap_touch_fw_version;
module_param_named(
    fw_version, msm_cap_touch_fw_version, int, S_IRUGO | S_IWUSR | S_IWGRP
);

//Added by Stanley (2010.05.18)++
static int msm_cap_sk_fw_version;
module_param_named(
    sk_fw_version, msm_cap_sk_fw_version, int, S_IRUGO | S_IWUSR | S_IWGRP
);
//Added by Stanley (2010.05.18)--

int sk_intr_gpio;

int lastX=900,lastY=900;


//int64_t time = 0;

/************************************************
 * Control structure
 ************************************************/
struct panjit_m32emau {
	struct i2c_client *client;
	struct input_dev  *input;
	struct work_struct wqueue;
	struct work_struct sk_wqueue;    
#ifdef CONFIG_FIH_FXX
#ifdef CONFIG_HAS_EARLYSUSPEND
    struct early_suspend panjit_early_suspend_desc;
#endif
#endif
    
} *panjit;

static int panjit_recv(u8 *buf, u32 count)
{
        struct i2c_adapter *adap = panjit->client->adapter;
        struct i2c_msg msgs[]= {
		[0] = {
			.addr = panjit->client->addr,
            .flags = I2C_M_RD,
            .len = count,
            .buf = buf,
		},
	};

	return (i2c_transfer(adap, msgs, 1) < 0) ? -EIO : 0;
}

static int panjit_sk_recv(u8 *buf, u32 count)
{
        struct i2c_adapter *adap = panjit->client->adapter;
        struct i2c_msg msgs[]= {
		[0] = {
			.addr = 0x29,
            .flags = I2C_M_RD,
            .len = count,
            .buf = buf,
		},
	};

	return (i2c_transfer(adap, msgs, 1) < 0) ? -EIO : 0;
}


static int panjit_send(u8 *buf, u32 count)
{
	struct i2c_adapter *adap = panjit->client->adapter;
	struct i2c_msg msgs[]= {
		[0] = {
            .addr = panjit->client->addr,
			.flags = 0,
			.len = count,
			.buf = buf,
        },
    };

	return (i2c_transfer(adap, msgs, 1) < 0) ? -EIO : 0;
}

static int panjit_sk_send(u8 *buf, u32 count)
{
	struct i2c_adapter *adap = panjit->client->adapter;
	struct i2c_msg msgs[]= {
		[0] = {
            .addr =0x29,
			.flags = 0,
			.len = count,
			.buf = buf,
        },
    };

	return (i2c_transfer(adap, msgs, 1) < 0) ? -EIO : 0;
}


static int panjit_write_reg(u8 reg, u8 data)
{
    int ret;
    buffer[0]=reg;
    buffer[1]=data;
    ret = panjit_send(buffer, 2);
    if(ret<0)
    {
        //Fail
        panjit_msg(ERR, "[FM6] panjit_write_reg failed 1 err = %d, reg = 0x%x, data = 0x%x\r\n",ret, reg, data); 
    }
    else
    {
        //panjit_msg(INFO, "[FM6] reg 0x%x = 0x%x\r\n", reg, data); 
    }
    
    return ret;
}

static int panjit_sk_write_reg(u8 reg, u8 data)
{
    int ret;
    buffer[0]=reg;
    buffer[1]=data;
    ret = panjit_sk_send(buffer, 2);
    if(ret<0)
    {
        //Fail
        panjit_msg(ERR, "[FM6] panjit_sk_write_reg failed 1 err = %d, reg = 0x%x, data = 0x%x\r\n",ret, reg, data); 
    }
    else
    {
        //panjit_msg(INFO, "[FM6] SK reg 0x%x = 0x%x\r\n", reg, data); 
    }
    
    return ret;
}


static int panjit_read_reg(u8 reg, u8 *data, u32 count)
{
    int ret;
    ret = panjit_send(&reg, sizeof(u8));
    if(ret<0)
    {
        //Fail
        panjit_msg(ERR, "[FM6] panjit_read_reg failed 1 err = %d, reg = 0x%x\r\n",ret, reg); 
        
        return ret;
    }
    else
    {
        ret = panjit_recv(data, count);

        if(ret < 0)
        {
            //Fail
            panjit_msg(ERR, "[FM6] panjit_read_reg failed 2 err = %d\r\n",ret); 
        }
        else
        {
            //panjit_msg(INFO, "[FM6]reg 0x%x = 0x%x\r\n", reg, *data); 
        }
    }
    return ret;
}

static int panjit_sk_read_reg(u8 reg, u8 *data)
{
    int ret;
    ret = panjit_sk_send(&reg, sizeof(u8));
    if(ret<0)
    {
        //Fail
        panjit_msg(ERR, "[FM6] panjit_sk_read_reg failed 1 err = %d, reg = 0x%x\r\n",ret, reg); 
        
        return ret;
    }
    else
    {
        ret = panjit_sk_recv(data, sizeof(u8));

        if(ret < 0)
        {
            //Fail
            panjit_msg(ERR, "[FM6] panjit_sk_read_reg failed 2 err = %d\r\n",ret); 
        }
        else
        {
            //panjit_msg(INFO, "[FM6]SK reg 0x%x = 0x%x\r\n", reg, *data); 
        }
    }
    return ret;
}


#define FW_MAJOR(x) ((((int)((x)[1])) & 0xF) << 4) + ((((int)(x)[2]) & 0xF0) >> 4)
#define FW_MINOR(x) ((((int)((x)[2])) & 0xF) << 4) + ((((int)(x)[3]) & 0xF0) >> 4)
static int panjit_get_fw_version(int *version)
{
    char cmd = 0x03;
    char buf = 0;
    panjit_send(&cmd, sizeof(char));
    panjit_recv(&buf, sizeof(char));
    *version = buf;
    panjit_msg(ERR, "[FM6] buf = %d,addr=0x%x\r\n", buf, panjit->client->addr); 

	return 0;
}

//Added by Stanley (2010.05.18)++
static int panjit_get_sk_fw_version(int *version)
{
    char cmd = 0x07;
    char buf = 0;
    panjit_sk_send(&cmd, sizeof(char));
    panjit_sk_recv(&buf, sizeof(char));
    *version = buf;
    panjit_msg(ERR, "[FM6] buf = %d,addr=0x%x\r\n", buf, panjit->client->addr); 

	return 0;
}
//Added by Stanley (2010.05.18)--
#define FWID_HBYTE(x) (((((int)(x)[1]) & 0xF) << 4) + ((((int)(x)[2]) & 0xF0) >> 4))
#define FWID_LBYTE(x) (((((int)(x)[2]) & 0xF) << 4) + ((((int)(x)[3]) & 0xF0) >> 4))
static int panjit_get_fw_id(int *id)
{
	char cmd[4] = { 0x53, 0xF0, 0x00, 0x01 };

	panjit_send(cmd, ARRAY_SIZE(cmd));

	if (buffer[0] != 0x52) {
        return -1;
    }

	*id = (FWID_HBYTE(buffer) << 8) + FWID_LBYTE(buffer);

	return 0;
}

#if 0
static int panjit_get_report_rate(int *rate)
{
	char cmd[4] = { 0x53, 0xB0, 0x00, 0x01 };

	panjit_send(cmd, ARRAY_SIZE(cmd));

	if (res[0] != 0x52) {
        return -1;
    }

	*rate = buffer[1] & 0xF;

	return 0;
}

static int panjit_set_report_rate(unsigned rate)
{
	char cmd[4] = { 0x54, 0xE0, 0x00, 0x01 };

	if (rate > 5) rate = 5;
	cmd[1] |= (char)rate;

	if (panjit_send(cmd, ARRAY_SIZE(cmd)) < 0) {
        panjit_msg(ERR, "set report rate failed");
        return -1;
    }

    return 0;
}
#endif

static void panjit_isr_workqueue(struct work_struct *work)
{
    struct input_dev *input = panjit->input;
    int tempX = 0,tempY = 0;
    disable_irq(panjit->client->irq);

    panjit_read_reg(0x11,&buffer[8], 1);

    //input_report_key(input, BTN_TOUCH, (buffer[8]>0));    
    //input_report_key(input, BTN_2, (buffer[8]==2));

    
    if (buffer[8]) 
    {
#if 0    //for virtual softkey
        panjit_read_reg(0x09,&buffer[0], 1);
        panjit_read_reg(0x0A,&buffer[1], 1);
        panjit_read_reg(0x0B,&buffer[2], 1);
        panjit_read_reg(0x0C,&buffer[3], 1);
        tempX = 800 - (buffer[0] * 256 + buffer[1]);
        tempY = 480 - (buffer[2] * 256 + buffer[3]);
        if(tempX < 50)
        {
            if(tempY < 240)
            {
                input_report_key(input, KEY_BACK, 1);
                bSoft1CapKeyPressed = 1;  
            }
            else
            {
                input_report_key(input, KEY_KBDILLUMDOWN, 1);
                bSoft2CapKeyPressed = 1;  
            }
        }
        else
        {
            input_report_key(input, BTN_TOUCH, 1);    
            input_report_abs(input, ABS_X, tempX);  //Add for protect origin point
            input_report_abs(input, ABS_Y, tempY);            
        }
#else
/*
        panjit_read_reg(0x09,&buffer[0], 1);
        panjit_read_reg(0x0A,&buffer[1], 1);
        panjit_read_reg(0x0B,&buffer[2], 1);
        panjit_read_reg(0x0C,&buffer[3], 1);
*/
        panjit_read_reg(0x09, &buffer[0], 4);


        tempX = 800 - (buffer[0] * 256 + buffer[1]);
        tempY = 480 - (buffer[2] * 256 + buffer[3]);
//#if 0    
        if(buffer[8] == 1){  //Added the dynamic SW filter by Stanley (2010/07/22)
        if((lastX - tempX) > 5 )
        {
            lastX = tempX;
            lastY = tempY;
            //input_report_abs(input, ABS_X, tempX);  //Add for protect origin point
            //input_report_abs(input, ABS_Y, tempY);
            //Added the MT protocol for Eclair by Stanley (2010/06/23)++
            input_report_abs(input, ABS_MT_TOUCH_MAJOR, 255);
			input_report_abs(input, ABS_MT_POSITION_X, tempX);
			input_report_abs(input, ABS_MT_POSITION_Y, tempY);
			input_mt_sync(input);
            //Added the MT protocol for Eclair by Stanley (2010/06/23)--
        }
        else if((lastX - tempX) < -5)
        {
            lastX = tempX;
            lastY = tempY;        
            //input_report_abs(input, ABS_X, tempX);  //Add for protect origin point
            //input_report_abs(input, ABS_Y, tempY);
            //Added the MT protocol for Eclair by Stanley (2010/06/23)++
            input_report_abs(input, ABS_MT_TOUCH_MAJOR, 255);
			input_report_abs(input, ABS_MT_POSITION_X, tempX);
			input_report_abs(input, ABS_MT_POSITION_Y, tempY);
			input_mt_sync(input);
            //Added the MT protocol for Eclair by Stanley (2010/06/23)--
        }
        else if((lastY - tempY) > 5)
        {
            lastX = tempX;
            lastY = tempY;        
            //input_report_abs(input, ABS_X, tempX);  //Add for protect origin point
            //input_report_abs(input, ABS_Y, tempY);
            //Added the MT protocol for Eclair by Stanley (2010/06/23)++
            input_report_abs(input, ABS_MT_TOUCH_MAJOR, 255);
			input_report_abs(input, ABS_MT_POSITION_X, tempX);
			input_report_abs(input, ABS_MT_POSITION_Y, tempY);
			input_mt_sync(input);
            //Added the MT protocol for Eclair by Stanley (2010/06/23)--
        }
        else if((lastY - tempY) < -5)
        {
            lastX = tempX;
            lastY = tempY;        
            //input_report_abs(input, ABS_X, tempX);  //Add for protect origin point
            //input_report_abs(input, ABS_Y, tempY);
            //Added the MT protocol for Eclair by Stanley (2010/06/23)++
            input_report_abs(input, ABS_MT_TOUCH_MAJOR, 255);
			input_report_abs(input, ABS_MT_POSITION_X, tempX);
			input_report_abs(input, ABS_MT_POSITION_Y, tempY);
			input_mt_sync(input);
            //Added the MT protocol for Eclair by Stanley (2010/06/23)--
        }
        else
        {
            //panjit_msg(ERR, "[FM6] Don't report X|Y!!\r\n"); 
            fih_printk(msm_cap_touch_debug_mask, FIH_DEBUG_ZONE_G1, "[FM6] Don't report X|Y (while only one finger)!\r\n");  //Added the dynamic SW filter by Stanley (2010/07/22)  
        }
        }
//#endif
        else if (buffer[8] > 1)  //Added the dynamic SW filter by Stanley (2010/07/22)
        {
            //Remove SW filter by Stanley (2010/07/19)++
            input_report_abs(input, ABS_MT_TOUCH_MAJOR, 255);
		    input_report_abs(input, ABS_MT_POSITION_X, tempX);
		    input_report_abs(input, ABS_MT_POSITION_Y, tempY);
		    input_mt_sync(input);
            //Remove SW filter by Stanley (2010/07/19)--
        }
#endif
        fih_printk(msm_cap_touch_debug_mask, FIH_DEBUG_ZONE_G1, "[FM6] XCORD1 = 0x%x (%d)\r\n", tempX, tempX);  
        fih_printk(msm_cap_touch_debug_mask, FIH_DEBUG_ZONE_G1, "[FM6] YCORD1 = 0x%x (%d)\r\n", tempY, tempY);  
        //panjit_msg(ERR, "[FM6] XCORD1 = 0x%x (%d)\r\n",tempX,tempX); 
        //panjit_msg(ERR, "[FM6] YCORD1 = 0x%x (%d)\r\n",tempY,tempY);         
    }
    else
    {
#if 0    
        if(bSoft1CapKeyPressed)
        {
            input_report_key(input, KEY_BACK, 0);
            bSoft1CapKeyPressed = 0;
            panjit_msg(ERR, "[FM6] KEY_BACK\r\n");         
            
        }
        else if (bSoft2CapKeyPressed)
        {
            input_report_key(input, KEY_KBDILLUMDOWN, 0);
            bSoft2CapKeyPressed = 0;
            panjit_msg(ERR, "[FM6] KEY_KBDILLUMDOWN\r\n");         
            
        }
        else
        {
            input_report_key(input, BTN_TOUCH, 0);    
            panjit_msg(ERR, "[FM6] NON-Touch\r\n"); 
        }
#else
        //input_report_key(input, BTN_TOUCH, 0);    
        //panjit_msg(ERR, "[FM6] NON-Touch\r\n");
        //Added the MT protocol for Eclair by Stanley (2010/06/23)++
        input_report_abs(input, ABS_MT_TOUCH_MAJOR, 0);
		input_report_abs(input, ABS_MT_POSITION_X, tempX);
		input_report_abs(input, ABS_MT_POSITION_Y, tempY);
		input_mt_sync(input);
        //Added the MT protocol for Eclair by Stanley (2010/06/23)--
        //Added for SW filter fine tune (2010/08/03)++
        lastX = 0;
        lastY = 0;
        //Added for SW filter fine tune (2010/08/03)--
        fih_printk(msm_cap_touch_debug_mask, FIH_DEBUG_ZONE_G1, "[FM6] NON-Touch\r\n");  
        
#endif
    }
    
    if (buffer[8] > 1) 
    {
/*    
        panjit_read_reg(0x0D,&buffer[4], 1);
        panjit_read_reg(0x0E,&buffer[5], 1);
        panjit_read_reg(0x0F,&buffer[6], 1);
        panjit_read_reg(0x10,&buffer[7], 1);
*/
        panjit_read_reg(0x0D, &buffer[4], 4);
        tempX = 800 - (buffer[4] * 256 + buffer[5]);
        tempY = 480 - (buffer[6] * 256 + buffer[7]);
        //input_report_abs(input, ABS_HAT0X, tempX);  //Add for protect origin point
        //input_report_abs(input, ABS_HAT0Y, tempY);
        //Added the MT protocol for Eclair by Stanley (2010/06/23)++
        input_report_abs(input, ABS_MT_TOUCH_MAJOR, 255);
		input_report_abs(input, ABS_MT_POSITION_X, tempX);
		input_report_abs(input, ABS_MT_POSITION_Y, tempY);
		input_mt_sync(input);
        //Added the MT protocol for Eclair by Stanley (2010/06/23)--
        //panjit_msg(ERR, "[FM6] buffer[8]=%d\r\n",buffer[8]); 
        //panjit_msg(ERR, "[FM6] XCORD2 = 0x%x (%d)\r\n",tempX, tempX); 
        //panjit_msg(ERR, "[FM6] YCORD2 = 0x%x (%d)\r\n",tempY, tempY); 
        fih_printk(msm_cap_touch_debug_mask, FIH_DEBUG_ZONE_G1, "[FM6] XCORD2 = 0x%x (%d)\r\n", tempX, tempX);  
        fih_printk(msm_cap_touch_debug_mask, FIH_DEBUG_ZONE_G1, "[FM6] YCORD2 = 0x%x (%d)\r\n", tempY, tempY);  
        
    }  
    //panjit_msg(ERR, "[FM6] input_sync in");     
    input_sync(input);
    //panjit_msg(ERR, "[FM6] input_sync time out"); 

    panjit_write_reg(0x7,0x1);

    //gpio_clear_detect_status(panjit->client->irq);  
    //Modify the scheme for receive hello packet--
    enable_irq(panjit->client->irq);

    
}

static void panjit_sk_isr_workqueue(struct work_struct *work)
{
    struct input_dev *input = panjit->input;
    disable_irq(MSM_GPIO_TO_INT(sk_intr_gpio));

    panjit_sk_read_reg(0x3,&buffer[8]);
    
    //panjit_msg(ERR, "[FM6] SK key = %d\r\n",buffer[8]); 
    
    if (buffer[8]) 
    {
            if(buffer[8] == 0x2)
            {
                if(!bSoft1CapKeyPressed)
                {
                    input_report_key(input, KEY_SEARCH, 1);
                    bSoft1CapKeyPressed = 1;  
                    fih_printk(msm_cap_touch_debug_mask, FIH_DEBUG_ZONE_G2, "[FM6] KEY_SEARCH down\r\n");                      
                    //panjit_msg(ERR, "[FM6] KEY_SEARCH down\r\n");         
                }
            }
            else if(buffer[8] == 0x4)
            {
                if(!bSoft2CapKeyPressed)
                {
                    input_report_key(input, KEY_HOME, 1);
                    bSoft2CapKeyPressed = 1;  
                    fih_printk(msm_cap_touch_debug_mask, FIH_DEBUG_ZONE_G2, "[FM6] KEY_HOME down\r\n");                                          
                    //panjit_msg(ERR, "[FM6] KEY_HOME down\r\n");         
                }
            }
            else if(buffer[8] == 0x8)
            {
                if(!bSoft3CapKeyPressed)            
                {
                    input_report_key(input, KEY_KBDILLUMDOWN, 1);
                    bSoft3CapKeyPressed = 1;  
                    fih_printk(msm_cap_touch_debug_mask, FIH_DEBUG_ZONE_G2, "[FM6] KEY_KBDILLUMDOWN down\r\n");                                                              
                    //panjit_msg(ERR, "[FM6] KEY_KBDILLUMDOWN down\r\n");         
                }
            }
            else if(buffer[8] == 0x1)
            {
                if(!bSoft4CapKeyPressed)     
                {
                    input_report_key(input, KEY_BACK, 1);
                    bSoft4CapKeyPressed = 1;  
                    fih_printk(msm_cap_touch_debug_mask, FIH_DEBUG_ZONE_G2, "[FM6] KEY_BACK down\r\n");                                                              
                    //panjit_msg(ERR, "[FM6] KEY_BACK down\r\n");         
                }
            }
            else 
            {
                fih_printk(msm_cap_touch_debug_mask, FIH_DEBUG_ZONE_G2, "[FM6] multi-keys pressed key value = %d\r\n",buffer[8]);                                                              
                //panjit_msg(ERR, "[FM6] multi-keys pressed key value = %d!!!\r\n",buffer[8]);         
            }

    }
    else
    {
        if(bSoft1CapKeyPressed)
        {
            input_report_key(input, KEY_SEARCH, 0);
            bSoft1CapKeyPressed = 0;
            fih_printk(msm_cap_touch_debug_mask, FIH_DEBUG_ZONE_G2, "[FM6] KEY_SEARCH up\r\n");                                                              
            //panjit_msg(ERR, "[FM6] KEY_SEARCH up\r\n");         
            
        }
        else if (bSoft2CapKeyPressed)
        {
            input_report_key(input, KEY_HOME, 0);
            bSoft2CapKeyPressed = 0;
            fih_printk(msm_cap_touch_debug_mask, FIH_DEBUG_ZONE_G2, "[FM6] KEY_HOME up\r\n");                                                              
            //panjit_msg(ERR, "[FM6] KEY_HOME up\r\n");         
            
        }
        else if (bSoft3CapKeyPressed)
        {
            input_report_key(input, KEY_KBDILLUMDOWN, 0);
            bSoft3CapKeyPressed = 0;
            fih_printk(msm_cap_touch_debug_mask, FIH_DEBUG_ZONE_G2, "[FM6] KEY_KBDILLUMDOWN up\r\n");                                                              
            //panjit_msg(ERR, "[FM6] KEY_KBDILLUMDOWN up\r\n");                     
        }
        else if (bSoft4CapKeyPressed)
        {
            input_report_key(input, KEY_BACK, 0);
            bSoft4CapKeyPressed = 0;
            fih_printk(msm_cap_touch_debug_mask, FIH_DEBUG_ZONE_G2, "[FM6] KEY_BACK up\r\n");                                                              
            //panjit_msg(ERR, "[FM6] KEY_BACK up\r\n");         
            
        }
        else
        {
            input_report_key(input, BTN_TOUCH, 0);   
            fih_printk(msm_cap_touch_debug_mask, FIH_DEBUG_ZONE_G2, "[FM6] NON-Touch\r\n");                                                              
            //panjit_msg(ERR, "[FM6] NON-Touch\r\n"); 
        }
    }
            


    input_sync(input);

    panjit_sk_write_reg(0x1,0x0);

    //gpio_clear_detect_status(MSM_GPIO_TO_INT(sk_intr_gpio));  
    //Modify the scheme for receive hello packet--
    enable_irq(MSM_GPIO_TO_INT(sk_intr_gpio));
}


static irqreturn_t panjit_isr(int irq, void * dev_id)
{
	//disable_irq(irq);
	schedule_work(&panjit->wqueue);

	return IRQ_HANDLED;
}

static irqreturn_t panjit_sk_isr(int irq, void * dev_id)
{
	//disable_irq(irq);
	schedule_work(&panjit->sk_wqueue);

	return IRQ_HANDLED;
}

static int input_open(struct input_dev * idev)
{
    struct i2c_client *client = panjit->client;

    if(g_bTP_ready)
    {
        if (request_irq(client->irq, panjit_isr, 2, TOUCH_NAME, panjit)) {
            panjit_msg(ERR, "can not register irq %d", client->irq);
    		return -1;
        }
    }
    
    if(g_bSK_ready)
    {
        if (request_irq(MSM_GPIO_TO_INT(sk_intr_gpio), panjit_sk_isr, 2, TOUCH_NAME, panjit)) {
            panjit_msg(ERR, "[FM6] can not register irq %d", MSM_GPIO_TO_INT(sk_intr_gpio));
    		return -1;
        }
    }
    
    return 0;
}

static void input_close(struct input_dev *idev)
{
    struct i2c_client *client = panjit->client;

    if(g_bTP_ready)
        free_irq(client->irq, panjit);

    if(g_bSK_ready)
        free_irq(MSM_GPIO_TO_INT(sk_intr_gpio), panjit);

}

static int panjit_misc_open(struct inode *inode, struct file *file)
{
    if ((file->f_flags & O_ACCMODE) == O_WRONLY) {
		panjit_msg(INFO, "device node is readonly");
        return -1;
    }

	return 0;
}

static int panjit_misc_release(struct inode *inode, struct file *file)
{
    return 0;
}

static int panjit_misc_ioctl(struct inode *inode, struct file *file,
									unsigned cmd, unsigned long arg)
{
	int value, ret = 0;
    
	if (_IOC_TYPE(cmd) != PANJIT_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > PANJIT_IOC_MAXNR) return -ENOTTY;

	switch(cmd) {
	case PANJIT_IOC_GFWVERSION:
		if (panjit_get_fw_version(&value) < 0)
			return -EIO;
		ret = put_user(value, (int __user *)arg);
		break;

	case PANJIT_IOC_GFWID:
		if (panjit_get_fw_id(&value) < 0)
			return -EIO;
		ret = put_user(value, (int __user *)arg);
        break;
        
	default:
		return -ENOTTY;
	}

	return ret;
}

static struct file_operations panjit_misc_fops = {
    .open	= panjit_misc_open,
    .ioctl	= panjit_misc_ioctl,
    .release= panjit_misc_release,
};

static struct miscdevice panjit_misc_dev = {
    .minor= MISC_DYNAMIC_MINOR,
    .name = TOUCH_NAME,
	.fops = &panjit_misc_fops,
};

static int panjit_remove(struct i2c_client * client)
{
    misc_deregister(&panjit_misc_dev);
    input_unregister_device(panjit->input);
    dev_set_drvdata(&client->dev, 0);
    kfree(panjit);

    return 0;
}

#ifdef CONFIG_PM
//static int panjit_suspend(struct i2c_client *client, pm_message_t state)
static int panjit_suspend(struct device *client)  //Added for new kernel suspend scheme (2010.08.21)
{
    //int ret;  //Add for VREG_WLAN power in, 07/08

    cancel_work_sync(&panjit->wqueue);
    cancel_work_sync(&panjit->sk_wqueue);
    
    //disable_irq(client->irq);
    disable_irq(panjit->client->irq);  //Added for new kernel suspend scheme (2010.08.21)
    disable_irq(MSM_GPIO_TO_INT(sk_intr_gpio));

    //Added for deep sleep mode by Stanley++
    gpio_set_value(23, 1);
    mdelay(1);
    gpio_set_value(23,0);
    mdelay(10);
    //Added for deep sleep mode by Stanley--

    panjit_sk_write_reg(0x1,0x0);

    panjit_sk_write_reg(0x0,0x88);  //Added for enable SCAN_EN bit (2010.05.18) 
    panjit_write_reg(0x6,0x0); 

    g_bTP_ready = 0;
    g_bSK_ready = 0;

    printk(KERN_ERR "%s: panjit_suspend\n", __func__);
    

    printk(KERN_ERR "%s: panjit_suspend done!\n", __func__);

    return 0; 
}
//static int panjit_resume(struct i2c_client *client)
static int panjit_resume(struct device *client)  //Added for new kernel suspend scheme (2010.08.21)
{
    int ret;  

#if 0
    if (!vreg_wlan) 
    {
        vreg_wlan = vreg_get(0, "wlan");
        if(!vreg_wlan)
        {
            printk(KERN_ERR "%s: vreg WLAN get failed\n", __func__);
            return -EIO;
        }
    }

    ret = vreg_enable(vreg_wlan);
    if (ret) 
    {
        printk(KERN_ERR "%s: vreg WLAN enable failed (%d)\n", __func__, ret);
        return -EIO;
    }
#endif

    gpio_set_value(23, 1);
    gpio_set_value(84, 1);
    mdelay(1);
    gpio_set_value(23,0);
    gpio_set_value(84,0);
    mdelay(10);

//TP
    ret = panjit_write_reg(0x7,0x1); //Clear INT
    if (ret <0) 
    {
	 panjit_msg(ERR, "[FM6]Touch : clear INT flag fail!");
    }
    else
    {
        ret= panjit_write_reg(0x4,0x10); //Enable auto low power
        if (ret <0) 
        {
        	 panjit_msg(ERR, "[FM6]Touch : Enable auto low power failed!");
        }
        else
        {
            //ret = panjit_write_reg(0x4,0x18); //Enable scan
            ret = panjit_write_reg(0x4,0x08); //Enable scan and disable auto LP mode
            if (ret <0) 
            {
                panjit_msg(ERR, "[FM6]Touch : Enable scan failed!");
            }
            else
            {
                g_bTP_ready = 1;
                panjit_msg(INFO, "[FM6]Touch : resume done!");
            }
        }
    }


//SK
    ret = panjit_sk_write_reg(0x1,0x0); //Clear INT
    if (ret <0) 
    {
	 panjit_msg(ERR, "[FM6]SK : clear INT flag fail!");
    }
    else
    {
        panjit_msg(ERR, "[FM6]SK :  resume done!");
    }    

    //enable_irq(client->irq);
    enable_irq(panjit->client->irq);  //Added for new kernel suspend scheme (2010.08.21)
    enable_irq(MSM_GPIO_TO_INT(sk_intr_gpio));    

    
    return 0;
}
#else
#define panjit_suspend	NULL
#define panjit_resume	NULL
#endif

#ifdef CONFIG_FIH_FXX
#ifdef CONFIG_HAS_EARLYSUSPEND
void panjit_early_suspend(struct early_suspend *h)
{
    struct panjit_m32emau *ppanjit;
    ppanjit = container_of(h, struct panjit_m32emau, panjit_early_suspend_desc);

    printk(KERN_INFO "panjit_early_suspend()\n");
    //panjit_suspend(ppanjit->client, PMSG_SUSPEND);
    printk(KERN_INFO "panjit_suspend() disable IRQ: %d\n", ppanjit->client->irq);


    disable_irq(ppanjit->client->irq);
    disable_irq(MSM_GPIO_TO_INT(sk_intr_gpio));

    panjit_sk_write_reg(0x0,0x00); //Disable scan
    panjit_sk_write_reg(0x1,0x0);


    printk(KERN_INFO "panjit_early_suspend() exit!\n");
}
void panjit_late_resume(struct early_suspend *h)
{
    struct panjit_m32emau *ppanjit;
    int ret;
    
    ppanjit = container_of(h, struct panjit_m32emau, panjit_early_suspend_desc);
    
    printk(KERN_INFO "panjit_late_resume()\n");
    
    ret = panjit_sk_write_reg(0x0,0x08); //Enable scan
    if (ret <0) 
    {
        panjit_msg(ERR, "[FM6]SK :  Enable can failed!");
    }
    else
    {
        g_bSK_ready = 1;       
        panjit_msg(ERR, "[FM6]SK :  resume done!");
    }

    //Added for FM6E.B-354 (2010.07.05)++
    ret = panjit_sk_write_reg(0x06,0x00); //Clear SK BKL reg during resume
    if (ret <0) 
    {
        panjit_msg(ERR, "[FM6]SK :  Clear SK BKL failed!");
    }
    else
    {
        panjit_msg(ERR, "[FM6]SK :  Clear SK BKL done!");
    }
    mdelay(15);
    
    ret = panjit_sk_write_reg(0x06,0x01); //Enable SK BKL during resume
    if (ret <0) 
    {
        panjit_msg(ERR, "[FM6]SK :  Enable SK BKL failed!");
    }
    else
    {
        panjit_msg(ERR, "[FM6]SK :  Enable SK BKL done!");
    }
    //Added for FM6E.B-354 (2010.07.05)--
    
    enable_irq(ppanjit->client->irq);
    enable_irq(MSM_GPIO_TO_INT(sk_intr_gpio));    
    printk(KERN_INFO "panjit_late_resume() exit!\n");
}
#endif	
#endif


static int panjit_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct input_dev *input;
    struct panjit_i2c_platform_data *pdata;
    int iValue = 0;  
    int ret;

    panjit = kzalloc(sizeof(struct panjit_m32emau), GFP_KERNEL);
    
    if (panjit == NULL) 
    {
        panjit_msg(ERR, "can not allocate memory for panjit");
        return -ENOMEM;
    }

#ifdef CONFIG_FIH_FXX
#ifdef CONFIG_HAS_EARLYSUSPEND
    panjit->panjit_early_suspend_desc.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN - 11;
    panjit->panjit_early_suspend_desc.suspend = panjit_early_suspend;
    panjit->panjit_early_suspend_desc.resume = panjit_late_resume;
    printk(KERN_INFO "CAP_Touch register_early_suspend()\n");
    register_early_suspend(&panjit->panjit_early_suspend_desc);
#endif
#endif

    panjit->client = client;
    dev_set_drvdata(&client->dev, panjit);

    gpio_set_value(23,1);
    gpio_set_value(84,1);    
    mdelay(1);
    gpio_set_value(23,0);
    gpio_set_value(84,0);    
    mdelay(10);    

//TP
    ret = panjit_write_reg(0x7,0x1); //Clear INT
    if (ret <0) 
    {
	 panjit_msg(ERR, "[FM6]Touch : clear INT flag fail!");
    }
    else
    {
        ret = panjit_write_reg(0x4,0x10); //Enable auto low power
        if (ret <0) 
        {
        	 panjit_msg(ERR, "[FM6]Touch : Enable auto low power failed!");
        }
        else
        {
            //ret = panjit_write_reg(0x4,0x18); //Enable scan
            ret = panjit_write_reg(0x4,0x08); //Enable scan and disable auto LP mode  //Added for deep sleep mode by Stanley
            if (ret <0) 
            {
                panjit_msg(ERR, "[FM6]Touch : Enable scan failed!");
            }
            else
            {
                g_bTP_ready = 1;
                panjit_msg(INFO, "[FM6]Touch : INIT done!");
            }
        }
    }


//SK
    ret = panjit_sk_write_reg(0x1,0x0); //Clear INT
    if (ret <0) 
    {
	 panjit_msg(ERR, "[FM6]SK : clear INT flag fail!");
    }
    else
    {
        ret = panjit_sk_write_reg(0x0,0x08); //Enable scan
        if (ret <0) 
        {
            panjit_msg(ERR, "[FM6]SK :  Enable can failed!");
        }
        else
        {
            g_bSK_ready = 1;               
            panjit_msg(ERR, "[FM6]SK :  INIT done!");
        }
    }    

    if(g_bTP_ready)
        INIT_WORK(&panjit->wqueue, panjit_isr_workqueue);

    if(g_bSK_ready)
        INIT_WORK(&panjit->sk_wqueue, panjit_sk_isr_workqueue);
    
    pdata = client->dev.platform_data;
    
    if (pdata == NULL) 
    {
        panjit_msg(ERR, "can not get platform data");
        goto err1;
    }

    sk_intr_gpio = pdata->sk_intr_gpio;
    panjit_msg(ERR, "SK IRQ GPIO = %d",sk_intr_gpio);


    input = input_allocate_device();
    if (input == NULL) {
        panjit_msg(ERR, "can not allocate memory for input device");
        goto err1;
    }

    input->name = "PANJIT Touchscreen";
    input->phys = "panjit/input0";
    input->open = input_open;
    input->close= input_close;
	
    set_bit(EV_KEY, input->evbit);
    set_bit(EV_ABS, input->evbit);
    set_bit(EV_SYN, input->evbit);
    set_bit(BTN_TOUCH, input->keybit);
    set_bit(BTN_2, input->keybit);  
    set_bit(KEY_BACK, input->keybit);  
    set_bit(KEY_KBDILLUMDOWN, input->keybit);  
    set_bit(KEY_HOME, input->keybit);  
    set_bit(KEY_SEARCH, input->keybit);  
    
    input_set_abs_params(input, ABS_X, pdata->abs_x_min,
								pdata->abs_x_max, 0, 0);
    input_set_abs_params(input, ABS_Y, pdata->abs_y_min,
								pdata->abs_y_max, 0, 0);
    input_set_abs_params(input, ABS_HAT0X, pdata->abs_x_min,
								pdata->abs_x_max, 0, 0);
    input_set_abs_params(input, ABS_HAT0Y, pdata->abs_y_min,
								pdata->abs_y_max, 0, 0);

    //Added the MT protocol for Eclair by Stanley (2010/06/23)++
    input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0,
								255, 0, 0);
    input_set_abs_params(input, ABS_MT_POSITION_X, pdata->abs_x_min,
								pdata->abs_x_max, 0, 0);
    input_set_abs_params(input, ABS_MT_POSITION_Y, pdata->abs_y_min,
								pdata->abs_y_max, 0, 0);
	//Added the MT protocol for Eclair by Stanley (2010/06/23)--
    panjit->input = input;
    
    if (input_register_device(panjit->input)) 
    {
        panjit_msg(ERR, "can not register input device");
        goto err2;
    }

    if (misc_register(&panjit_misc_dev)) 
    {
        panjit_msg(ERR, "can not add misc device");
        goto err3;
    }

    if (MSM_GPIO_TO_INT(pdata->intr_gpio) != client->irq) 
    {
        panjit_msg(ERR, "irq not match");
        goto err4;
    }
    
    gpio_tlmm_config(GPIO_CFG(pdata->intr_gpio, 0, GPIO_INPUT,
    					GPIO_PULL_UP, GPIO_2MA), GPIO_ENABLE);

    gpio_tlmm_config(GPIO_CFG(sk_intr_gpio, 0, GPIO_INPUT,
    					GPIO_PULL_UP, GPIO_2MA), GPIO_ENABLE);
    
    panjit_msg(ERR, "[FM6]TP IRQ = GPIO %d, SK IRQ = GPIO %d",pdata->intr_gpio, sk_intr_gpio);

    if (panjit_get_fw_version(&iValue) >= 0)
    {
        panjit_msg(INFO, "[FM6]panjit firmware version = %d", iValue);
        msm_cap_touch_fw_version = iValue;  //Added for show FW version on FQC
    }
    else
    {
        panjit_msg(INFO, "[FM6]panjit firmware version failed!\r\n");
        return -EIO;
    }

    //Added by Stanley (2010.05.18)++ 
    if (panjit_get_sk_fw_version(&iValue) >= 0)
    {
        panjit_msg(INFO, "[FM6]panjit SK firmware version = %d", iValue);
        msm_cap_sk_fw_version = iValue;  //Added for show FW version on FQC
    }
    else
    {
        panjit_msg(INFO, "[FM6]panjit SK firmware version failed!\r\n");
        return -EIO;
    }
    //Added by Stanley (2010.05.18)--

    panjit_msg(INFO, "[FM6]panjit_probe init ok!\r\n");
    return 0;

err4:
	misc_deregister(&panjit_misc_dev);
err3:
	input_unregister_device(panjit->input);
err2:
	input_free_device(input);
err1:
	dev_set_drvdata(&client->dev, 0);
	kfree(panjit);
	return -1;
}



static const struct i2c_device_id panjit_id[] = {
    { TOUCH_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, panjit);

//Added for new kernel suspend scheme (2010.08.21)++
static struct dev_pm_ops panjit_pm_ops = 
{   .suspend = panjit_suspend,
    .resume  = panjit_resume,
};
//Added for new kernel suspend scheme (2010.08.21)--

static struct i2c_driver panjit_i2c_driver = {
	.driver = {
		.name	= TOUCH_NAME,
		.owner	= THIS_MODULE,
//Added for new kernel suspend scheme (2010.08.21)++
#ifdef CONFIG_PM
    .pm = &panjit_pm_ops,
#endif
//Added for new kernel suspend scheme (2010.08.21)--
	},
	.id_table   = panjit_id,
	.probe  	= panjit_probe,
	.remove 	= panjit_remove,
	//.suspend	= panjit_suspend,
    //.resume		= panjit_resume,
};

static int __init panjit_init( void )
{
    panjit_msg(INFO, "[FM6]panjit_init\r\n");
    
    msm_cap_touch_debug_mask = *(uint32_t *)TOUCH_DEBUG_MASK_OFFSET;  //Added for debug mask (2010/01/04)

    return i2c_add_driver(&panjit_i2c_driver);

}

static void __exit panjit_exit( void )
{
	i2c_del_driver(&panjit_i2c_driver);
}

module_init(panjit_init);
module_exit(panjit_exit);

MODULE_DESCRIPTION("PANJIT Touchscreen driver");
MODULE_AUTHOR("Eric Pan <erictcpan@tp.cmcs.com.tw>");
MODULE_VERSION("0:1.4");
MODULE_LICENSE("GPL");
