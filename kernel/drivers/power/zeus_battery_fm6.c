/* drivers/power/goldfish_battery.c
 *
 * Power supply driver for the goldfish emulator
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <mach/msm_iomap.h>
#include <mach/msm_smd.h>
#include <linux/i2c.h>
#include <mach/mpp.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/ioctl.h>
#include <asm/uaccess.h>
#include <asm/fcntl.h>

#include <linux/workqueue.h>  /* Modify to create a new work queue for BT play MP3 smoothly*/
#include "../../arch/arm/mach-msm/proc_comm.h"

#define T_FIH
#ifdef T_FIH	///+T_FIH

#include <asm/gpio.h>
#include <linux/wakelock.h>  /* Add wake lock to avoid incompleted update battery information*/
#include <linux/eventlog.h>   /* Add event log*/
#include<linux/cmdbgapi.h>  /* Add for debug mask*/

/*FIHTDC, add for FN6 HWID, MayLi, 2010.06.21 {*/
#include <mach/msm_smd.h>  
unsigned int g_orig_hwid;
/*FIHTDC, add for FN6 HWID, MayLi, 2010.06.21 }*/

typedef struct _TemperatureMappingData
{
  	int T;
	int Ohm;
}TemperatureMappingData;

static TemperatureMappingData MapTable[106]=
{
	{-30, 111364},	// 0
	{-29, 105833},
	{-28, 100607},
	{-27, 95668},
	{-26, 90997},
	{-25, 86580},
	{-24, 82401},
	{-23, 78447},
	{-22, 74703},
	{-21, 71159},
	
	{-20, 67801},		// 10
	{-19, 64620},
	{-18, 61606},
	{-17, 58748},
	{-16, 56038},
	{-15, 53468},
	{-14, 51029},
	{-13, 48715},
	{-12, 46518},
	{-11, 44432},

	{-10, 42450},		// 20
	{-9, 40567},
	{-8, 38778},
	{-7, 37077},
	{-6, 35460},
	{-5, 33922},
	{-4, 32459},
	{-3, 31067},
	{-2, 29741},
	{-1, 28480},

	{0, 27278},		// 30
	{1, 26133},
	{2, 25043},
	{3, 24004},
	{4, 23013},
	{5, 22068},
	{6, 21167},
	{7, 20308},
	{8, 19488},
	{9, 18705},

	{10, 17958},		// 40
	{11, 17244},
	{12, 16563},
	{13, 15912},
	{14, 15290},
	{15, 14695},
	{16, 14127},
	{17, 13583},
	{18, 13064},
	{19, 12566},

	{20, 12091},		// 50
	{21, 11635},
	{22, 11200},
	{23, 10782},
	{24, 10383},
	{25, 10000},
	{26, 9633},
	{27, 9282},
	{28, 8945},
	{29, 8622},

	{30, 8313},		// 60
	{31, 8016},
	{32, 7731},
	{33, 7458},
	{34, 7195},
	{35, 6944},
	{36, 6702},
	{37, 6470},
	{38, 6247},
	{39, 6033},

	{40, 5827},		// 70
	{41, 5630},
	{42, 5440},
	{43, 5257},
	{44, 5082},
	{45, 4913},
	{46, 4750},
	{47, 4594},
	{48, 4444},
	{49, 4299},

	{50, 4160},		// 80
	{51, 4026},
	{52, 3897},
	{53, 3773},
	{54, 3653},
	{55, 3537},
	{56, 3426},
	{57, 3319},
	{58, 3216},
	{59, 3116},
	
	{60, 3020},		// 90
	{61, 2928},
	{62, 2838},
	{63, 2752},
	{64, 2669},
	{65, 2589},
	{66, 2511},
	{67, 2437},
	{68, 2365},
	{69, 2295},
	
	{70, 2228},		// 100
	{71, 2163},
	{72, 2100},
	{73, 2039},
	{74, 1980},
	{75, 1924},
};


/*FIHTDC, add for FM6 PR3 temperature, MayLi, 2010.07.06 {*/
static TemperatureMappingData MapTablePR3[106]=
{
	{-30,	17455},	// 0
	{-29,	16425},
	{-28,	15461},
	{-27,	14559},
	{-26,	13715},
	{-25,	12925},
	{-24,	12184},
	{-23,	11490},
	{-22,	10840},
	{-21,	10230},
	
	{-20,	9658},	// 10
	{-19,	9121},
	{-18,	8617},
	{-17,	8144},
	{-16,	7699},
	{-15,	7281},
	{-14,	6888},
	{-13,	6518},
	{-12,	6170},
	{-11,	5843},
	
	{-10,	5535},	// 20
	{-9,		5245},
	{-8,		4971},
	{-7,		4714}, 
	{-6,		4474},
	{-5,		4242},
	{-4,		4026},
	{-3,		3822},
	{-2,		3629},
	{-1,		3448},
	
	{0,		3276},	// 30
	{1,		3114},
	{2,		2960},
	{3,		2815},
	{4,		2678},
	{5,		2549},
	{6,		2426},
	{7,		2310},
	{8,		2200},
	{9,		2096},
	
	{10,		1997},	// 40
	{11,		1903},
	{12,		1815},
	{13,		1731},
	{14,		1651},
	{15,		1575},
	{16,		1504},
	{17,		1435},
	{18,		1371},
	{19,		1309},	
	
	{20,		1251},	// 50
	{21,		1195},
	{22,		1143},
	{23,		1093},
	{24,		1045},
	{25,		1000},
	{26,		956},
	{27,		915},
	{28,		876},
	{29,		839},
	
	{30,		803},	// 60
	{31,		770},
	{32,		737},
	{33,		707},
	{34,		677},
	{35,		650},
	{36,		623},
	{37,		598},
	{38,		573},
	{39,		550},
	
	{40,		528},	// 70
	{41,		507},
	{42,		487},
	{43,		468},
	{44,		449},
	{45,		432},
	{46,		415},
	{47,		399},
	{48,		383},
	{49,		369},
	
	{50,		355},	// 80
	{51,		341},
	{52,		328},
	{53,		316},
	{54,		304},
	{55,		293},
	{56,		282},
	{57,		272},
	{58,		262},
	{59,		252},
	
	{60,		243},	// 90
	{61,		234},
	{62,		226},
	{63,		218},
	{64,		210},
	{65,		202},
	{66,		195},
	{67,		188},
	{68,		182},
	{69,		176},
	
	{70,		169},	// 100
	{71,		164},
	{72,		158},
	{73,		153},
	{74,		147},
	{75,		142},
};
/*FIHTDC, add for FM6 PR3 temperature, MayLi, 2010.07.06 }*/


//FIHTDC, Add for setting Low voltage POCV, MayLi, 2010.07.06 {+
typedef struct _VOLT_TO_PERCENT
{
    int Volt;
    int Percent;
} VOLT_TO_PERCENT;

static VOLT_TO_PERCENT g_Volt2PercentMode[9] =
{
    { 3281, 0},	   // empty,    Rx Table
    /* FIH, Michael Kao, 2010/01/11{ */
    { 3579, 6},    // level 1
    { 3674, 14},    // level 2
    { 3754, 29},    // level 3
    { 3784, 44},    // level 4
    { 3841, 59},    // level 5
    { 3898, 66},    // level 6
    { 4036, 85},    // level 7
    { 4190, 100},   // full
};
//FIHTDC, Add for setting Low voltage POCV, MayLi, 2010.07.06 -}


/*for FM6************************************************
*                                  no charge  100mA   500mA   1A    2A
*GPIO_CHR_IUSB			X		L		H	  X	  X
*GPIO_CHR_DCM			L		L		L     	  H     H
*GPIO_CHR_USBSUS		H		L		L	  L	  L
*GPIO_CHR_nCEN           	H		L		L	  L	  L
*GPIO_USB_2ACHG_SEL_n	L		L		L	  L	  H
**********************************************************/
#define GPIO_CHR_IUSB  123
#define GPIO_CHR_DCM   57
#define GPIO_CHR_USBSUS  33
#define GPIO_CHR_nCEN  26
#define GPIO_USB_2ACHG_SEL_n  20	

#define GPIO_CHR_DET 30		/* CHR_nDOK, USB or AC detect pin*/
#define GPIO_CHR_FLT 32		/* CHR_nFLT, charging timeout fault state*/
//#define GPIO_USBIN_OVP_N 58  /* USB-IN_OVP_N, it will pull low when USB in OVP*/ /*MayLi masks it, 2010.07.08*/

#define FLAG_BATTERY_POLLING
#define FLAG_CHARGER_DETECT
#endif	// T_FIH	///-T_FIH

//I2C
#define i2c_ds2786_name "ds2786"
#define I2C_SLAVE_ADDR	(0x6c >> 1)

struct i2c_client *bat_ds2786_i2c = NULL;
static struct proc_dir_entry *ds2786_proc_file = NULL;

int percentage_mode = 0;   /*RD uses for echo command */
int manual_percentage = 0;  /*RD uses for echo command */
int g_initVoltage = 0;
int g_OCVstatus = 1;  /*check whether gas gauge ocv table has filled*/
int g_LearnCapacityScalingFactor=-1;  /*FIHTDC, Add for monitoring it, MayLi, 2010.07.23*/

unsigned mpp_13=12;
unsigned mpp_15=14;
unsigned mpp_16=15;
unsigned mpp_4=3;   /*FIHTDC, Add for FN6 charging, MayLi, 2010.06.21*/

enum {
	CHARGER_STATE_UNKNOWN,		
	CHARGER_STATE_CHARGING,		
	CHARGER_STATE_DISCHARGING,	
	CHARGER_STATE_NOT_CHARGING,	
	CHARGER_STATE_FULL,
	CHARGER_STATE_LOW_POWER,
};

static int g_charging_fault=1;  /*FIHTDC, Adds for charging timeout detect, MayLi, 2010.05.12 */
//static int g_usb_ovp=1; /*FIHTDC, Adds for USB OVP detect, MayLi, 2010.05.20 */ /*MayLi masks it, 2010.07.08*/
static int g_charging_state = CHARGER_STATE_NOT_CHARGING;
static int g_charging_state_last = CHARGER_STATE_NOT_CHARGING;
struct workqueue_struct *zeus_batt_wq;  /* Modify to create a new work queue for BT play MP3 smoothly*/
static struct power_supply * g_ps_battery;  /* = data->battery power supply*/

struct goldfish_battery_data {
	uint32_t reg_base;
	int irq;
	spinlock_t lock;
	struct power_supply battery;	
	struct wake_lock battery_wakelock;  /* Add wake lock to avoid incompleted update battery information*/
};

static struct goldfish_battery_data *data;
bool wakelock_flag;

