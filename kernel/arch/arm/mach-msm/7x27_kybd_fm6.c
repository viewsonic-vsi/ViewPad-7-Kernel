#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/io.h>
#include <linux/bootmem.h>

#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/delay.h>

#include <mach/gpio.h>
#include <mach/7x27_kybd.h>

#include <mach/msm_iomap.h>
#include <mach/msm_smd.h>

/* FIH, SimonSSChang, 2009/09/04 { */
/* [FXX_CR], change keypad suspend/resume function
to earlysuspend */
#include <linux/earlysuspend.h>
/* } FIH, SimonSSChang, 2009/09/04 */

#include "smd_rpcrouter.h"
//++++++++++++++++++++++++FIH_F0X_misty
#include "../../../kernel/power/power.h"
#include <linux/suspend.h>
#include "../../../include/linux/cmdbgapi.h"
static uint32_t Q7x27_kybd_debug_mask = 0;
bool b_EnableWakeKey = false;
bool b_EnableIncomingCallWakeKey = false;

bool b_VolUp_EnableWakeIrq = false;
bool b_VolDown_EnableWakeIrq = false;
bool b_HookKey_EnableWakeIrq = false;  //FIH, KarenLiao@20100304: F0X.B-9873: [Call control]Cannot end the call when long press hook key.
static bool SetupKeyFail=false;
void KeySetup(void);
//-----------------------FIH_F0X_misty
// FIH, WillChen, 2009/08/21 ++
#ifdef CONFIG_FIH_FXX_FORCEPANIC
#include <linux/proc_fs.h>
// FIH, WillChen, 2009/08/21 --

// FIH, WillChen, 2009/08/14 ++
//Press VolumeUp+VolumeDown+End key to force panic and dump log
bool VUP_Key = false;
bool VDN_Key = false;
static int flag = 0;
static DECLARE_WAIT_QUEUE_HEAD(wq);
// FIH, WillChen, 2009/08/14 --
#endif//#ifdef CONFIG_FIH_FXX_FORCEPANIC
#ifdef CONFIG_FIH_FXX
//+FIH_Misty
int g_HWID=0;
int g_ORIGHWID=0;
static int EnableKeyInt = 0; // 1:enable key interrupt
//-FIH_Misty
#endif
#define Q7x27_kybd_name "7x27_kybd"

#define VOLUME_KEY_ENABLE               1
#define SWITCH_KEY_ENABLE               1

#define HEADSET_DECTECT_SUPPORT         1

#define KEYPAD_DEBUG                    1

/* FIH, PeterKCTseng, @20090520 { */
/* The active type of input pin   */
#define ACTIVE_MODE_ENABLE              1

#define ACTIVE_LOW  0
#define ACTIVE_HIGH 1
/* } FIH, PeterKCTseng, @20090520 */
#define GPIO_NEG_Y 93

/* FIH, PeterKCTseng, @20090520 { */
/* The active type of input pin   */
#if ACTIVE_MODE_ENABLE // Peter, Debug
    struct  input_active_type   {
    	int volup_pin_actype;
	    int voldn_pin_actype;
	    int hook_sw_pin_actype;            
    };        
#endif
/* } FIH, PeterKCTseng, @20090520 */


struct Q7x27_kybd_record {
	struct	input_dev *Q7x27_kybd_idev;
	int volup_pin;
	int voldn_pin;
	int hook_sw_pin;

	uint8_t kybd_connected;
	struct	delayed_work kb_cmdq;


#if VOLUME_KEY_ENABLE // Peter, Debug
	struct	work_struct kybd_volkey1;
	struct	work_struct kybd_volkey2;    
#endif


#if SWITCH_KEY_ENABLE // Peter, Debug
	struct	work_struct hook_switchkey;
#endif


/* FIH, PeterKCTseng, @20090520 { */
/* The active type of input pin   */
#if ACTIVE_MODE_ENABLE // Peter, Debug
    struct  input_active_type   active;
#endif
/* } FIH, PeterKCTseng, @20090520 */

/* FIH, PeterKCTseng, @20090527 { */
/* phone jack dectect             */
#if HEADSET_DECTECT_SUPPORT
	int	bHookSWIRQEnabled;
#endif
/* } FIH, PeterKCTseng, @20090527 */

/* FIH, SimonSSChang, 2009/09/04 { */
/* [FXX_CR], change keypad suspend/resume function
to earlysuspend */
#ifdef CONFIG_FIH_FXX
#ifdef CONFIG_HAS_EARLYSUSPEND
    struct early_suspend Q7x27_kybd_early_suspend_desc;
    struct platform_device *pdev;
#endif
#endif
/* } FIH, SimonSSChang, 2009/09/04 */
};

struct input_dev *kpdev		= NULL;
struct Q7x27_kybd_record *rd	= NULL;

enum kbd_inevents {
	KBD_DEBOUNCE_TIME	= 1,//128,GRE.B-288[BT/keypad]During mp3 via bt headset, when pressing key,avoid affecting bt.
	KBD_IN_KEYPRESS		= 1,
	KBD_IN_KEYRELEASE	= 0,
};

#if KEYPAD_DEBUG    // debugging
bool    hookswitchflag = true;
#endif



#if SWITCH_KEY_ENABLE // Peter, Debug

#define KEY_HEADSETHOOK                 227 // KEY_F15
#define KEY_RINGSWITCH                  184 //FIH, KarenLiao, @200907xx: [F0X.FC-41]: The action of Inserting headset is the wake up action.

#endif


/* FIH, PeterKCTseng, @20090526 { */
/* power key support              */
struct input_dev *fih_msm_keypad_get_input_dev(void)
{	
    //printk(KERN_INFO "fih_msm_keypad_get_input_dev: Pressed POWER_KEY\n");
    fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"fih_msm_keypad_get_input_dev: Pressed POWER_KEY\n");
	return kpdev;
}
EXPORT_SYMBOL(fih_msm_keypad_get_input_dev);
/* } FIH, PeterKCTseng, @20090526 */

/* FIH, SimonSSChang, 2009/07/28 { */
/* [FXX_CR], F0X.FC-116 Add option for wake up source*/
#ifdef CONFIG_FIH_FXX
bool key_wakeup_get(void)
{
    //printk(KERN_INFO "Simon: key_wakeup_get() return %d\n", b_EnableWakeKey);
    fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"Simon: key_wakeup_get() return %d\n", b_EnableWakeKey);
    return b_EnableWakeKey;
}
EXPORT_SYMBOL(key_wakeup_get);