/* temporary variable used between goldfish_battery_probe() and goldfish_battery_open() */
static struct goldfish_battery_data *battery_data;  /*看起來沒用到，要再問問Michael*/


/* For device state, 看起來沒用到 ++*/
enum{
	battery_charger_type=0,
	wifi_state,
	GPS_state,
	phone_state,
};

struct F9_device_state{
	int F9_battery_charger_type;
	int F9_wifi_state;
	int F9_GPS_state;
	int F9_phone_state;
};
static struct F9_device_state batt_state;
/* For device state, 看起來沒用到 --*/


/* FIHTDC, MayLi, add for recording battery fault state, 2010.04.13
Battery full cycle, battery temperature high, battery charging timeout */
unsigned int battery_check_state_mask=0;
#define BATTERY_TEMP_HIGH_FLAG            0x01  /* This flag will be set when T>=45 or T<=0 */
#define BATTERY_GASGAUGE_FULL_FLAG    0x02  /* This flay will be set when gas gauge report battery is 100% */

int g_battery_percentage;  //report to OS, user will see
int g_battery_voltage;
int g_battery_current;  /*FIHTDC, Add for setting POCV, MayLi, 2010.07.06*/
int g_battery_temperature;
int g_battery_charging_current = 0;
int g_init_charge_gpio=0;  /*FIHTDC, Fix Can not charging when device power on with charger, 2010.10.08*/ 

//FIHTDC, Add for setting low voltage POCV, MayLi, 2010.07.06{+
int g_GasGauge_percentage;    
int g_SW_OCV_percentage;
int g_SW_OCV_ReMap_Percentage;
int g_POCV_flag = 0;
//FIHTDC, Add for setting low voltage POCV, MayLi, 2010.07.06-}

//re-mapping gas gauge percentage, 6%~98% to 0%~100%
#define BAT_PERCENTAGE_USER_FULL    98 /*98*/
#define BAT_PERCENTAGE_USER_EMPTY   4  /*FIHTDC, MayLi adds for 3450mV shut down, 2010.08.31*/

bool suspend_update_flag;  /* Add for update battery information more accuracy in suspend mode*/
int pre_suspend_time;  /* Add for update battery information more accuracy only for battery update in suspend mode*/
int suspend_time_duration;


/* Add for debug mask + */
static int battery_debug_mask;
module_param_named(debug_mask, battery_debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);
/* Add for debug mask - */

/* FIH; Tiger; 2009/8/15 { */
/* ecompass needs this value to control charger behavior */
#ifdef CONFIG_FIH_FXX
extern unsigned fihsensor_battery_voltage;
extern unsigned fihsensor_battery_level;
extern int fihsensor_magnet_guard1;
#endif
/* } FIH; Tiger; 2009/8/15 */

struct ds2786_i2c_data
{
  	char reg;
	char data;
};
/* FIH, Michael Kao, 2010/05/07{ */
/* [FXX_CR], Add for solve OTG plug in show charging icon issue*/
extern int g_otg_status;
/* FIH, Michael Kao, 2010/05/07{ */

/*************I2C functions*******************/
static int i2c_rx( u8 * rxdata, int length )
{
	struct i2c_msg msgs[] =
	{
		{
			.addr = I2C_SLAVE_ADDR,
			.flags = 0,
			.len = 1,
			.buf = rxdata,
		},
		{
			.addr = I2C_SLAVE_ADDR,
			.flags = I2C_M_RD,
			.len = length,
			.buf = rxdata,
		},
	};

	int ret;
	if( ( ret = i2c_transfer( bat_ds2786_i2c->adapter, msgs, 2 ) ) < 0 )
	{
		printk(KERN_ERR "[DS2786]i2c rx failed %d\n", ret);
		return -EIO;
	}

	return 0;
}

#if 1
static int i2c_tx( u8 * txdata, int length )
{
	struct i2c_msg msg[] =
	{
		{
			.addr = I2C_SLAVE_ADDR,
			.flags = 0,
			.len = length,
			.buf = txdata,
		},
	};

	if( i2c_transfer( bat_ds2786_i2c->adapter, msg, 1 ) < 0 )
	{
	    if(length==1)
		    printk( KERN_ERR "[DS2786]i2c tx failed, addr(%x), len(%d), data0(0x%x)\n",msg[0].addr, msg[0].len, msg[0].buf[0]);
	   else
    		    printk( KERN_ERR "[DS2786]i2c tx failed, addr(%x), len(%d), data0(0x%x),data1(0x%x)\n",msg[0].addr, msg[0].len, msg[0].buf[0], msg[0].buf[1]);

	   return -EIO;
	}

	return 0;
}
#endif

int readMilliVolt(int *volt)
{
	short regData=0;
	struct ds2786_i2c_data value;

	value.reg = 0x0C;
	if(i2c_rx((u8*)&value, 2))
	{	
		printk(KERN_ERR "[DS2786]%s(): i2c_rx failed1!!\n", __func__);
		return -1;
	}
	else{
		//printk(KERN_INFO "[DS2786]%s() MSB: %d\n", __func__, value.reg);
		regData = (short)value.reg;
	}

	value.reg = 0x0D;
	if(i2c_rx((u8*)&value, 2))
	{
		printk(KERN_ERR "[DS2786]%s(): i2c_rx failed2!!\n", __func__);
		return -1;
	}
	else
	{
		regData = regData << 8;
		regData |= (short)value.reg;
		*volt = ((regData>>3)*122)/100;
		fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "[DS2786]%s(): %d mV\n"
															, __func__, *volt);
		//printk(KERN_INFO "[DS2786]%s(): %d mV\n", __func__, *volt);
	}

	return 0;
}

int readInitVolt(void)
{
	short regData=0;
	struct ds2786_i2c_data value;

	value.reg = 0x14;
	if(i2c_rx((u8*)&value, 2))
	{	
		printk(KERN_ERR "[DS2786]%s(): i2c_rx failed1!!\n", __func__);
		return -1;
	}
	else{
		//printk(KERN_INFO "[DS2786]%s() MSB: %d\n", __func__, value.reg);
		regData = (short)value.reg;
	}

	value.reg = 0x15;
	if(i2c_rx((u8*)&value, 2))
	{
		printk(KERN_ERR "[DS2786]%s(): i2c_rx failed2!!\n", __func__);
		return -1;
	}
	else
	{
		//printk(KERN_INFO "[DS2786]%s() LSB: %d\n", __func__, value.reg);
		regData = regData << 8;
		regData |= (short)value.reg;
		g_initVoltage = ((regData>>3)*122)/100;
		fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "[DS2786]%s(): %d mV\n"
															, __func__, g_initVoltage);
		eventlog("                                                            \n");
		eventlog("[BAT] ====================== \n");
		eventlog("[BAT] init-Volt:%d\n", g_initVoltage);
		//printk(KERN_INFO "[DS2786]%s(): %d mV\n", __func__, g_initVoltage);
	}

	return 0;
}

int readCurrent(int *cur)
{
	short regData=0;
	struct ds2786_i2c_data value;

	value.reg = 0x0E;
	if(i2c_rx((u8*)&value, 2))
	{
		printk(KERN_ERR "[DS2786]%s(): i2c_rx failed1!!\n", __func__);
		return -1;
	}
	else
	{
		//printk(KERN_INFO "[DS2786]%s() MSB: %d\n", __func__, value.reg);
		regData = (short)value.reg;
	}

	value.reg = 0x0F;
	if(i2c_rx((u8*)&value, 2))
	{
		printk(KERN_ERR "[DS2786]%s(): i2c_rx failed2!!\n", __func__);
		return -1;
	}
	else
	{
		//printk(KERN_INFO "[DS2786]%s() LSB: %d\n", __func__, value.reg);
		regData = regData << 8;
		regData |= (short)value.reg;
		*cur = ((regData>>4)*1250)/1000;
		fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "[DS2786]%s(): %d mA\n"
															, __func__, *cur);
		//printk(KERN_INFO "[DS2786]%s(): %d mA\n", __func__, *cur);
	}

	return 0;
}

int readPercentage(int *cap)
{
	struct ds2786_i2c_data value;

	value.reg = 0x02;
	if(i2c_rx((u8*)&value, 2))
	{
		printk(KERN_ERR "[DS2786]%s(): i2c_rx failed!!\n", __func__);
		return -1;
	}
	else
	{
		//printk(KERN_INFO "[DS2786]%s(): %d\n", __func__, value.reg);
		*cap = value.reg/2;
		fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "[DS2786]%s(): %d percent\n"
															, __func__, *cap);
		//printk(KERN_INFO "[DS2786]%s(): %d percent\n", __func__, *cap);
	}

	return 0;
}


int MapOhmToPercent(int Ohm)
{
	int Temp = 0;
	int i;
	if(Ohm>=MapTable[0].Ohm)  /*T <= -30 degree*/
		Temp=MapTable[0].T;
	else if (Ohm<=MapTable[105].Ohm)  /*T >= 75 degree*/
		Temp=MapTable[105].T;
	else
	{
		for(i=1; i<=104; i++)
		{
			if(Ohm>=MapTable[i].Ohm)
			{
				Temp=MapTable[i].T;
				break;
			}
		}
	}		
	return Temp;
}

/*FIHTDC, add for FM6 PR3 temperature, MayLi, 2010.07.06 {*/
int MapOhmToPercentPR3(int Ohm)
{
	int Temp = 0;
	int i;
	if(Ohm>=MapTablePR3[0].Ohm)  /*T <= -30 degree*/
		Temp=MapTablePR3[0].T;
	else if (Ohm<=MapTablePR3[105].Ohm)  /*T >= 75 degree*/
		Temp=MapTablePR3[105].T;
	else
	{
		for(i=1; i<=104; i++)
		{
			if(Ohm>=MapTablePR3[i].Ohm)
			{
				Temp=MapTablePR3[i].T;
				break;
			}
		}
	}		
	return Temp;
}
/*FIHTDC, add for FM6 PR3 temperature, MayLi, 2010.07.06 }*/

int readBatteryTemperature(int *BatTemp)
{
	int iOhm=0; /* Ohm */
	short regData=0;
	struct ds2786_i2c_data value;

	value.reg = 0x0A;   /* AIN1, will connect to battery temperature pin*/
	if(i2c_rx((u8*)&value, 2))
	{
		printk(KERN_ERR "[DS2786]%s(): i2c_rx failed1!!\n", __func__);
		return -1;
	}
	else
	{
		//printk(KERN_INFO "[DS2786]%s() MSB: %d\n", __func__, value.reg);
		regData = (short)value.reg;
	}

	value.reg = 0x0B;
	if(i2c_rx((u8*)&value, 2))
	{
		printk(KERN_ERR "[DS2786]%s(): i2c_rx failed2!!\n", __func__);
		return -1;
	}
	else
	{
		regData = regData << 8;
		regData |= (short)value.reg;
		regData = regData>>4;

		/*FIHTDC, add for FM6 PR3~, FN6 PR2~ temperature, MayLi, 2010.07.09 {*/
		if((g_orig_hwid>=CMCS_128_FM6_PR1 && g_orig_hwid<=CMCS_125_FM6_PR2) ||
			(g_orig_hwid==CMCS_FN6_ORIG_PR1))
		{
			iOhm =(regData*10)*1000/(2047-regData); /*R=10K, 2047 is gas gauge unite*/
			*BatTemp = MapOhmToPercent(iOhm);
		}
		else if((g_orig_hwid >=CMCS_128_FM6_PR3 && g_orig_hwid <= CMCS_125_FM6_MP) ||
			(g_orig_hwid>=CMCS_FN6_ORIG_PR2 && g_orig_hwid<=CMCS_FN6_ORIG_MP1))
		{
			//iOhm =(regData*100)*1000/(2047-regData); /*R=100K, 2047 is gas gauge unite*/
			iOhm =(regData*100)*10/(2047-regData); /*R=100K, 2047 is gas gauge unite, iOhm's unit is KOhm*0.1 */
			*BatTemp = MapOhmToPercentPR3(iOhm);
		}
		/*FIHTDC, add for FM6 PR3~, FN6 PR2~ temperature, MayLi, 2010.07.09 }*/
		
		fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G1,  "[DS2786]%s(): BatTemp=%d, reg=%d, Ohm=%d\n"
															, __func__, *BatTemp, regData, iOhm);
		//printk(KERN_INFO "[DS2786]%s(): BatTemp=%d, reg=%d, Ohm=%d\n", __func__, *BatTemp, regData, iOhm);
	}

	return 0;
}

int readGasGaugeConfigure(int *state)
{
	struct ds2786_i2c_data value;

	value.reg = 0x01;
	if(i2c_rx((u8*)&value, 2))
	{
		printk(KERN_ERR "[DS2786]%s(): i2c_rx failed!!\n", __func__);
		return -1;
	}
	else
	{
		//printk(KERN_INFO "[DS2786]%s(): %d\n", __func__, value.reg);
		*state = value.reg;
		fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "[DS2786]%s(): gas gauge state=0x%x\n"
															, __func__, *state);
		//printk(KERN_INFO "[DS2786]%s(): gas gauge state=0x%x\n", __func__, *state);
	}

	return 0;
}

/* FIH, Michael Kao, 2009/06/08{ */
/* [FXX_CR], Add For Blink RED LED when battery low in suspend mode */
void Battery_power_supply_change(void)
{
	/* FIH, Michael Kao, 2009/08/13{ */
	/* [FXX_CR], Modify to create a new work queue for BT play MP3 smoothly*/
	//	power_supply_changed(g_ps_battery);
	
	/* FIH, Michael Kao, 2009/09/30{ */
	/* [FXX_CR], add for update battery information more accuracy in suspend mode*/
	
	/* FIH, Michael Kao, 2009/12/01{ */
	/* [FXX_CR], add for update battery information more accuracy only for battery update in suspend mode*/
	suspend_time_duration=get_seconds()-pre_suspend_time;
	if(suspend_time_duration>300)
	{
		suspend_update_flag=true;
		g_ps_battery->changed = 1;  /*FIHTDC, MayLi adds for 6030 battery update, 2010.08.20*/
		queue_work(zeus_batt_wq, &g_ps_battery->changed_work);
		/* FIH, Michael Kao, 2009/10/14{ */
		/* [FXX_CR], Add wake lock to avoid incompleted update battery information*/
		wake_lock(&data->battery_wakelock);
		wakelock_flag=true;
		/* FIH, Michael Kao, 2009/10/14{ */
		pre_suspend_time=get_seconds();
		fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "Battery_power_supply_change : suspend_update_flag=%d,  wake_lock start\n",suspend_update_flag);
	}
	else
		fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "Battery_power_supply_change : suspend_time_duration=%d\n",suspend_time_duration);
	/* FIH, Michael Kao, 2009/12/01{ */
	/* FIH, Michael Kao, 2009/09/30{ */

	/* FIH, Michael Kao, 2009/08/13{ */
}
EXPORT_SYMBOL(Battery_power_supply_change);
/* } FIH, Michael Kao, 2009/06/08 */


void Battery_update_state(int _device, int device_state)
{
	if(_device==battery_charger_type)
	{
		batt_state.F9_battery_charger_type=device_state;
		printk(KERN_INFO "Battery_update_state, device=%d, device_state=%d\n", _device, device_state);
	}
	else if(_device==wifi_state)
		batt_state.F9_wifi_state=device_state;
	else if(_device==GPS_state)
		batt_state.F9_GPS_state=device_state;
	else if(_device==phone_state)
		batt_state.F9_phone_state=device_state;
	fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "[Battery_update_state]_device=%d, device_state=%d\n",_device, device_state);
}
EXPORT_SYMBOL(Battery_update_state);


void SetBatteryRelativeLED(void)
{
	//Turn off Red and Green LEDs
	//if(mpp_15)
	{
		//printk(KERN_INFO "Close Red led.\n");
		if(mpp_config_digital_out(mpp_15,MPP_CFG(MPP_DLOGIC_LVL_MSMP,MPP_DLOGIC_OUT_CTRL_HIGH)))
			printk(KERN_ERR "Close Red led failed!\n");
	}
	//else
		//printk(KERN_ERR "Did not get mpp15\n");
	
	//if(mpp_13)
	{
		//printk(KERN_INFO "Close green led.\n");
		if(mpp_config_digital_out(mpp_13,MPP_CFG(MPP_DLOGIC_LVL_MSMP,MPP_DLOGIC_OUT_CTRL_HIGH)))
			printk(KERN_ERR "Close green led failed!\n");
	}
	//else
		//printk(KERN_ERR "Did not get mpp13\n");
	
	if(g_charging_state==CHARGER_STATE_CHARGING)
	{
		//Turn on Red LED
		//if(mpp_15)
		{
			fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "%s():Open red led\n", __func__);
			if(mpp_config_digital_out(mpp_15,MPP_CFG(MPP_DLOGIC_LVL_MSMP,MPP_DLOGIC_OUT_CTRL_LOW)))
				printk(KERN_ERR "Open red led failed!\n");
		}
		//else
			//printk(KERN_ERR "Did not get mpp15\n");
	}
	else if(g_charging_state==CHARGER_STATE_FULL)
	{
		//Turn on Green LED
		//if(mpp_13)
		{
			fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "%s():Open green led\n", __func__);
			if(mpp_config_digital_out(mpp_13,MPP_CFG(MPP_DLOGIC_LVL_MSMP,MPP_DLOGIC_OUT_CTRL_LOW)))
				printk(KERN_ERR "Open green led failed!\n");
		}
		//else
			//printk(KERN_ERR "Did not get mpp13\n");
	}
}


//FIHTDC, Add for setting POCV, MayLi, 2010.07.06 {++
void SetGasGaugePOCV(void)
{	
	struct ds2786_i2c_data value;
	char i2cBuf[2];
	int ret=0;

	value.reg = 0xFE;
	if(i2c_rx((u8*)&value, 2))
	{
		printk(KERN_ERR "[DS2786]%s(): i2c_rx failed!!\n", __func__);
		return;
	}
	else
	{
		printk(KERN_INFO "[DS2786]%s(): 0xFE=0x%x\n", __func__, value.reg);
		value.reg |= 0x08;  // set bit3 POCV to 1
		i2cBuf[0] = 0xFE;
		i2cBuf[1] = value.reg;
		ret = i2c_tx((u8*)&i2cBuf, 2);
		if(ret)
		{
			printk(KERN_ERR "SetToInternalTemperature(): set SetGasGaugePOCV failed\n");
			ret = i2c_tx((u8*)&i2cBuf, 2);
			if(ret)
				printk(KERN_ERR "SetToInternalTemperature(): set SetGasGaugePOCV failed again !!!\n");
		}
	}
}


void Get_SW_OCV_Percentage(void)
{
	int i=0;
	//printk(KERN_ERR "User_P: %d, Ori-Percent: %d, SW_OCV_P: %d, Volt: %d, i: %d, POCV_flag: %d\n",
			//g_battery_percentage, g_GasGauge_percentage, g_SW_OCV_percentage, 
			//g_battery_voltage, g_battery_current, g_POCV_flag);
	if(g_battery_voltage <= g_Volt2PercentMode[0].Volt)
		g_SW_OCV_percentage = 0;
	else if(g_battery_voltage >= g_Volt2PercentMode[8].Volt)
		g_SW_OCV_percentage = 100;
	else
	{
		for(i=1; i<8; i++)
		{
			if(g_battery_voltage < g_Volt2PercentMode[i].Volt)
				break;
		}

		g_SW_OCV_percentage = (g_battery_voltage - g_Volt2PercentMode[i-1].Volt) * 
								(g_Volt2PercentMode[i].Percent - g_Volt2PercentMode[i-1].Percent) /
								(g_Volt2PercentMode[i].Volt- g_Volt2PercentMode[i-1].Volt) +
								g_Volt2PercentMode[i-1].Percent;
	}
}


void CheckSetPOCV(void)
{
	if(g_battery_voltage >= 4190 && (g_battery_current >= 0 && g_battery_current <=100 ))
	{
		g_POCV_flag = 1;
		SetGasGaugePOCV();
		eventlog("[BAT] Set High-POCV, Us_P=%d, Gas_P=%d, SW_P=%d, V=%d, i=%d, POCV_flag=%d\n", 
			g_battery_percentage, g_GasGauge_percentage, g_SW_OCV_percentage, 
			g_battery_voltage, g_battery_current, g_POCV_flag);
		
	}
	else if(abs(g_SW_OCV_percentage-g_GasGauge_percentage)>=5 && 
			g_battery_voltage<=3650 && (g_battery_current >= -150 && g_battery_current < 0 ))
	{
		g_POCV_flag = 1;
		SetGasGaugePOCV();
		eventlog("[BAT] Set Low-POCV, Us_P=%d, Gas_P=%d, SW_P=%d, V=%d, i=%d, POCV_flag=%d\n", 
			g_battery_percentage, g_GasGauge_percentage, g_SW_OCV_percentage, 
			g_battery_voltage, g_battery_current, g_POCV_flag);
		
	}
}
//FIHTDC, Add for setting POCV, MayLi, 2010.07.06 --}