int key_wakeup_set(int on)
{
    if(on)
    {
        b_EnableWakeKey = true;
        //printk(KERN_INFO "Simon: key_wakeup_set() %d\n", b_EnableWakeKey);
        fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"Simon: key_wakeup_set() %d\n", b_EnableWakeKey);
        return 0;
    }
    else
    {
        b_EnableWakeKey = false;
        //printk(KERN_INFO "Simon: key_wakeup_set() %d\n", b_EnableWakeKey);        
        fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"Simon: key_wakeup_set() %d\n", b_EnableWakeKey);
        return 0;
    }
}    
EXPORT_SYMBOL(key_wakeup_set);
#endif
/* } FIH, SimonSSChang, 2009/07/28 */

/* FIH, SimonSSChang, 2009/09/10 { */
/* [FXX_CR], To enable Send & End key wakeup when incoming call*/
#ifdef CONFIG_FIH_FXX
bool incoming_call_get(void)
{
    //printk(KERN_INFO "Simon: incoming_call_get() return %d\n", b_EnableIncomingCallWakeKey);
    fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"Simon: incoming_call_get() return %d\n", b_EnableIncomingCallWakeKey);
    return b_EnableIncomingCallWakeKey;
}
EXPORT_SYMBOL(incoming_call_get);

int incoming_call_set(int on)
{
    if(on)
    {
        b_EnableIncomingCallWakeKey = true;
        //printk(KERN_INFO "Simon: incoming_call_set() %d\n", b_EnableIncomingCallWakeKey);
        fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"Simon: incoming_call_set() %d\n", b_EnableIncomingCallWakeKey);
        return 0;
    }
    else
    {
        b_EnableIncomingCallWakeKey = false;
        //printk(KERN_INFO "Simon: incoming_call_set() %d\n", b_EnableIncomingCallWakeKey);        
        fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"Simon: incoming_call_set() %d\n", b_EnableIncomingCallWakeKey);
        return 0;
    }
}    
EXPORT_SYMBOL(incoming_call_set);
#endif/* } FIH, SimonSSChang, 2009/09/10 */

static irqreturn_t Q7x27_kybd_irqhandler(int irq, void *dev_id)
{
	struct Q7x27_kybd_record *kbdrec = dev_id;

    //printk(KERN_INFO "irqreturn_t Q7x27_kybd_irqhandler+, irq= %X \n", irq);
    fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"irqreturn_t Q7x27_kybd_irqhandler+, irq= %X\n", irq);

    if (kbdrec->kybd_connected) {

#if VOLUME_KEY_ENABLE // Peter, Debug
	 if (MSM_GPIO_TO_INT(kbdrec->volup_pin) == irq){
			schedule_work(&kbdrec->kybd_volkey1);
        } else if (MSM_GPIO_TO_INT(kbdrec->voldn_pin) == irq) {
			schedule_work(&kbdrec->kybd_volkey2);            
		}
#endif

#if SWITCH_KEY_ENABLE // Peter, Debug
		else if (MSM_GPIO_TO_INT(kbdrec->hook_sw_pin) == irq){
			schedule_work(&kbdrec->hook_switchkey);
        }
#endif


	}

	
	return IRQ_HANDLED;
}

static int Q7x27_kybd_irqsetup(struct Q7x27_kybd_record *kbdrec)
{
	int rc;
				
#if VOLUME_KEY_ENABLE // Peter, Debug
	/* Vol UP and Vol DOWN keys interrupt */
	rc = request_irq(MSM_GPIO_TO_INT(kbdrec->volup_pin), &Q7x27_kybd_irqhandler,
			     (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING), 
			     Q7x27_kybd_name, kbdrec);
	if (rc < 0) {
		//printk(KERN_ERR
		//       "Could not register for  %s interrupt "
		//       "(rc = %d)\n", Q7x27_kybd_name, rc);
		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"Could not register for  %s interrupt(rc = %d)\n",Q7x27_kybd_name, rc);
		rc = -EIO;
	}
	rc = request_irq(MSM_GPIO_TO_INT(kbdrec->voldn_pin), &Q7x27_kybd_irqhandler,
			     (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING), 
			     Q7x27_kybd_name, kbdrec);
	if (rc < 0) {
		//printk(KERN_ERR
		//       "Could not register for  %s interrupt "
		//       "(rc = %d)\n", Q7x27_kybd_name, rc);
		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"Could not register for  %s interrupt(rc = %d)\n",Q7x27_kybd_name, rc);
		rc = -EIO;
	}
#endif
	
	
	return rc;
}


/* FIH, PeterKCTseng, @20090527 { */
/* phone jack dectect             */
#if HEADSET_DECTECT_SUPPORT
int Q7x27_kybd_hookswitch_irqsetup(bool activate_irq)
{
	int rc = 0;
    bool  hook_sw_val;

	suspend_state_t SuspendState = PM_SUSPEND_ON;//FIH, KarenLiao, @20090731: [F0X.FC-41]: The action of Inserting headset is the wake up action.

	
    //printk(KERN_INFO "Q7x27_kybd_hookswitch_irqsetup \n"); 
    fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"Q7x27_kybd_hookswitch_irqsetup\n");

//+++ FIH, KarenLiao, @20090731: [F0X.FC-41]: The action of Inserting headset is the wake up action.
	SuspendState = get_suspend_state();
	if(SuspendState == PM_SUSPEND_MEM)//3
	{
		if(kpdev)
		{
			input_report_key(kpdev, KEY_RINGSWITCH, KBD_IN_KEYPRESS);
			//printk(KERN_INFO "FIH: keypress KEY_RINGSWITCH = %d\n", KEY_LEFTSHIFT);
			fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"FIH: keypress KEY_RINGSWITCH = %d\n",KEY_LEFTSHIFT);
			
			input_report_key(kpdev, KEY_RINGSWITCH, KBD_IN_KEYRELEASE);
			//printk(KERN_INFO "FIH: keyrelease KEY_RINGSWITCH = %d\n", KEY_LEFTSHIFT);
			fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"FIH: keyrelease KEY_RINGSWITCH = %d\n",KEY_LEFTSHIFT);
			input_sync(kpdev);
		}
	}
//--- FIH, KarenLiao, @20090731: [F0X.FC-41]: The action of Inserting headset is the wake up action.

	if (activate_irq && (gpio_get_value(rd->hook_sw_pin) == 1)) {

/* FIH, PeterKCTseng, @20090603 { */
/* clear pending interrupt        */
        gpio_clear_detect_status(rd->hook_sw_pin);
   	    hook_sw_val = (bool)gpio_get_value(rd->hook_sw_pin);
  	    //printk(KERN_INFO "Read back hook switch eky <%d>\n", hook_sw_val);
  	    fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"Read back hook switch key <%d>\n", hook_sw_val);
        mdelay(250);
/* } FIH, PeterKCTseng, @20090603 */

    	rc = request_irq(MSM_GPIO_TO_INT(rd->hook_sw_pin), &Q7x27_kybd_irqhandler,
			        (IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING), 
			        Q7x27_kybd_name, rd);
	    if (rc < 0) {
    		//printk(KERN_ERR
		    //    "Could not register for  %s interrupt "
		    //    "(rc = %d)\n", Q7x27_kybd_name, rc);
		    fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"Could not register for  %s interrupt(rc = %d) \n",Q7x27_kybd_name, rc);
		    rc = -EIO;
	    }
        fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"Hook Switch IRQ Enable!\n");
        //printk(KERN_INFO "Hook Switch IRQ Enable! \n");
		rd->bHookSWIRQEnabled = true;
	} else {
		if (rd->bHookSWIRQEnabled)  {
			//printk(KERN_INFO "Free IRQ\n");
    		free_irq(MSM_GPIO_TO_INT(rd->hook_sw_pin), rd);
             fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"Hook Switch IRQ disable\n");
            //printk(KERN_INFO "Hook Switch IRQ disable! \n");
			rd->bHookSWIRQEnabled = false;
		}
	}
	
	return rc;

}
#endif
//EXPORT_SYMBOL(Q7x27_kybd_hookswitch_irqsetup);
/* } FIH, PeterKCTseng, @20090527 */

static int Q7x27_kybd_release_gpio(struct Q7x27_kybd_record *kbrec)
{

#if VOLUME_KEY_ENABLE // Peter, Debug
	int kbd_volup_pin	= kbrec->volup_pin;
	int kbd_voldn_pin	= kbrec->voldn_pin;
#endif


#if SWITCH_KEY_ENABLE // Peter, Debug
	int kbd_hook_sw_pin	= kbrec->hook_sw_pin;
#endif


#if VOLUME_KEY_ENABLE // Peter, Debug
	gpio_free(kbd_volup_pin);
	gpio_free(kbd_voldn_pin);
#endif

#if SWITCH_KEY_ENABLE // Peter, Debug
	gpio_free(kbd_hook_sw_pin);
#endif

	return 0;
}

static int Q7x27_kybd_config_gpio(struct Q7x27_kybd_record *kbrec)
{
	int kbd_volup_pin	= kbrec->volup_pin;
	int kbd_voldn_pin	= kbrec->voldn_pin;
	int kbd_hook_sw_pin	= kbrec->hook_sw_pin;

	int rc;


#if VOLUME_KEY_ENABLE // Peter, Debug
	rc = gpio_request(kbd_volup_pin, "gpio_keybd_volup");
	if (rc) {
		//printk(KERN_ERR "gpio_request failed on pin %d (rc=%d)\n",
		//	kbd_volup_pin, rc);
		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"gpio_request failed on pin %d (rc=%d)\n",kbd_volup_pin, rc);
		goto err_gpioconfig;
	}
	rc = gpio_request(kbd_voldn_pin, "gpio_keybd_voldn");
	if (rc) {
		//printk(KERN_ERR "gpio_request failed on pin %d (rc=%d)\n",
		//	kbd_voldn_pin, rc);
		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"gpio_request failed on pin %d (rc=%d)\n",kbd_voldn_pin, rc);
		goto err_gpioconfig;
	}
#endif


#if SWITCH_KEY_ENABLE // Peter, Debug
	rc = gpio_request(kbd_hook_sw_pin, "gpio_hook_sw");
	if (rc) {
		//printk(KERN_ERR "gpio_request failed on pin %d (rc=%d)\n",
		//	kbd_hook_sw_pin, rc);
		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"gpio_request failed on pin %d (rc=%d)\n",kbd_hook_sw_pin, rc);
		goto err_gpioconfig;
	}
#endif

#if VOLUME_KEY_ENABLE // Peter, Debug
	rc = gpio_direction_input(kbd_volup_pin);
	if (rc) {
		//printk(KERN_ERR "gpio_direction_input failed on "
		//       "pin %d (rc=%d)\n", kbd_volup_pin, rc);
		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"gpio_direction_input failed on pin %d (rc=%d)\n",kbd_volup_pin, rc);
		goto err_gpioconfig;
	}
	rc = gpio_direction_input(kbd_voldn_pin);
	if (rc) {
		//printk(KERN_ERR "gpio_direction_input failed on "
		//       "pin %d (rc=%d)\n", kbd_voldn_pin, rc);
		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"gpio_direction_input failed on pin %d (rc=%d)\n",kbd_voldn_pin, rc);
		goto err_gpioconfig;
	}
#endif


#if SWITCH_KEY_ENABLE // Peter, Debug
	rc = gpio_direction_input(kbd_hook_sw_pin);
	if (rc) {
		//printk(KERN_ERR "gpio_direction_input failed on "
		//       "pin %d (rc=%d)\n", kbd_hook_sw_pin, rc);
		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"gpio_direction_input failed on pin %d (rc=%d)\n",kbd_hook_sw_pin, rc);
		goto err_gpioconfig;
	}
#endif
    //printk(KERN_INFO "FIH: enter 5\n");
    return rc;
    
err_gpioconfig:
    //printk(KERN_INFO "FIH: enter 6\n");    
	Q7x27_kybd_release_gpio(kbrec);
	return rc;
}


#if VOLUME_KEY_ENABLE // Peter, Debug
static void Q7x27_kybd_volkey1(struct work_struct *work)
{
	struct Q7x27_kybd_record *kbdrec= container_of(work, struct Q7x27_kybd_record, kybd_volkey1);
	struct input_dev *idev		= kbdrec->Q7x27_kybd_idev;
	bool debounceDelay		= false;
	
	bool volup_val			= (bool)gpio_get_value(kbdrec->volup_pin);
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++misty
	suspend_state_t SuspendState = PM_SUSPEND_ON;//0
	//-----------------------------------------------------------------misty
/* FIH, PeterKCTseng, @20090520 { */
/* The active type of input pin   */
#if ACTIVE_MODE_ENABLE // Peter, Debug
	bool state;
#endif
/* } FIH, PeterKCTseng, @20090520 */
     fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"[Misty]VOL UP <%d>\n", volup_val);
	//printk(KERN_INFO "VOL UP <%d>\n", volup_val);

	disable_irq(MSM_GPIO_TO_INT(kbdrec->volup_pin));