void CheckBatteryFullCycle(void)
{
	int iPercentage=0;  /*gas gauge read back*/

	if(g_charging_state_last==CHARGER_STATE_FULL)
	{
		if(!readPercentage(&iPercentage))
		{
			if(iPercentage==100 && !(battery_check_state_mask&BATTERY_GASGAUGE_FULL_FLAG))
			{
				//disable charging
				battery_check_state_mask |= BATTERY_GASGAUGE_FULL_FLAG;
				gpio_set_value(GPIO_CHR_nCEN, 1 );
				eventlog("[BAT] Full Cycle: %d, DisCharging\n", iPercentage);
				printk(KERN_ERR "[DS2786]%s(): Full Cycle: %d, DisCharging\n", __func__, iPercentage);
			}
			else if(iPercentage<=BAT_PERCENTAGE_USER_FULL && (battery_check_state_mask&BATTERY_GASGAUGE_FULL_FLAG))
			{
				battery_check_state_mask &= ~BATTERY_GASGAUGE_FULL_FLAG;
				if(!battery_check_state_mask)
				{
					//enable charging
					gpio_set_value(GPIO_CHR_nCEN, 0 );
					
					eventlog("[BAT] Full Cycle: %d, EnCharging\n", iPercentage);
					printk(KERN_ERR "[DS2786]%s(): Full Cycle: %d, EnCharging\n", __func__, iPercentage);
				}
				else
				{
					//some error state of battery charging, can not enable charging.
					eventlog("[BAT] Full Cycle: %d, EnCharging stop:0x%x \n", iPercentage, battery_check_state_mask);
					printk(KERN_ERR "[DS2786]%s(): Full Cycle: %d, EnCharging stop:0x%x\n", __func__, iPercentage, battery_check_state_mask);
				}
			}
		}
	}
	else
		battery_check_state_mask &= ~BATTERY_GASGAUGE_FULL_FLAG;
}


int ReMapBatteryPercentage(int iP)
{
	int retPert=0;

	if(iP<=BAT_PERCENTAGE_USER_EMPTY)
		retPert=0;
	else if(iP>=BAT_PERCENTAGE_USER_FULL)
		retPert=100;
	else
		retPert = ((iP-BAT_PERCENTAGE_USER_EMPTY)*100 + (BAT_PERCENTAGE_USER_FULL-BAT_PERCENTAGE_USER_EMPTY)/2)
				/(BAT_PERCENTAGE_USER_FULL-BAT_PERCENTAGE_USER_EMPTY);
	
	return retPert;
}


void SetChargingCurrent(int iCurrent)
{
	printk(KERN_INFO "SetChargingCurrent, iCurrent=%d\n", iCurrent);
	eventlog("[BAT] Set current:%d\n", iCurrent);
	//Set charging current to iCurrent.

	/*FIHTDC, Fix Can not charging when device power on with charger, 2010.10.08 {+++*/
	if(g_init_charge_gpio==0)
	{
		printk(KERN_ERR "SetChargingCurrent, Init charge GPIO!!\r\n");
		//Init GPIOs, for setting charging current, MayLi
		gpio_direction_output(GPIO_CHR_IUSB,0 );
		//FIHTDC, add for FN6 charging, MayLi, 2010.06.21 {+
		if(!(g_orig_hwid>=CMCS_FN6_PR1 && g_orig_hwid<=CMCS_FN6_MP1) )
			gpio_direction_output(GPIO_CHR_DCM,0);
		//FIHTDC, add for FN6 charging, MayLi, 2010.06.21 -}
		gpio_direction_output(GPIO_CHR_USBSUS,1);
		gpio_direction_output(GPIO_CHR_nCEN,1);
		gpio_direction_output(GPIO_USB_2ACHG_SEL_n,0);
		g_init_charge_gpio = 1;
	}
	/*FIHTDC, Fix Can not charging when device power on with charger, 2010.10.08 ---} */

	switch(iCurrent)
	{
		case 0:
			gpio_set_value(GPIO_CHR_IUSB, 0 );
			/*FIHTDC, add for FN6 charging, MayLi, 2010.06.21 {*/
			if(g_orig_hwid>=CMCS_FN6_ORIG_PR1 && g_orig_hwid<=CMCS_FN6_ORIG_MP1)
			{
				if(mpp_config_digital_out(mpp_4,MPP_CFG(MPP_DLOGIC_LVL_MSMP,MPP_DLOGIC_OUT_CTRL_LOW)))
					printk(KERN_ERR "Set mpp_4 to low failed!\n");
			}else
				gpio_set_value(GPIO_CHR_DCM, 0 );
			/*FIHTDC, add for FN6 charging, MayLi, 2010.06.21 }*/
			
			gpio_set_value(GPIO_CHR_USBSUS, 1 );
			//gpio_set_value(GPIO_CHR_nCEN, 1 );
			gpio_set_value(GPIO_USB_2ACHG_SEL_n, 0 );
			
			gpio_set_value(GPIO_CHR_nCEN, 1 );
			break;
			
		case 500:
			gpio_set_value(GPIO_CHR_IUSB, 1 );
			/*FIHTDC, add for FN6 charging, MayLi, 2010.06.21 {*/
			if(g_orig_hwid>=CMCS_FN6_ORIG_PR1 && g_orig_hwid<=CMCS_FN6_ORIG_MP1)
			{	
				if(mpp_config_digital_out(mpp_4,MPP_CFG(MPP_DLOGIC_LVL_MSMP,MPP_DLOGIC_OUT_CTRL_LOW)))
					printk(KERN_ERR "Set mpp_4 to low failed!\n");
			}else
				gpio_set_value(GPIO_CHR_DCM, 0 );
			/*FIHTDC, add for FN6 charging, MayLi, 2010.06.21 }*/
			
			gpio_set_value(GPIO_CHR_USBSUS, 0 );
			//gpio_set_value(GPIO_CHR_nCEN, 0 );
			gpio_set_value(GPIO_USB_2ACHG_SEL_n, 0 );

			if(!battery_check_state_mask)
				gpio_set_value(GPIO_CHR_nCEN, 0 );
			else
				printk(KERN_INFO "SetChargingCurrent, Error=0x%x\n", battery_check_state_mask);
			break;

		case 1000:
			gpio_set_value(GPIO_CHR_IUSB, 0 );
			/*FIHTDC, add for FN6 charging, MayLi, 2010.06.21 {*/
			if(g_orig_hwid>=CMCS_FN6_ORIG_PR1 && g_orig_hwid<=CMCS_FN6_ORIG_MP1)
			{
				if(mpp_config_digital_out(mpp_4,MPP_CFG(MPP_DLOGIC_LVL_MSMP,MPP_DLOGIC_OUT_CTRL_HIGH)))
					printk(KERN_ERR "Set mpp_4 to high failed!\n");
			}else
				gpio_set_value(GPIO_CHR_DCM, 1 );
			/*FIHTDC, add for FN6 charging, MayLi, 2010.06.21 }*/
			gpio_set_value(GPIO_CHR_USBSUS, 0 );
			//gpio_set_value(GPIO_CHR_nCEN, 0 );
			gpio_set_value(GPIO_USB_2ACHG_SEL_n, 0 );

			if(!battery_check_state_mask)
				gpio_set_value(GPIO_CHR_nCEN, 0 );
			else
				printk(KERN_INFO "SetChargingCurrent, Error=0x%x\n", battery_check_state_mask);
			break;

		case 2000:
			gpio_set_value(GPIO_CHR_IUSB, 0 );
			/*FIHTDC, add for FN6 charging, MayLi, 2010.06.21 {*/
			if(g_orig_hwid>=CMCS_FN6_ORIG_PR1 && g_orig_hwid<=CMCS_FN6_ORIG_MP1)
			{	if(mpp_config_digital_out(mpp_4,MPP_CFG(MPP_DLOGIC_LVL_MSMP,MPP_DLOGIC_OUT_CTRL_HIGH)))
					printk(KERN_ERR "Set mpp_4 to high failed!\n");
			}else
				gpio_set_value(GPIO_CHR_DCM, 1 );
			/*FIHTDC, add for FN6 charging, MayLi, 2010.06.21 }*/
			gpio_set_value(GPIO_CHR_USBSUS, 0 );
			//gpio_set_value(GPIO_CHR_nCEN, 0 );
			gpio_set_value(GPIO_USB_2ACHG_SEL_n, 1 );

			if(!battery_check_state_mask)
				gpio_set_value(GPIO_CHR_nCEN, 0 );
			else
				printk(KERN_INFO "SetChargingCurrent, Error=0x%x\n", battery_check_state_mask);
			break;

		default:
			printk(KERN_ERR "[DS2786]%s(): Do not support current: %d\n", __func__, iCurrent);
			break;
	}

	g_battery_charging_current = iCurrent;
}
EXPORT_SYMBOL(SetChargingCurrent);


void CheckBatteryTemperature(void)
{
	if(g_charging_state_last==CHARGER_STATE_CHARGING || g_charging_state_last==CHARGER_STATE_FULL)
	{
		if((g_battery_temperature<0 || g_battery_temperature>45) && 
			!(battery_check_state_mask&BATTERY_TEMP_HIGH_FLAG))  /* Over temperature*/
		{
			//disable charging
			battery_check_state_mask |= BATTERY_TEMP_HIGH_FLAG;
			gpio_set_value(GPIO_CHR_nCEN, 1 );
			
			eventlog("[BAT] OTP: %d, DisCharging\n", g_battery_temperature);
			printk(KERN_ERR "[DS2786]%s(): OTP: %d, DisCharging\n", __func__, g_battery_temperature);
		}
		else if((g_battery_temperature>=5 && g_battery_temperature<=40) && 
			(battery_check_state_mask&BATTERY_TEMP_HIGH_FLAG))  /*Temperature save*/
		{
			//re-charging
			battery_check_state_mask &= ~BATTERY_TEMP_HIGH_FLAG;
			if(!battery_check_state_mask)
			{
				//enable charging
				gpio_set_value(GPIO_CHR_nCEN, 0 );
				
				eventlog("[BAT] OTP: %d, EnCharging\n", g_battery_temperature);
				printk(KERN_ERR "[DS2786]%s(): OTP: %d, EnCharging\n", __func__, g_battery_temperature);
			}
			else
			{
				//some error state of battery charging, can not enable charging.
				eventlog("[BAT] OTP: %d, EnCharging stop:0x%x \n", g_battery_temperature, battery_check_state_mask);
				printk(KERN_ERR "[DS2786]%s(): OTP: %d, EnCharging stop:0x%x\n", 
					__func__, g_battery_temperature, battery_check_state_mask);
			}
		}
	}
	else
		battery_check_state_mask &= ~BATTERY_TEMP_HIGH_FLAG;
}