//+++++++++++++++++++++++++++++++FIH_F0X_misty
    if(EnableKeyInt)
    {
        SuspendState = get_suspend_state();
        if(SuspendState == PM_SUSPEND_MEM)
        {
            if(idev)
            {
            	input_report_key(idev, KEY_VOLUMEUP, KBD_IN_KEYPRESS);
            		//printk(KERN_INFO "FIH: keypress KEY_VOLUMEUP\n");
            		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"FIH: keypress KEY_VOLUMEUP\n");
            	input_report_key(idev, KEY_VOLUMEUP, KBD_IN_KEYRELEASE);
            		//printk(KERN_INFO "FIH: keyrelease KEY_VOLUMEUP\n");
            		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"FIH: keyrelease KEY_VOLUMEUP\n");
            	input_sync(idev);
            }
        }
        //-------------------------------FIH_F0X_misty
        else
        {
        /* FIH, PeterKCTseng, @20090520 { */
        /* The active type of input pin   */
        #if ACTIVE_MODE_ENABLE // Peter, Debug
            state = (kbdrec->active.volup_pin_actype == ACTIVE_HIGH) ? volup_val : !volup_val;
        	//printk(KERN_INFO "active type= %d \n", state);
        #endif
        /* } FIH, PeterKCTseng, @20090520 */
            if(idev)
            {
            	if (state) {
            		input_report_key(idev, KEY_VOLUMEUP, KBD_IN_KEYPRESS);
            		//printk(KERN_INFO "FIH: keypress KEY_VOLUMEUP\n");
            		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"FIH: keypress KEY_VOLUMEUP\n");
        // FIH, WillChen, 2009/08/14 ++
        //Press VolumeUp+VolumeDown+End key to force panic and dump log
        #ifdef CONFIG_FIH_FXX_FORCEPANIC
            		VUP_Key = true;
            		if (VUP_Key && VDN_Key && END_key)
            		{
            			printk(KERN_ERR "KPD: Three key panic!!\n");
            			flag = 1;
        				wake_up(&wq);
        				msleep(5000);
        				panic("Three key panic");
        			}
        #endif
        // FIH, WillChen, 2009/08/14 --
            	} else {
            		input_report_key(idev, KEY_VOLUMEUP, KBD_IN_KEYRELEASE);
            		//printk(KERN_INFO "FIH: keyrelease KEY_VOLUMEUP\n");		
            		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"FIH: keyrelease KEY_VOLUMEUP\n");
            		debounceDelay = true;
        // FIH, WillChen, 2009/08/14 ++
        //Press VolumeUp+VolumeDown+End key to force panic and dump log
        #ifdef CONFIG_FIH_FXX_FORCEPANIC
            		VUP_Key = false;
        #endif
        // FIH, WillChen, 2009/08/14 --
            	}
            	
            	input_sync(idev);
            }
        	
        	if (debounceDelay) {
        		mdelay(KBD_DEBOUNCE_TIME); //Debounce
        	}
           }
    }//if(EnableKeyInt)
    	
	enable_irq(MSM_GPIO_TO_INT(kbdrec->volup_pin));
}

static void Q7x27_kybd_volkey2(struct work_struct *work)
{
	struct Q7x27_kybd_record *kbdrec= container_of(work, struct Q7x27_kybd_record, kybd_volkey2);
	struct input_dev *idev		= kbdrec->Q7x27_kybd_idev;
	bool debounceDelay		= false;
	
	bool voldn_val			= (bool)gpio_get_value(kbdrec->voldn_pin);
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++misty
	suspend_state_t SuspendState = PM_SUSPEND_ON;//0
	//-----------------------------------------------------------------misty
/* FIH, PeterKCTseng, @20090520 { */
/* The active type of input pin   */
#if ACTIVE_MODE_ENABLE // Peter, Debug
	bool state;
#endif
/* } FIH, PeterKCTseng, @20090520 */
    //fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"[Misty]VOL DN <%d>\n", voldn_val);
	//printk(KERN_INFO "VOL DN <%d>\n", voldn_val);

	disable_irq(MSM_GPIO_TO_INT(kbdrec->voldn_pin));
//+++++++++++++++++++++++++++++++FIH_F0X_misty
    if(EnableKeyInt)
    {
        SuspendState = get_suspend_state();
        if(SuspendState == PM_SUSPEND_MEM)
        {
            if(idev)
            {
            	input_report_key(idev, KEY_VOLUMEDOWN, KBD_IN_KEYPRESS);
            		//printk(KERN_INFO "FIH: keypress KEY_VOLUMEDOWN\n");
            		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"FIH: keypress KEY_VOLUMEDOWNP\n");
            	input_report_key(idev, KEY_VOLUMEDOWN, KBD_IN_KEYRELEASE);
            		//printk(KERN_INFO "FIH: keyrelease KEY_VOLUMEDOWN\n");
            		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"FIH: keyrelease KEY_VOLUMEDOWN\n");
            	input_sync(idev);
            }
        }
        //-------------------------------FIH_F0X_misty
        else
        {
        /* FIH, PeterKCTseng, @20090520 { */
        /* The active type of input pin   */
        #if ACTIVE_MODE_ENABLE // Peter, Debug
            state = (kbdrec->active.voldn_pin_actype == ACTIVE_HIGH) ? voldn_val : !voldn_val;
        	//printk(KERN_INFO "active type= %d \n", state);
        #endif
        /* } FIH, PeterKCTseng, @20090520 */
            if(idev)
            {
            	if (state) {
            		input_report_key(idev, KEY_VOLUMEDOWN, KBD_IN_KEYPRESS);
            		//printk(KERN_INFO "FIH: keypress KEY_VOLUMEDOWN\n");
            		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"FIH: keypress KEY_VOLUMEDOWN\n");
        // FIH, WillChen, 2009/08/14 ++
        //Press VolumeUp+VolumeDown+End key to force panic and dump log
        #ifdef CONFIG_FIH_FXX_FORCEPANIC
            		VDN_Key = true;
            		if (VUP_Key && VDN_Key && END_key)
            		{
            			printk(KERN_ERR "KPD: Three key panic!!\n");
            			flag = 1;
        				wake_up(&wq);
        				msleep(5000);
        				panic("Three key panic");
        			}
        #endif
        // FIH, WillChen, 2009/08/14 --
            	} else {
            		input_report_key(idev, KEY_VOLUMEDOWN, KBD_IN_KEYRELEASE);
            		//printk(KERN_INFO "FIH: keyrelease KEY_VOLUMEDOWN\n");		
            		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"FIH: keyrelease KEY_VOLUMEDOWN\n");
            		debounceDelay = true;
        // FIH, WillChen, 2009/08/14 ++
        //Press VolumeUp+VolumeDown+End key to force panic and dump log
        #ifdef CONFIG_FIH_FXX_FORCEPANIC
            		VDN_Key = false;
        #endif
        // FIH, WillChen, 2009/08/14 --
            	}
            
            	input_sync(idev);
            			//printk(KERN_INFO "FIH: keypress KEY_VOLUMEDOWN\n");	
            			fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"FIH: keypress KEY_VOLUMEDOWN\n");
            }	
        
        	if (debounceDelay) {
        		mdelay(KBD_DEBOUNCE_TIME); //Debounce
        	}
         }
    }//if(EnableKeyInt)
	enable_irq(MSM_GPIO_TO_INT(kbdrec->voldn_pin));
}
#endif


#if SWITCH_KEY_ENABLE // Peter, Debug
static void Q7x27_hook_switchkey(struct work_struct *work)
{
	struct Q7x27_kybd_record *kbdrec= container_of(work, struct Q7x27_kybd_record, hook_switchkey);
	struct input_dev *idev		= kbdrec->Q7x27_kybd_idev;
	bool debounceDelay		= false;
	
	bool hook_sw_val			= (bool)gpio_get_value(kbdrec->hook_sw_pin);
	
/* FIH, PeterKCTseng, @20090520 { */
/* The active type of input pin   */
#if ACTIVE_MODE_ENABLE // Peter, Debug
	bool state;
#endif
/* } FIH, PeterKCTseng, @20090520 */

	if(rd->bHookSWIRQEnabled == true){  //FA3.FC-282: report key only when bHookSWIRQEnabled is true.
	
		//printk(KERN_INFO "HOOK SW <%d>\n", hook_sw_val);	
		disable_irq(MSM_GPIO_TO_INT(kbdrec->hook_sw_pin));

	/* FIH, PeterKCTseng, @20090520 { */
	/* The active type of input pin   */
#if ACTIVE_MODE_ENABLE // Peter, Debug
	    state = (kbdrec->active.hook_sw_pin_actype == ACTIVE_HIGH) ? hook_sw_val : !hook_sw_val;
		//printk(KERN_INFO "active type= %d \n", state);
#endif
	/* } FIH, PeterKCTseng, @20090520 */
	    if(idev)
	    {
	    	if (state) {
	    		input_report_key(idev, KEY_HEADSETHOOK, KBD_IN_KEYPRESS); //report KEY_HEADSETHOOK pressing
	    		//printk(KERN_INFO "FIH: keypress KEY_HEADSETHOOK= %d\n", KEY_HEADSETHOOK);
	    		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"FIH: keypress KEY_HEADSETHOOK= %d\n", KEY_HEADSETHOOK);
	    	} else {
	    		input_report_key(idev, KEY_HEADSETHOOK, KBD_IN_KEYRELEASE); //report KEY_HEADSETHOOK releasing
	    		//printk(KERN_INFO "FIH: keyrelease KEY_HEADSETHOOK= %d\n", KEY_HEADSETHOOK);
	    		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"FIH: keyrelease KEY_HEADSETHOOK= %d\n", KEY_HEADSETHOOK);
	    		debounceDelay = true;
	    	}
	    
	    	input_sync(idev);
	    }
		
		if (debounceDelay) {
			mdelay(KBD_DEBOUNCE_TIME); //Debounce
		}
		
		enable_irq(MSM_GPIO_TO_INT(kbdrec->hook_sw_pin));
	}//FA3.FC-282: report key only when bHookSWIRQEnabled is true.//karen test
}
#endif




static void Q7x27_kybd_shutdown(struct Q7x27_kybd_record *rd)
{
	if (rd->kybd_connected) {
		printk(KERN_INFO "disconnecting keyboard\n");
		rd->kybd_connected = 0;


#if VOLUME_KEY_ENABLE // Peter, Debug
		free_irq(MSM_GPIO_TO_INT(rd->volup_pin), rd);
		free_irq(MSM_GPIO_TO_INT(rd->voldn_pin), rd);
#endif
		
#if SWITCH_KEY_ENABLE // Peter, Debug
		free_irq(MSM_GPIO_TO_INT(rd->hook_sw_pin), rd);
#endif


#if VOLUME_KEY_ENABLE // Peter, Debug
		flush_work(&rd->kybd_volkey1);
       	flush_work(&rd->kybd_volkey2);
#endif

#if SWITCH_KEY_ENABLE // Peter, Debug
		flush_work(&rd->hook_switchkey);
#endif

	}
}

static int Q7x27_kybd_opencb(struct input_dev *idev)
{
	struct Q7x27_kybd_record *kbdrec	= input_get_drvdata(idev);
		__set_bit(KEY_VOLUMEDOWN, idev->keybit);
		__set_bit(KEY_VOLUMEUP, idev->keybit);

	kbdrec->kybd_connected = 1;

	return 0;
}

static void Q7x27_kybd_closecb(struct input_dev *idev)
{

}

static struct input_dev *create_inputdev_instance(struct Q7x27_kybd_record *kbdrec)
{
	struct input_dev *idev	= NULL;

	idev = input_allocate_device();
	if (idev != NULL) {
		idev->name		= Q7x27_kybd_name;
		idev->open		= Q7x27_kybd_opencb;
		idev->close		= Q7x27_kybd_closecb;
		idev->keycode		= NULL;
		idev->keycodesize	= sizeof(uint8_t);
		idev->keycodemax	= 256;
/* FIH, PeterKCTseng, @20090603 { */
//		idev->evbit[0]		= BIT(EV_KEY);
        idev->evbit[0] = BIT(EV_KEY) | BIT(EV_REL);
        idev->relbit[0] = BIT(REL_X) | BIT(REL_Y);
/* } FIH, PeterKCTseng, @20090603 */

		/* a few more misc keys */

#if VOLUME_KEY_ENABLE // Peter, Debug
		__set_bit(KEY_VOLUMEDOWN, idev->keybit);
		__set_bit(KEY_VOLUMEUP, idev->keybit);
#endif


#if SWITCH_KEY_ENABLE // Peter, Debug
		__set_bit(KEY_HEADSETHOOK, idev->keybit);
		__set_bit(KEY_RINGSWITCH, idev->keybit); //FIH, KarenLiao, @20090731: [F0X.FC-41]: The action of Inserting headset is the wake up action.
#endif

		__set_bit(KEY_POWER, idev->keybit);

		input_set_drvdata(idev, kbdrec);
		kpdev = idev;
	} else {
		//printk(KERN_ERR "Failed to allocate input device for %s\n",
		//	Q7x27_kybd_name);
		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"Failed to allocate input device for %s\n",Q7x27_kybd_name);
	}
	
	return idev;
}