void SetToInternalTemperature(void)
{
	int state=0;
	char i2cBuf[2];

	int ret=0;

	if(!readGasGaugeConfigure(&state))
	{
		if(state & 0x4)  /*check ITEMP, bit2*/
		{
			state &= ~0x4;
			i2cBuf[0] = 0x01;
			i2cBuf[1] = state&0xFF;
			ret = i2c_tx((u8*)&i2cBuf, 2);
			if(ret)
			{
				printk(KERN_ERR "SetToInternalTemperature(): set i2c tx on failed\n");
				ret = i2c_tx((u8*)&i2cBuf, 2);
				if(ret)
					printk(KERN_ERR "SetToInternalTemperature(): set i2c tx on failed again !!!\n");
			}
		}
	}
}


//FIHTDC, Add for new battery low shut down condition, MayLi, 2010.07.06 {
int BatteryLowShutDownDevice(void)
{
	int ShutDownFlag = 0;
	int i=0;
	int shutdownflag=0;
	int volt=0;

	//FIHTDC, Add for delay device power on time, MayLi, 2010.07.23 {+ 
	if((g_battery_percentage<=0 && g_battery_voltage<=3450) ||
   		(g_battery_voltage<=3450))  /*FIHTDC, MayLi adds for 3450mV shut down, 2010.08.31*/
	//if(g_battery_percentage==0 || g_battery_voltage<=3550)  /*FIHTDC, Here is for 3550mV shut down condition, MayLi, 2010.07.01*/
	{
		//FIHTDC, Add for double check battery low shut down condition, MayLi, 2010.06.30 {+
		//printk(KERN_ERR "BatteryLowShutDownDevice(): volt=%d, i=%d\n", g_battery_voltage, i);
		/*if(g_battery_percentage==0)
			ShutDownFlag = 1;
		else*/ if(g_battery_voltage<=3450)  /*FIHTDC, MayLi adds for 3450mV shut down, 2010.08.31*/
		//FIHTDC, Add for delay device power on time, MayLi, 2010.07.23 -}
		{
			for(i=0; i<2; i++)
			{
				//sleep 500ms, it must more than 440ms.
				msleep(500);
				
				if(!readMilliVolt(&volt))
				{
					//printk(KERN_ERR "BatteryLowShutDownDevice(): volt=%d, i=%d\n", volt, i);
					if(volt<=3450 && shutdownflag<2)  /*FIHTDC, MayLi adds for 3450mV shut down, 2010.08.31*/
						shutdownflag = shutdownflag + 1;
					else
						break;
				}
			}

			if(shutdownflag>=2)
				ShutDownFlag = 1;
		}
		//FIHTDC, Add for double check battery low shut down condition, MayLi, 2010.06.30 -}
	}
	return ShutDownFlag;
}
//FIHTDC, Add for new battery low shut down condition, MayLi, 2010.07.06 }

//FIHTDC, Add for monitor and clear abnormal Learn Capacity Scaling Factor, MayLi, 2010.07.23 {+
void SetGasGaugeSOCV(void)
{	
	struct ds2786_i2c_data value;
	char i2cBuf[2];
	int ret=0;

	value.reg = 0xFE;
	if(i2c_rx((u8*)&value, 2))
	{
		printk(KERN_ERR "[DS2786]%s(): i2c_rx failed!!\n", __func__);
		return;
	}
	else
	{
		printk(KERN_INFO "[DS2786]%s(): 0xFE=0x%x\n", __func__, value.reg);
		value.reg |= 0x04;  // set bit2 SOCV to 1
		i2cBuf[0] = 0xFE;
		i2cBuf[1] = value.reg;
		ret = i2c_tx((u8*)&i2cBuf, 2);
		if(ret)
		{
			printk(KERN_ERR "set SetGasGaugeSOCV failed\n");
			ret = i2c_tx((u8*)&i2cBuf, 2);
			if(ret)
				printk(KERN_ERR "set SetGasGaugeSOCV failed again !!!\n");
		}
	}
}

void SetGasGaugePOR(void)
{	
	struct ds2786_i2c_data value;
	char i2cBuf[2];
	int ret=0;

	value.reg = 0xFE;
	if(i2c_rx((u8*)&value, 2))
	{
		printk(KERN_ERR "[DS2786]%s(): i2c_rx failed!!\n", __func__);
		return;
	}
	else
	{
		printk(KERN_INFO "[DS2786]%s(): 0xFE=0x%x\n", __func__, value.reg);
		value.reg |= 0x80;  // set bit7 POR to 1
		i2cBuf[0] = 0xFE;
		i2cBuf[1] = value.reg;
		ret = i2c_tx((u8*)&i2cBuf, 2);
		if(ret)
		{
			printk(KERN_ERR "SetToInternalTemperature(): set SetGasGaugePOR failed\n");
			ret = i2c_tx((u8*)&i2cBuf, 2);
			if(ret)
				printk(KERN_ERR "SetToInternalTemperature(): set SetGasGaugePOR failed again !!!\n");
		}
	}
}


void Init_GasGauge_OCV(void)
{
	int ret=0;
	char i2cBuf[27] = {	0x00,
					0x0C,0x1B,0x39,0x57,0x75,0x84,0xAA,0xA8,0x10,0xB7,
					0x50,0xBC,0x10,0xC0,0x40,0xC1,0xC0,0xC4,0x90,0xC7,
					0xB0,0xCE,0xC0,0xD6,0xA0,0x13};

	i2cBuf[0] = 0x61;  /*gas gauge start register*/

	ret = i2c_tx((u8*)&i2cBuf, 27);
	if(ret)
	{
		printk(KERN_ERR "[May]: Init_GasGauge_OCV failed\n");
		ret = i2c_tx((u8*)&i2cBuf, 27);
		if(ret)
			printk(KERN_ERR "[May]: Init_GasGauge_OCV failed again !!!\n");
	}

	//SetGasGaugeSOCV();

}


void check_OCV_table(void)
{
	struct ds2786_i2c_data value;

	int i=0,retrycnt=0;
	
	char registers[26] = {0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,
	                          0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0x73,0x74,
	                          0x75,0x76,0x77,0x78,0x79,0x7A};

	char values[26] = {0x0C,0x1B,0x39,0x57,0x75,0x84,0xAA,0xA8,0x10,0xB7,
	                       0x50,0xBC,0x10,0xC0,0x40,0xC1,0xC0,0xC4,0x90,0xC7,
	                       0xB0,0xCE,0xC0,0xD6,0xA0,0x13};

	for(i=0;i<26;i++)
	{
		retrycnt = 0;
		value.reg = registers[i];

		while(retrycnt < 10)
		{	
			if(i2c_rx((u8*)&value, 2))
			{
				printk(KERN_ERR "[DS2786]%s(): i2c_rx failed at register 0x%x!!%d times\n", __func__, registers[i], retrycnt);
				retrycnt++;
			}
			else
			{
				if(value.reg != values[i])
				{
					printk(KERN_ERR "[DS2786]%s(): Register 0x%x value = %x is not match OCV!\n", __func__, registers[i], value.reg);
					g_OCVstatus = 0;
				}
				break;
			}
		}
		if(g_OCVstatus ==0)
		{
			Init_GasGauge_OCV();
			SetGasGaugeSOCV();
			break;
		}
	}
	
}


void CheckLearnCapacityScalingFactor(void)
{
	struct ds2786_i2c_data value;

	value.reg = 0x17;
	if(i2c_rx((u8*)&value, 2))
	{
		printk(KERN_ERR "[DS2786]%s(): i2c_rx failed!!\n", __func__);
		return;
	}
	else
	{
		if(g_LearnCapacityScalingFactor!=value.reg)
		{
			g_LearnCapacityScalingFactor=value.reg;
			eventlog("[BAT] LCSF:0x%x\n", g_LearnCapacityScalingFactor);
		}

		if(g_LearnCapacityScalingFactor==0xFF)   /*Big error!!!*/
		{
			eventlog("[BAT] LCSF:0x%x, Set POR\n", g_LearnCapacityScalingFactor);
			printk(KERN_INFO "%s: LCSF=%d, Set POR flag\n", __func__, g_LearnCapacityScalingFactor);
			SetGasGaugePOR();
			
			/*Set gas gauge ITEMP to 0, for measuring external BAT_temp, MayLi*/
			printk(KERN_INFO "Set Ex-Temp flag\n");
			SetToInternalTemperature();

			printk(KERN_INFO "Init OCV table\n");
			Init_GasGauge_OCV();

			check_OCV_table();
			printk(KERN_INFO "Check OCV table=%d\n", g_OCVstatus);
		}
	}
}
//FIHTDC, Add for monitor and clear abnormal Learn Capacity Scaling Factor, MayLi, 2010.07.23 -}


static int goldfish_battery_get_property(struct power_supply *psy,
				 enum power_supply_property psp,
				 union power_supply_propval *val)
{
//	int buf;
	int ret = 0;
	//int cap = 0;
	int tempcap=0;
	int vol = 0;
	int cur = 0;
	int temp=0;
	int GPIO2_OUT_1,GPIO2_OE_1;
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		//fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "<ubh> goldfish_battery_get_property : POWER_SUPPLY_PROP_STATUS\r\n");
		// "Unknown", "Charging", "Discharging", "Not charging", "Full"
             if (g_charging_state != CHARGER_STATE_LOW_POWER)
			val->intval = g_charging_state;
             else 
             	val->intval = CHARGER_STATE_NOT_CHARGING;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		//fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "<ubh> goldfish_battery_get_property : POWER_SUPPLY_PROP_HEALTH : AVC(%d)\r\n", buf);
		// "Unknown", "Good", "Overheat", "Dead", "Over voltage", "Unspecified failure"
		if(battery_check_state_mask&BATTERY_TEMP_HIGH_FLAG)
			val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;
		else
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
		//fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "POWER_SUPPLY_PROP_HEALTH : HEALTH=%d\n",val->intval);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		//GetBatteryInfo(BATT_CURRENT_INFO, &buf);
		//fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "<ubh> goldfish_battery_get_property : POWER_SUPPLY_PROP_PRESENT : C(%d)\r\n", buf);
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		//fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "<ubh> goldfish_battery_get_property : POWER_SUPPLY_PROP_TECHNOLOGY\r\n");
		// "Unknown", "NiMH", "Li-ion", "Li-poly", "LiFe", "NiCd", "LiMn"
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = g_battery_temperature*10;
		//fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "POWER_SUPPLY_PROP_TEMP : msm_termal=%d\n",msm_termal);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = g_battery_voltage*1000;  /*Add for Eclair *#*#4636#*#*, MayLi, 2010.07.20*/
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		// 1. Read current
		if(!readCurrent(&cur))
			g_battery_current = cur;