static void Q7x27_kybd_connect2inputsys(struct work_struct *work)
{
	struct Q7x27_kybd_record *kbdrec =
		container_of(work, struct Q7x27_kybd_record, kb_cmdq.work);	

	kbdrec->Q7x27_kybd_idev = create_inputdev_instance(kbdrec);
	if (kbdrec->Q7x27_kybd_idev) {
		if (input_register_device(kbdrec->Q7x27_kybd_idev) != 0) {
			//printk(KERN_ERR "Failed to register with"
			//	" input system\n");
			fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"Failed to register with input system\n");
			input_free_device(kbdrec->Q7x27_kybd_idev);
		}
	}
}

static int testfor_keybd(void)
{
	int rc = 0;

	INIT_DELAYED_WORK(&rd->kb_cmdq, Q7x27_kybd_connect2inputsys);
	schedule_delayed_work(&rd->kb_cmdq, msecs_to_jiffies(600));

	return rc;
}
/* FIH, SimonSSChang, 2009/09/04 { */
/* [FXX_CR], change keypad suspend/resume function
to earlysuspend */
#ifdef CONFIG_FIH_FXX
#ifdef CONFIG_HAS_EARLYSUSPEND
void Q7x27_kybd_early_suspend(struct early_suspend *h)
{
    fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"Q7x27_kybd_early_suspend()(%d)\n",rd->kybd_connected);
 //printk(KERN_INFO "Q7x27_kybd_early_suspend()(%d)\n",rd->kybd_connected);
 if(SetupKeyFail)
 {
     SetupKeyFail=false;
     KeySetup();
    
 }

    if(b_EnableIncomingCallWakeKey)
    {
      if(device_may_wakeup(&rd->pdev->dev)) 
      {
                
        //printk(KERN_INFO "enable VolUp   wakeup pin: %d\n", rd->volup_pin);
        fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"enable VolUp wakeup pin: %d\n", rd->volup_pin);      
        enable_irq_wake(MSM_GPIO_TO_INT(rd->volup_pin));
        b_VolUp_EnableWakeIrq = true;

        //printk(KERN_INFO "enable VolDown   wakeup pin: %d\n", rd->voldn_pin); 
        fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"enable VolDown wakeup pin: %d\n", rd->voldn_pin);      
        enable_irq_wake(MSM_GPIO_TO_INT(rd->voldn_pin));
        b_VolDown_EnableWakeIrq = true;
      }
    }

 //+++ FIH, KarenLiao@20100304: F0X.B-9873: [Call control]Cannot end the call when long press hook key.
 if((b_EnableIncomingCallWakeKey ==true) && (rd->bHookSWIRQEnabled == true) && device_may_wakeup(&rd->pdev->dev) )
 {
     printk(KERN_INFO "enable Hook Key   wakeup pin: %d\n", rd->hook_sw_pin);
     fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"enable Hook Key   wakeup pin: %d\n",rd->hook_sw_pin);
     enable_irq_wake(MSM_GPIO_TO_INT(rd->hook_sw_pin));
     b_HookKey_EnableWakeIrq = true;
 }
 //--- FIH, KarenLiao@20100304: F0X.B-9873: [Call control]Cannot end the call when long press hook key.
 
}
void Q7x27_kybd_late_resume(struct early_suspend *h)
{
    fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"Q7x27_kybd_late_resume()(%d)\n",rd->kybd_connected);
 //printk(KERN_INFO "Q7x27_kybd_late_resume()(%d)\n",rd->kybd_connected);
// printk(KERN_ERR "%s""#######################g_center_pin:%d \n", __func__,g_center_pin);
  if(SetupKeyFail)
 {
     SetupKeyFail=false;
     KeySetup();
 }
 if (device_may_wakeup(&rd->pdev->dev))
 {   

   if(b_VolUp_EnableWakeIrq)
   {
     //printk(KERN_INFO "disable VolUp   wakeup pin: %d\n", rd->volup_pin);
     fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"disable VolUp   wakeup pin: %d\n", rd->volup_pin);
	 disable_irq_wake(MSM_GPIO_TO_INT(rd->volup_pin));
     b_VolUp_EnableWakeIrq = false;     
   }

   if(b_VolDown_EnableWakeIrq)
   {
     //printk(KERN_INFO "disable VolDown   wakeup pin: %d\n", rd->voldn_pin);
     fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"disable VolDown   wakeup pin: %d\n", rd->voldn_pin);
	 disable_irq_wake(MSM_GPIO_TO_INT(rd->voldn_pin));
     b_VolDown_EnableWakeIrq = false;     
   }
   //+++ FIH, KarenLiao@20100304: F0X.B-9873: [Call control]Cannot end the call when long press hook key.
   if(b_HookKey_EnableWakeIrq == true)
   {
     printk(KERN_INFO "disable HookKey   wakeup pin: %d\n", rd->hook_sw_pin);
     fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"disable HookKey   wakeup pin: %d\n", rd->hook_sw_pin);
	 disable_irq_wake(MSM_GPIO_TO_INT(rd->hook_sw_pin));
     b_HookKey_EnableWakeIrq = false;
   }
   //--- FIH, KarenLiao@20100304: F0X.B-9873: [Call control]Cannot end the call when long press hook key.
 }
 
}
#endif
#endif
/* } FIH, SimonSSChang, 2009/09/04 */

static int Q7x27_kybd_remove(struct platform_device *pdev)
{
	//printk(KERN_INFO "removing keyboard driver\n");
	fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"removing keyboard driver\n");

/* FIH, SimonSSChang, 2009/09/04 { */
/* [FXX_CR], change keypad suspend/resume function
to earlysuspend */
#ifdef CONFIG_FIH_FXX
#ifdef CONFIG_HAS_EARLYSUSPEND
    //printk(KERN_INFO "Keypad unregister_early_suspend()\n");
    fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"Keypad unregister_early_suspend()\n");
	unregister_early_suspend(&rd->Q7x27_kybd_early_suspend_desc);
#endif
#endif
/* } FIH, SimonSSChang, 2009/09/04 */

	if (rd->Q7x27_kybd_idev) {
		//printk(KERN_INFO "deregister from input system\n");
		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"deregister from input system\n");
		input_unregister_device(rd->Q7x27_kybd_idev);
		rd->Q7x27_kybd_idev = NULL;
	}
	Q7x27_kybd_shutdown(rd);
	Q7x27_kybd_release_gpio(rd);

	kfree(rd);

	return 0;
}