		// 2. Read voltage
		if(!readMilliVolt(&vol))
			g_battery_voltage=vol;
		
		// 3. Read percentage
		if(percentage_mode)  /* RD uses command mode */
		{
			val->intval = manual_percentage;
			g_battery_percentage = val->intval;
			if(!readPercentage(&g_GasGauge_percentage)) 
				printk(KERN_INFO "manual_percentage:%d\n", manual_percentage);
		}
		else
		{
			if(!readPercentage(&g_GasGauge_percentage)) 
			{
				//FIHTDC, Add for setting Low voltage POCV, MayLi, 2010.07.06 {+
				Get_SW_OCV_Percentage();  /*use g_battery_voltage to get sw ocv percentage*/
				g_SW_OCV_ReMap_Percentage = ReMapBatteryPercentage(g_SW_OCV_percentage);
				tempcap = ReMapBatteryPercentage(g_GasGauge_percentage);

				if(g_POCV_flag==1)
				{
					if(g_battery_percentage>g_SW_OCV_ReMap_Percentage)
					{
						g_battery_percentage = g_battery_percentage -1;
						eventlog("[BAT] Us_P=%d, gas_P=%d, gas_Mp=%d, SW_Mp=%d, v=%d, i=%d, pocv=%d\n", 
							g_battery_percentage, g_GasGauge_percentage, tempcap, g_SW_OCV_ReMap_Percentage, 
							g_battery_voltage, g_battery_current, g_POCV_flag);
					}
					else if(g_battery_percentage<g_SW_OCV_ReMap_Percentage && 
						(g_charging_state_last==CHARGER_STATE_CHARGING || 
							g_charging_state_last==CHARGER_STATE_FULL))
					{
						g_battery_percentage = g_battery_percentage +1;
						eventlog("[BAT] Us_P=%d, gas_P=%d, gas_Mp=%d, SW_Mp=%d, v=%d, i=%d, pocv=%d\n", 
							g_battery_percentage, g_GasGauge_percentage, tempcap, g_SW_OCV_ReMap_Percentage, 
							g_battery_voltage, g_battery_current, g_POCV_flag);
					}
					else if(g_battery_percentage==g_SW_OCV_ReMap_Percentage)
					{
						g_POCV_flag = 0;
						eventlog("[BAT] Us_P=%d, gas_P=%d, gas_Mp=%d, SW_Mp=%d, v=%d, i=%d, pocv=%d\n", 
							g_battery_percentage, g_GasGauge_percentage, tempcap, g_SW_OCV_ReMap_Percentage, 
							g_battery_voltage, g_battery_current, g_POCV_flag);
					}
				}
				else
				{
					if(g_battery_percentage != tempcap)
					{
						g_battery_percentage = tempcap;
						eventlog("[BAT] Us_P=%d, gas_P=%d, gas_Mp=%d, SW_Mp=%d, v=%d, i=%d, pocv=%d\n", 
							g_battery_percentage, g_GasGauge_percentage, tempcap, g_SW_OCV_ReMap_Percentage, 
							g_battery_voltage, g_battery_current, g_POCV_flag);
					}
				}
				//FIHTDC, Add for setting Low voltage POCV, MayLi, 2010.07.06 -}
				val->intval=g_battery_percentage;
			}
		}

		//Here is for battery low shut down check
		if(BatteryLowShutDownDevice())
		{
			val->intval=0;
			eventlog("[BAT] Bat Low Shut Down, Us_P=%d, gas_P=%d, V=%d\n", 
							g_battery_percentage, g_GasGauge_percentage, g_battery_voltage);
			printk(KERN_ERR "goldfish_battery_get_property: Bat Low Shut Down, P=%d, V=%d\n", g_battery_percentage, g_battery_voltage);
		}
		else
		{
			//Do not update percentage=0, when it does not meet battery low shut down condition
			if(g_battery_percentage==0)
			{
				val->intval += 1;
				fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G1,  "%s():Not meet battery low shut down condition\n", __func__);
			}
		}

		// 4. read temperature
		if(!readBatteryTemperature(&temp))
			g_battery_temperature = temp;

		//fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G1,  "Ori-Percent: %d, Percent: %d, Vol: %d, T: %d, cur: %d, charging state: %d, OCVstatus: %d, initVoltage: %d\n", 
			//cap, g_battery_percentage, g_battery_voltage,  g_battery_temperature,
			//cur, g_charging_state, g_OCVstatus, g_initVoltage);
			
		printk(KERN_ERR "[May] User_P=%d, Gas_P=%d, Gas_MP=%d, SW_OCV_P=%d, SW_OCV_MP=%d, POCV_flag=%d\n",
			g_battery_percentage, g_GasGauge_percentage, tempcap, g_SW_OCV_percentage, g_SW_OCV_ReMap_Percentage, g_POCV_flag);
		printk(KERN_ERR "[May] Vol=%d, T=%d, cur=%d, charging state=%d, OCVstatus=%d, initVoltage=%d \n", 
			g_battery_voltage,  g_battery_temperature, g_battery_current, g_charging_state, g_OCVstatus, g_initVoltage);
		
		// Check battery full flag ++
       	if ((g_battery_percentage == 100)&&(g_charging_state == CHARGER_STATE_CHARGING))
		{
		 	g_charging_state = CHARGER_STATE_FULL;
			fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "%s(): set the charging status to full\n", __func__);
		}
		else if (g_charging_state == CHARGER_STATE_NOT_CHARGING)
		{
			// We do not support Red LED blink when battery is low.
			if (g_battery_percentage < 15) 
			{
				g_charging_state = CHARGER_STATE_LOW_POWER;
				fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "%s(): set the charging status to low power\n", __func__);
		  	}
		}
		// Check battery full flag --
		
		if(g_charging_state_last != g_charging_state)
		{
			SetBatteryRelativeLED();
			g_charging_state_last = g_charging_state;
		}
		CheckBatteryTemperature();
		CheckBatteryFullCycle();
		CheckSetPOCV();

		CheckLearnCapacityScalingFactor(); /*FIHTDC, Add for monitor and clear abnormal Learn Capacity Scaling Factor, MayLi, 2010.07.23 */

		/* FIH, Michael Kao, 2009/10/14{ */
		/* [FXX_CR], Add wake lock to avoid incompleted update battery information*/
		if(wakelock_flag)
		{
			wake_unlock(&data->battery_wakelock);
			wakelock_flag=false;
			fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "POWER_SUPPLY_PROP_CAPACITY : wake_unlock\n");
		}
		/* FIH, Michael Kao, 2009/10/14{ */
		/* FIH, Michael Kao, 2010/05/06{ */
		/* [FXX_CR], Add for dump GPIO OUT status*/
		GPIO2_OUT_1 = readl(ioremap(0xA9300C00, 4096) + 0x0000);  //GPIO2_OUT_1
		GPIO2_OE_1 = readl(ioremap(0xA9300C00, 4096) + 0x0008);  //GPIO2_OE_1
		printk(KERN_INFO "POWER_SUPPLY_PROP_CAPACITY : GPIO2_OUT_1 = 0x%x, GPIO2_OE_1 = 0x%x\n", GPIO2_OUT_1, GPIO2_OE_1);
		/* FIH, Michael Kao, 2010/05/06{ */
             break;
	default:
		printk(KERN_ERR "goldfish_battery_get_property : psp(%d)\n", psp);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static enum power_supply_property goldfish_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

///static enum power_supply_property goldfish_ac_props[] = {
///	POWER_SUPPLY_PROP_ONLINE,
///};


#ifdef T_FIH	///+T_FIH
#ifdef FLAG_BATTERY_POLLING
static struct timer_list polling_timer;


#define BATTERY_POLLING_TIMER  30000//300000 10000

static void polling_timer_func(unsigned long unused)
{
	/* FIH, Michael Kao, 2009/08/13{ */
	/* [FXX_CR], Modify to create a new work queue for BT play MP3 smoothly*/
	//power_supply_changed(g_ps_battery);
	g_ps_battery->changed = 1;  /*FIHTDC, MayLi adds for 6030 battery update, 2010.08.20*/
	queue_work(zeus_batt_wq, &g_ps_battery->changed_work);
	
	/* FIH, Michael Kao, 2009/10/14{ */
	/* [FXX_CR], Add wake lock to avoid incompleted update battery information*/
	wake_lock(&data->battery_wakelock);
	wakelock_flag=true;
	fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "polling_timer_func : wake_lock start\n");
	/* FIH, Michael Kao, 2009/10/14{ */
	/* FIH, Michael Kao, 2009/08/13{ */
	mod_timer(&polling_timer,
		  jiffies + msecs_to_jiffies(BATTERY_POLLING_TIMER));
}
#endif	// FLAG_BATTERY_POLLING

#ifdef FLAG_CHARGER_DETECT
static irqreturn_t chgdet_irqhandler(int irq, void *dev_id)
{
	/* FIH, Michael Kao, 2010/05/07{ */
	/* [FXX_CR], Add for solve OTG plug in show charging icon issue*/
	if(g_otg_status==1)
		return IRQ_HANDLED;
	/* FIH, Michael Kao, 2010/05/07{ */
	g_charging_state = (gpio_get_value(GPIO_CHR_DET)) ? CHARGER_STATE_NOT_CHARGING : CHARGER_STATE_CHARGING;

	/* Modify to create a new work queue for BT play MP3 smoothly*/
	//power_supply_changed(g_ps_battery);
	g_ps_battery->changed = 1;  /*FIHTDC, MayLi adds for 6030 battery update, 2010.08.20*/
	queue_work(zeus_batt_wq, &g_ps_battery->changed_work);
	
	/* [FXX_CR], Add wake lock to avoid incompleted update battery information*/
	wake_lock(&data->battery_wakelock);
	wakelock_flag=true;
	//Add by May +
	if(g_charging_state == CHARGER_STATE_NOT_CHARGING)
	{
		//gpio_set_value(GPIO_CHR_DCM , 0 );
		fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "chgdet_irqhandler, Charger Plug OUT!!!\n");
		printk(KERN_ERR "%s: Charger Plug OUT!!!\n", __func__);
		eventlog("[BAT] CHR OUT\n");
		SetChargingCurrent(0);
	}
	else
	{
		fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "chgdet_irqhandler, Charger Plug IN!!!\n");
		printk(KERN_ERR "%s: Charger Plug IN!!!\n", __func__);
		eventlog("[BAT] CHR IN\n");
	}
	//Add by May -
	fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "chgdet_irqhandler : wake_lock start\n");
	/* FIH, Michael Kao, 2009/10/14{ */
	/* FIH, Michael Kao, 2009/08/13{ */
	return IRQ_HANDLED;
}