//++++++++++++++++++++++++++++FIH_F0X_misty
/* FIH, SimonSSChang, 2009/09/04 { */
/* [FXX_CR], change keypad suspend/resume function
to earlysuspend*/
//#ifdef CONFIG_PM
#ifdef CONFIG_PM__
/* } FIH, SimonSSChang, 2009/09/04 */
static int Q7x27_kybd_suspend(struct platform_device *pdev, pm_message_t state)
{
 struct Q7x27_kybd_platform_data *kbdrec;
 kbdrec       = pdev->dev.platform_data;
 
 printk(KERN_ERR "%s""@@@@@@@@@@@@@@@@@@@@@@@@g_center_pin:%d \n", __func__,g_center_pin);
 return 0;
}
static int Q7x27_kybd_resume(struct platform_device *pdev)
{
 struct Q7x27_kybd_platform_data *kbdrec;
 kbdrec       = pdev->dev.platform_data;
 
 printk(KERN_ERR "%s""#######################g_center_pin:%d \n", __func__,g_center_pin);
 
 return 0;
}
#else
# define Q7x27_kybd_suspend NULL
# define Q7x27_kybd_resume  NULL
#endif
//---------------------------FIH_F0X_misty
static int Q7x27_kybd_probe(struct platform_device *pdev)
{
	struct Q7x27_kybd_platform_data *setup_data;
	int rc = -ENOMEM;

/* FIH, PeterKCTseng, @20090520 { */
/* The active type of input pin   */
#if ACTIVE_MODE_ENABLE // Peter, Debug
    g_HWID = FIH_READ_HWID_FROM_SMEM();
    g_ORIGHWID = FIH_READ_ORIG_HWID_FROM_SMEM();
#endif
/* } FIH, PeterKCTseng, @20090520 */

    //FIH_debug_log
    fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"Q7x27_kybd_probe\n");
    //printk(KERN_INFO "FIH: enter Q7x27_kybd_probe()\n");
	rd = kzalloc(sizeof(struct Q7x27_kybd_record), GFP_KERNEL);
	if (!rd) {
		//printk(KERN_ERR "i2ckybd_record memory allocation failed!!\n");
		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"kybd_record memory allocation failed!!\n");
		return rc;
	}

	setup_data 		    = pdev->dev.platform_data;
	//+++++++++++++++++++++++++++FIH_F0X_misty
	//printk(KERN_ERR "++++++++++++++++++++++++add init_wakeup\n");
	device_init_wakeup(&pdev->dev, 1);
	//printk(KERN_ERR "---------------------add init_wakeup\n");
	//------------------------------FIH_F0X_misty
/* FIH, PeterKCTseng, @20090520 { */
/* The active type of input pin   */
#if ACTIVE_MODE_ENABLE // Peter, Debug
    
     if (g_HWID >= CMCS_RTP_PR1) {
		//printk(KERN_ERR "FIH: CMCS_RTP_PR1, g_HWID= %d \n", g_HWID);
		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"FIH: CMCS_RTP_PR1, g_HWID= %d\n", g_HWID);
        rd->active.volup_pin_actype     = ACTIVE_HIGH;
        rd->active.voldn_pin_actype     = ACTIVE_HIGH;
        rd->active.hook_sw_pin_actype   = ACTIVE_LOW;

    }          
    else {
        // target board undefined 
		//printk(KERN_ERR "target borad can not be recognized!! \n");
		fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"target borad can not be recognized!!\n");

    }  
#endif
/* } FIH, PeterKCTseng, @20090520 */

#if VOLUME_KEY_ENABLE // Peter, Debug
	rd->volup_pin		= setup_data->volup_pin;
	rd->voldn_pin		= setup_data->voldn_pin;
#endif


#if SWITCH_KEY_ENABLE // Peter, Debug
	rd->hook_sw_pin     = setup_data->hook_sw_pin;

/* FIH, PeterKCTseng, @20090603 { */
	rd->bHookSWIRQEnabled = false; //FIH, KarenLiao, @20100520: FM6.B-706: Set initial value of HookSWIRQEnabled to false to fix calling enable_irq_wake() without headset.
/* } FIH, PeterKCTseng, @20090603 */
#endif


	//Initialize GPIO
	rc = Q7x27_kybd_config_gpio(rd);
	if (rc)
		goto failexit1;


#if VOLUME_KEY_ENABLE // Peter, Debug
	INIT_WORK(&rd->kybd_volkey1, Q7x27_kybd_volkey1);
    INIT_WORK(&rd->kybd_volkey2, Q7x27_kybd_volkey2);
#endif


#if SWITCH_KEY_ENABLE // Peter, Debug
	INIT_WORK(&rd->hook_switchkey, Q7x27_hook_switchkey);


//  msm_mic_en_proc(true);

#endif

    KeySetup();

    /* FIH, Debbie, 2010/01/05 { */
    /* modify for key definition of OTA update*/
    if(fih_read_kpd_from_smem())
    {
    	   EnableKeyInt = 1;
    }
    /* FIH, Debbie, 2010/01/05 } */
#if 0//misty +++
    printk(KERN_INFO "FIH: enter 7\n");    
	rc = Q7x27_kybd_irqsetup(rd);
    printk(KERN_INFO "FIH: enter 8\n");        
	if (rc)
		goto failexit2;

	rc = testfor_keybd();
    printk(KERN_INFO "FIH: enter 9\n");    
	if (rc)
		goto failexit2;
	
	rd->kybd_connected = 1;
#endif//misty ---
/* FIH, SimonSSChang, 2009/09/04 { */
/* [FXX_CR], change keypad suspend/resume function
to earlysuspend */
#ifdef CONFIG_FIH_FXX
#ifdef CONFIG_HAS_EARLYSUSPEND
    rd->Q7x27_kybd_early_suspend_desc.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN - 10;
	rd->Q7x27_kybd_early_suspend_desc.suspend = Q7x27_kybd_early_suspend;
	rd->Q7x27_kybd_early_suspend_desc.resume = Q7x27_kybd_late_resume;
    //printk(KERN_INFO "Keypad register_early_suspend()\n");
    fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"Keypad register_early_suspend()\n");
	register_early_suspend(&rd->Q7x27_kybd_early_suspend_desc);
    rd->pdev = pdev;
#endif
#endif
/* } FIH, SimonSSChang, 2009/09/04 */

    //FIH_debug_log
    //printk(KERN_INFO "FIH: out Q7x27_kybd_probe()\n");
    return 0;