//FIHTDC, Adds for charging timeout detect, MayLi, 2010.05.12 {+
static irqreturn_t chgflt_irqhandler(int irq, void *dev_id)
{
	g_charging_fault = gpio_get_value(GPIO_CHR_FLT);

	if(!g_charging_fault)
		eventlog("[BAT] CHR FAULT!!!\n");
	else
		eventlog("[BAT] CHR NOT FAULT!!!\n");

	return IRQ_HANDLED;
}
//FIHTDC, Adds for charging timeout detect, MayLi, 2010.05.12 -}

//FIHTDC, Adds for USB OVP detect, MayLi, 2010.05.20 {+
#if 0  /*MayLi masks it, 2010.07.08 ++*/
static irqreturn_t usbovp_irqhandler(int irq, void *dev_id)
{
	g_usb_ovp = gpio_get_value(GPIO_USBIN_OVP_N);

	if(!g_usb_ovp)
		eventlog("[BAT] USB OVP!!!\n");
	else
		eventlog("[BAT] USB NOT OVP!!!\n");

	return IRQ_HANDLED;
}
#endif   /*MayLi masks it, 2010.07.08--*/
//FIHTDC, Adds for USB OVP detect, MayLi, 2010.05.20 -}
#endif	// FLAG_CHARGER_DETECT
#endif	// T_FIH	///-T_FIH


static int goldfish_battery_probe(struct platform_device *pdev)
{
	int ret;
	//struct goldfish_battery_data *data;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (data == NULL) {
		ret = -ENOMEM;
		goto err_data_alloc_failed;
	}
	spin_lock_init(&data->lock);
	
	/* FIH, Michael Kao, 2009/10/14{ */
	/* [FXX_CR], Add wake lock to avoid incompleted update battery information*/
	wake_lock_init(&data->battery_wakelock, WAKE_LOCK_SUSPEND, "zeus_battery");
	/* FIH, Michael Kao, 2009/10/14{ */
	data->battery.properties = goldfish_battery_props;
	data->battery.num_properties = ARRAY_SIZE(goldfish_battery_props);
	data->battery.get_property = goldfish_battery_get_property;
	data->battery.name = "battery";
	data->battery.type = POWER_SUPPLY_TYPE_BATTERY;
/*	data->ac.properties = goldfish_ac_props;
	data->ac.num_properties = ARRAY_SIZE(goldfish_ac_props);
	data->ac.get_property = goldfish_ac_get_property;
	data->ac.name = "ac";
	data->ac.type = POWER_SUPPLY_TYPE_MAINS;*/

	/* Add for update battery information more accuracy in suspend mode*/
	suspend_update_flag=false;
	/* [FXX_CR], add for update battery information more accuracy only for battery update in suspend mode*/
	pre_suspend_time=0;
	
	/* Add wake lock to avoid incompleted update battery information*/
	wakelock_flag=false;
	
	g_battery_voltage=0;
	g_battery_percentage=0;
	g_battery_temperature=25;
	//g_battery_charging_current=0; 	/*FIHTDC, Fix Can not charging when device power on with charger, 2010.10.08 */
		
	ret = power_supply_register(&pdev->dev, &data->battery);
	if (ret)
		goto err_battery_failed;
        ///ret = power_supply_register(&pdev->dev, &data->ac);
	///if (ret)
		///goto err_battery_failed;

	platform_set_drvdata(pdev, data);
	battery_data = data;
	/* FIH, Michael Kao, 2009/08/13{ */
	/* [FXX_CR], Modify to create a new work queue for BT play MP3 smoothly*/
	zeus_batt_wq=create_singlethread_workqueue("zeus_battery");
	if (!zeus_batt_wq) {
		printk(KERN_ERR "%s: create workque failed", __func__);
		return -ENOMEM;
	}
	/* FIH, Michael Kao, 2009/08/13{ */

#ifdef T_FIH	///+T_FIH
#ifdef FLAG_BATTERY_POLLING
	setup_timer(&polling_timer, polling_timer_func, 0);
	mod_timer(&polling_timer,
		  jiffies + msecs_to_jiffies(BATTERY_POLLING_TIMER));
	g_ps_battery = &(data->battery);
#endif	// FLAG_BATTERY_POLLING

#ifdef FLAG_CHARGER_DETECT
	gpio_tlmm_config( GPIO_CFG(GPIO_CHR_DET, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA ), GPIO_ENABLE );
	ret = gpio_request(GPIO_CHR_DET, "gpio_chg_irq");
	if (ret)
		printk(KERN_ERR "<ubh> goldfish_battery_probe 04. : gpio_request fails!!!\r\n");

	ret = gpio_direction_input(GPIO_CHR_DET);
	if (ret)
		printk(KERN_ERR "<ubh> goldfish_battery_probe 05. : gpio_direction_input fails!!!\r\n");

	ret = request_irq(MSM_GPIO_TO_INT(GPIO_CHR_DET), &chgdet_irqhandler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, pdev->name, NULL);
	if (ret)
		printk(KERN_ERR "<ubh> goldfish_battery_probe 06. : request_irq fails!!!\r\n");
	else
		enable_irq_wake(MSM_GPIO_TO_INT(GPIO_CHR_DET));

	//FIHTDC, Adds for charging timeout detect, MayLi, 2010.05.12 {+
	gpio_tlmm_config( GPIO_CFG(GPIO_CHR_FLT, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA ), GPIO_ENABLE );
	ret = gpio_request(GPIO_CHR_FLT, "gpio_chg_flt_irq");
	if (ret)
		printk(KERN_ERR "<ubh> goldfish_battery_probe 07. : gpio_request fails!!!\r\n");

	ret = gpio_direction_input(GPIO_CHR_FLT);
	if (ret)
		printk(KERN_ERR "<ubh> goldfish_battery_probe 08. : gpio_direction_input fails!!!\r\n");

	ret = request_irq(MSM_GPIO_TO_INT(GPIO_CHR_FLT), &chgflt_irqhandler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, pdev->name, NULL);
	if (ret)
		printk(KERN_ERR "<ubh> goldfish_battery_probe 09. : request_irq fails!!!\r\n");
	else
		enable_irq_wake(MSM_GPIO_TO_INT(GPIO_CHR_FLT));
	//FIHTDC, Adds for charging timeout detect, MayLi, 2010.05.12 -}

	//FIHTDC, Adds for USB OVP detect, MayLi, 2010.05.20 {+
	/*MayLi masks it, 2010.07.08++*/
	#if 0
	gpio_tlmm_config( GPIO_CFG(GPIO_USBIN_OVP_N, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA ), GPIO_ENABLE );
	ret = gpio_request(GPIO_USBIN_OVP_N, "gpio_usb_ovp_irq");
	if (ret)
		fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "<ubh> goldfish_battery_probe 10. : IRQ init fails!!!\r\n");

	ret = gpio_direction_input(GPIO_USBIN_OVP_N);
	if (ret)
		fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "<ubh> goldfish_battery_probe 11. : gpio_direction_input fails!!!\r\n");

	ret = request_irq(MSM_GPIO_TO_INT(GPIO_USBIN_OVP_N), &usbovp_irqhandler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, pdev->name, NULL);
	if (ret)
		fih_printk(battery_debug_mask, FIH_DEBUG_ZONE_G0,  "<ubh> goldfish_battery_probe 12. : request_irq fails!!!\r\n");
	else
		enable_irq_wake(MSM_GPIO_TO_INT(GPIO_USBIN_OVP_N));
	#endif
	 /*MayLi masks it, 2010.07.08--*/
	//FIHTDC, Adds for USB OVP detect, MayLi, 2010.05.20 -}

	/*FIHTDC, Fix Can not charging when device power on with charger, 2010.10.08 {+++*/
	if(g_init_charge_gpio==0)
	{
		//Init GPIOs, for setting charging current, MayLi
		gpio_direction_output(GPIO_CHR_IUSB,0 );
		//FIHTDC, add for FN6 charging, MayLi, 2010.06.21 {+
		if(!(g_orig_hwid>=CMCS_FN6_PR1 && g_orig_hwid<=CMCS_FN6_MP1) )
			gpio_direction_output(GPIO_CHR_DCM,0);
		//FIHTDC, add for FN6 charging, MayLi, 2010.06.21 -}
		gpio_direction_output(GPIO_CHR_USBSUS,1);
		gpio_direction_output(GPIO_CHR_nCEN,1);
		gpio_direction_output(GPIO_USB_2ACHG_SEL_n,0);
		g_init_charge_gpio = 1;
	}
	/*FIHTDC, Fix Can not charging when device power on with charger, 2010.10.08 ---}*/

#endif	// FLAG_CHARGER_DETECT
#endif	// T_FIH	///-T_FIH

	return 0;

err_battery_failed:
	kfree(data);
err_data_alloc_failed:
	return ret;
}


static int goldfish_battery_remove(struct platform_device *pdev)
{
	struct goldfish_battery_data *data = platform_get_drvdata(pdev);

#ifdef T_FIH	///+T_FIH
#ifdef FLAG_CHARGER_DETECT
	free_irq(MSM_GPIO_TO_INT(GPIO_CHR_DET), NULL);
	gpio_free(GPIO_CHR_DET);

	free_irq(MSM_GPIO_TO_INT(GPIO_CHR_FLT), NULL);
	gpio_free(GPIO_CHR_FLT);

	 /*MayLi masks it, 2010.07.08++*/
	//free_irq(MSM_GPIO_TO_INT(GPIO_USBIN_OVP_N), NULL);
	//gpio_free(GPIO_USBIN_OVP_N);
	 /*MayLi masks it, 2010.07.08--*/
#endif	// FLAG_CHARGER_DETECT

#ifdef FLAG_BATTERY_POLLING
	del_timer_sync(&polling_timer);
#endif	// FLAG_BATTERY_POLLING
#endif	// T_FIH	///-T_FIH

	power_supply_unregister(&data->battery);
	///power_supply_unregister(&data->ac);

	free_irq(data->irq, data);
	kfree(data);
	battery_data = NULL;
	return 0;
}


//FIHTDC, Adds for update battery information when suspend, MayLi, 2010.05.12 {+
static int battery_resume(struct platform_device *pdev)
{
	printk(KERN_ERR "%s(): battery resume !\n", __func__);
	Battery_power_supply_change();
	return 0;
}

static int battery_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	printk(KERN_ERR "%s(): battery suspend !\n", __func__);
	//Battery_power_supply_change();
	return 0;
}
//FIHTDC, Adds for update battery information when suspend, MayLi, 2010.05.12 -}

/******************Proc operation********************/
//FIHTDC, Add for dump gas gauge registers, MayLi, 2010.06.02 {+
void DumpGasGaugeRegister(void)
{
	struct ds2786_i2c_data value;
	char data=0;
	int i=0;
	
	for(i=0; i<=0x17; i++)
	{
		value.reg = i;
		if(i2c_rx((u8*)&value, 2))
		{
			printk(KERN_ERR "[DS2786]%s(): i2c_rx reg[0x%x] failed!!\n", __func__, i);
		}
		else
		{
			data = value.reg;
			printk(KERN_ERR  "[DS2786]%s(): reg[0x%x]=0x%x\n", __func__, i, data);
		}
	}

	for(i=0x60; i<=0x7F; i++)
	{
		value.reg = i;
		if(i2c_rx((u8*)&value, 2))
		{
			printk(KERN_ERR "[DS2786]%s(): i2c_rx reg[0x%x] failed!!\n", __func__, i);
		}
		else
		{
			data = value.reg;
			printk(KERN_ERR  "[DS2786]%s(): reg[0x%x]=0x%x\n", __func__, i, data);
		}
	}

		value.reg = 0xFE;
		if(i2c_rx((u8*)&value, 2))
		{
			printk(KERN_ERR "[DS2786]%s(): i2c_rx reg[0xFE] failed!!\n", __func__);
		}
		else
		{
			data = value.reg;
			printk(KERN_ERR  "[DS2786]%s(): reg[0xFE]=0x%x\n", __func__, data);
		}

}
//FIHTDC, Add for dump gas gauge registers, MayLi, 2010.06.02 -}


static int ds2786_seq_open(struct inode *inode, struct file *file)
{
  	return single_open(file, NULL, NULL);
}

static ssize_t ds2786_seq_write(struct file *file, const char *buff,
								size_t len, loff_t *off)
{
	char str[64];
	int param = -1;
	int param2 = -1;
	int param3 = -1;
	char cmd[32];

	if(copy_from_user(str, buff, sizeof(str)))
	{
		printk(KERN_ERR "ds2786_seq_write(): copy_from_user failed!\n");
		return -EFAULT;
	}

//	if(sscanf(str, "%s %d", cmd, &param) == -1)
	{
	  	if(sscanf(str, "%s %d %d %d", cmd, &param, &param2, &param3) == -1)
		{
		  	printk("parameter format: <type> <value>\n");
	 		return -EINVAL;
		}
	}

	printk(KERN_INFO "cmd:%s, param:%d, param2:%d, param3:%d\n", cmd, param, param2, param3);

	if(param==1)
		readMilliVolt(&param);
	else if(param==2)
		readCurrent(&param);
	else if(param==3)
		readPercentage(&param);
	else if(param==4)
	{
		percentage_mode = 1;
		manual_percentage = param2;
		printk(KERN_INFO "Set manual battery percentage:%d\n", manual_percentage);
	}
	else if(param==5)
	{
		percentage_mode = 0;
		printk(KERN_INFO "Disable manual percentage!\n");
	}
	else if(param==6)
	{
		//if(mpp_13)
		{
			printk(KERN_INFO "Open green led.\n");
			if(mpp_config_digital_out(mpp_13,MPP_CFG(MPP_DLOGIC_LVL_MSMP,MPP_DLOGIC_OUT_CTRL_LOW)))
				printk(KERN_ERR "Open green led failed!\n");
		}
		//else
			//printk(KERN_ERR "Did not get mpp13\n");
	}
	else if(param==7)
	{
		//if(mpp_13)
		{
			printk(KERN_INFO "Close green led.\n");
			if(mpp_config_digital_out(mpp_13,MPP_CFG(MPP_DLOGIC_LVL_MSMP,MPP_DLOGIC_OUT_CTRL_HIGH)))
				printk(KERN_ERR "Close green led failed!\n");
		}
		//else
			//printk(KERN_ERR "Did not get mpp13\n");
	}
	else if(param==8)
	{
		//if(mpp_15)
		{
			printk(KERN_INFO "Open Red led.\n");
			if(mpp_config_digital_out(mpp_15,MPP_CFG(MPP_DLOGIC_LVL_MSMP,MPP_DLOGIC_OUT_CTRL_LOW)))
				printk(KERN_ERR "Open red led failed!\n");
		}
		//else
			//printk(KERN_ERR "Did not get mpp15\n");
	}
	else if(param==9)
	{
		//if(mpp_15)
		{
			printk(KERN_INFO "Close Red led.\n");
			if(mpp_config_digital_out(mpp_15,MPP_CFG(MPP_DLOGIC_LVL_MSMP,MPP_DLOGIC_OUT_CTRL_HIGH)))
				printk(KERN_ERR "Close red led failed!\n");
		}
		//else
			//printk(KERN_ERR "Did not get mpp15\n");
	}
	else if(param==10)
	{
		printk(KERN_INFO "Disable charging\n");
		gpio_set_value(GPIO_CHR_nCEN, 1 );
	}
	else if(param==11)
	{
		printk(KERN_INFO "Enable charging\n");
		gpio_set_value(GPIO_CHR_nCEN, 0 );
	}
	//FIHTDC, Add for dump gas gauge registers, MayLi, 2010.06.02 {+
	else if(param==12)
	{
		printk(KERN_INFO "dump gas gauge registers\n");
		DumpGasGaugeRegister();
	}
	//FIHTDC, Add for dump gas gauge registers, MayLi, 2010.06.02 -}
	//FIHTDC, Add for testing, MayLi, 2010.07.23 {+
	else if(param==13)
	{
		printk(KERN_INFO "Set POR flag ++++++++++\n");
		SetGasGaugePOR();
		/*Set gas gauge ITEMP to 0, for measuring external BAT_temp, MayLi*/
		printk(KERN_INFO "Set Ex-Temp flag\n");
		SetToInternalTemperature();

		printk(KERN_INFO "Init OCV table\n");
		Init_GasGauge_OCV();

		check_OCV_table();
		printk(KERN_INFO "Check OCV table=%d--------\n", g_OCVstatus);
	}
	else if(param==14)
	{
		printk(KERN_INFO "Set POCV\n");
		SetGasGaugePOCV();
	}
	//FIHTDC, Add for testing, MayLi, 2010.07.23 -}

	return len;
}

static struct file_operations ds2786_seq_fops = 
{
  	.owner 		= THIS_MODULE,
	.open  		= ds2786_seq_open,
	.write 		= ds2786_seq_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int ds2786_create_proc(void)
{
  	ds2786_proc_file = create_proc_entry("driver/ds2786", 0666, NULL);
	
	if(!ds2786_proc_file){
	  	printk(KERN_ERR "create proc file for ds2786 failed\n");
		return -ENOMEM;
	}

	printk(KERN_INFO "ds2786 proc ok\n");
	ds2786_proc_file->proc_fops = &ds2786_seq_fops;
	return 0;
}

static int ds2786_probe(struct i2c_client *client,const struct i2c_device_id *device_id)
{
  	int ret = 0;

	bat_ds2786_i2c = client;
	//printk( KERN_INFO "[Neo]Driver probe in Zeus_battery: %s\n", __func__ );

	return ret;
}

static int ds2786_remove(struct i2c_client *client)
{
	int ret = 0;
	
	return ret;
}

static const struct i2c_device_id i2c_ds2786_idtable[] = {
       { i2c_ds2786_name, 0 },
       { }
};

static struct i2c_driver ds2786_driver = {
	.driver = {
		.name	= "ds2786",
	},
	.probe		= ds2786_probe,
	.remove		= ds2786_remove,
	.id_table = i2c_ds2786_idtable,
};

static struct platform_driver goldfish_battery_device = {
	.probe		= goldfish_battery_probe,
	.remove		= goldfish_battery_remove,
	.suspend        = battery_suspend,
	.resume         = battery_resume,
	.driver = {
		.name = "goldfish-battery"
	}
};

static int __init goldfish_battery_init(void)
{
	int ret;

	// i2c
	ret = i2c_add_driver(&ds2786_driver);
	if (ret) {
		printk(KERN_ERR "%s: [Neo]Driver registration failed, module not inserted.\n", __func__);
		goto driver_del;
	}

	// create proc
	ret = ds2786_create_proc();
	if(ret) {
	  	printk(KERN_ERR "%s: create proc file failed\n", __func__);
	}

	//FIHTDC, add for FN6 HWID, MayLi, 2010.06.21 {+
	g_orig_hwid = fih_read_orig_hwid_from_smem();
	printk(KERN_ERR "%s: g_orig_hwid=0x%x\n", __func__, g_orig_hwid);
	//FIHTDC, add for FN6 HWID, MayLi, 2010.06.21 -}
	//Green LED
	#if 0
	mpp_13 = mpp_get(NULL, "mpp13");
	if(!mpp_13)  
		printk(KERN_ERR "Can not get mpp13\n");
	//Red LED
	mpp_15 = mpp_get(NULL, "mpp15");
	if(!mpp_15)  
		printk(KERN_ERR "Can not get mpp15\n");
	//Red LED control
	mpp_16 = mpp_get(NULL, "mpp16");
	if(!mpp_16)  
		printk(KERN_ERR "Can not get mpp16\n");
	else
	#endif
	{
		//printk(KERN_INFO "Set mpp16, red control to HIGH.\n");
		if(mpp_config_digital_out(mpp_16,MPP_CFG(MPP_DLOGIC_LVL_MSMP,MPP_DLOGIC_OUT_CTRL_HIGH)))
			printk(KERN_ERR "Set mpp16, red control to High failed!\n");
	}

	ret = platform_driver_register(&goldfish_battery_device);
	if(ret)
	{
		printk(KERN_ERR "%s: register battery device failed.\n", __func__);
		goto ERROR;
	}
	
	/* FIH, Michael Kao, 2010/01/03{ */
	/* [FXX_CR], add for debug mask*/
	battery_debug_mask = *(int*)BAT_DEBUG_MASK_OFFSET;
	/* FIH, Michael Kao, 2010/01/03{ */

	//FIHTDC, Adds for FM6 gas gauge, NeoChen, 2010.03.25 +
	check_OCV_table();
	readInitVolt();
	//FIHTDC, Adds for FM6 gas gauge, NeoChen, 2010.03.25 -

	/*Set gas gauge ITEMP to 0, for measuring external BAT_temp, MayLi*/
	SetToInternalTemperature();

	return ret;
	
	driver_del:
	i2c_del_driver(&ds2786_driver);
	
	ERROR:    
	return ret;
}

static void __exit goldfish_battery_exit(void)
{	
	platform_driver_unregister(&goldfish_battery_device);
}

module_init(goldfish_battery_init);
module_exit(goldfish_battery_exit);

MODULE_AUTHOR("Mike Lockwood lockwood@android.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Battery driver for the Goldfish emulator");