#if 0//misty+++
 failexit2:
    //FIH_debug_log#if CAMERA_KEY_ENABLE // Peter, Debug

    printk(KERN_INFO "FIH: error out failexit2\n");
    
	free_irq(MSM_GPIO_TO_INT(rd->key_1_pin), rd);
	free_irq(MSM_GPIO_TO_INT(rd->key_2_pin), rd);

#if VOLUME_KEY_ENABLE // Peter, Debug
	free_irq(MSM_GPIO_TO_INT(rd->volup_pin), rd);
	free_irq(MSM_GPIO_TO_INT(rd->voldn_pin), rd);
#endif

#if CAMERA_KEY_ENABLE // Peter, Debug
	free_irq(MSM_GPIO_TO_INT(rd->cam_sw_t_pin), rd);
	free_irq(MSM_GPIO_TO_INT(rd->cam_sw_f_pin), rd);
#endif

#if SWITCH_KEY_ENABLE // Peter, Debug
	free_irq(MSM_GPIO_TO_INT(rd->hook_sw_pin), rd);
#endif

/* FIH, PeterKCTseng, @20090527 { */
/* add center key                 */
#if CENTER_KEY_ENABLE // Peter, Debug
	free_irq(MSM_GPIO_TO_INT(rd->center_pin), rd);
#endif
/* } FIH, PeterKCTseng, @20090527 */

#endif//misty----
 failexit1:
    //FIH_debug_log
    //printk(KERN_INFO "FIH: error out failexit1\n");
    fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"FIH: error out failexit1\n");
	Q7x27_kybd_release_gpio(rd);
	kfree(rd);

    //FIH_debug_log
    //printk(KERN_INFO "FIH: error out Q7x27_kybd_probe()\n");
    return 0;
    
	return rc;
}
// FIH, WillChen, 2009/08/21 ++
//Dump cpu and mem info when three key panic
#ifdef CONFIG_FIH_FXX_FORCEPANIC
static int panic_read_proc(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{
	printk("device_panic_read_proc\n");
	wait_event(wq,flag != 0);

	return 0;
}
#endif
// FIH, WillChen, 2009/08/21 --
static struct platform_driver Q7x27_kybd_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name  = Q7x27_kybd_name,
	},
	.probe	  = Q7x27_kybd_probe,
	.remove	  = Q7x27_kybd_remove,
	.suspend  = Q7x27_kybd_suspend,
	.resume   = Q7x27_kybd_resume,
};

static int __init Q7x27_kybd_init(void)
{
    //++misty for debug mask
    Q7x27_kybd_debug_mask = *(uint32_t *)KPD_DEBUG_MASK_OFFSET;
    //--misty for debug mask
// FIH, WillChen, 2009/08/21 ++
//Dump cpu and mem info when three key panic
#ifdef CONFIG_FIH_FXX_FORCEPANIC
	create_proc_read_entry("panicdump", 0, NULL, panic_read_proc, NULL);
#endif
// FIH, WillChen, 2009/08/21 --
	return platform_driver_register(&Q7x27_kybd_driver);
}

static void __exit Q7x27_kybd_exit(void)
{
	platform_driver_unregister(&Q7x27_kybd_driver);
}
//+++FIH_misty enable keypad interrupt until boot complete
void KeySetup(void)
{
	 int rc = -ENOMEM;
     int count=0;
     int count1=0;
retry1:
        rc = Q7x27_kybd_irqsetup(rd);
        //printk(KERN_INFO "KeySetup/Q7x27_kybd_irqsetup\n");   
        fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"KeySetup/Q7x27_kybd_irqsetup\n");     
        if (rc)
        {
            goto retry1;
            count++;    
            if(count > 6)
            {
                //printk(KERN_INFO "retry FAIL======>Q7x27_kybd_irqsetup\n"); 
                fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"retry FAIL======>Q7x27_kybd_irqsetup\n");     
                count=0;
	            SetupKeyFail=true;
                goto failexit2;
            }
        }
retry2: 
        rc = testfor_keybd();
        //printk(KERN_INFO "KeySetup/testfor_keybd\n");  
         fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"KeySetup/testfor_keybd\n");  
        if (rc)
        {
            goto retry2;
            count1++;
            if(count1 > 6)
            {
                count1=0;
                //printk(KERN_INFO "retry FAIL======>testfor_keybd\n");  
                 fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"retry FAIL======>testfor_keybd\n");
		        SetupKeyFail=true;
                goto failexit2;
            }
        }
        rd->kybd_connected = 1;
	 return;
failexit2:

#if VOLUME_KEY_ENABLE // Peter, Debug
	free_irq(MSM_GPIO_TO_INT(rd->volup_pin), rd);
	free_irq(MSM_GPIO_TO_INT(rd->voldn_pin), rd);
#endif

#if SWITCH_KEY_ENABLE // Peter, Debug
	free_irq(MSM_GPIO_TO_INT(rd->hook_sw_pin), rd);
#endif

    return ;


}
static int Q7x27_kybd_param_set(const char *val, struct kernel_param *kp)
{
    int ret=1;
    if(!EnableKeyInt)
    {
        ret = param_set_bool(val, kp);
        //printk(KERN_ERR "%s: EnableKeyInt= %d\n", __func__, EnableKeyInt); 
        fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"%s: EnableKeyInt= %d\n", __func__, EnableKeyInt);
        
        if(ret)
        {
            //printk(KERN_ERR "%s param set bool failed (%d)\n",
            //			__func__, ret);    
            fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"%s param set bool failed (%d)\n",__func__, ret);
    	    EnableKeyInt = 1;
    	}
	/* FIH, Debbie, 2010/01/05 { */
	/* modify for key definition of OTA update*/
	else
	{
           if(fih_read_kpd_from_smem())
    	    {
    	        fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"enter recovery mode and set EnableKeyInt = 1\n");
    	        EnableKeyInt = 1;
    	    }
	}
	/* FIH, Debbie, 2010/01/05 } */
    	return 0;
    }
    else
    {
        //printk(KERN_ERR "has alreay set EnableKeyInt\n"); 
        fih_printk(Q7x27_kybd_debug_mask, FIH_DEBUG_ZONE_G0,"has alreay set EnableKeyInt\n"); 
        return 0;    
    }

}
module_param_call(EnableKeyIntrrupt, Q7x27_kybd_param_set, param_get_int,
		  &EnableKeyInt, S_IWUSR | S_IRUGO);
module_param_named(debug_mask, Q7x27_kybd_debug_mask, uint, 0644);
//---FIH_misty enable keypad interrupt until boot complete
module_init(Q7x27_kybd_init);
module_exit(Q7x27_kybd_exit);
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("Q7x27 keyboard driver");
MODULE_LICENSE("GPL v2");
