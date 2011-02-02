/*
 *     mt9t112.c - Camera Sensor Driver
 *
 *     Copyright (C) 2009 Charles YS Huang <charlesyshuang@fihtdc.com>
 *     Copyright (C) 2008 FIH CO., Inc.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; version 2 of the License.
 */


#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include "mt9t112.h"

#include <mach/msm_iomap.h>
#include <mach/msm_smd.h>
/* FIH, Charles Huang, 2010/03/01 { */
/* [FXX_CR], Add power onoff vreg */
#ifdef CONFIG_FIH_FXX
#include <mach/vreg.h>
#endif
/* } FIH, Charles Huang, 2010/03/01 */
/* FIH, IvanCHTang, 2010/8/16 { */
/* [FXX_CR], [Camera] Add the SW standby for VCM WV511 - [FM6#0014] */
#ifdef CONFIG_FIH_FXX
#define EN_WV511_STANDBY 1
unsigned int mt9t112_otp_id = 0x0;
unsigned int mt9t112_otp_is_Read = false;
#endif
/* } FIH, IvanCHTang, 2010/8/16 */
/* FIH, IvanCHTang, 2010/6/30 { */
/* [FXX_CR], [camera] DVDD (msme2 -> mpp1) & VGA_PD (mpp22 -> gpio<108>) for FN6 PR2 - [FM6#0010] */
#ifdef CONFIG_FIH_FXX
#include <mach/mpp.h>
static unsigned mpp_1 = 0;
#endif
/* } FIH, IvanCHTang, 2010/6/30 */
/* FIH, IvanCHTang, 2010/5/28 { */
/* [FXX_CR], (FM6.B-591) [Stability][Camera]Camera ANR when enter camera then suspend mode quickly. - [FM6#0002] */
#define WAKELOCK_SUSPEND 1 // Enable if "1"
#if WAKELOCK_SUSPEND
static struct wake_lock mt9t112_wake_lock_suspend;
#endif // [end] #if WAKELOCK_SUSPEND
/* } FIH, IvanCHTang, 2010/5/28 */
/* FIH, Charles Huang, 2010/02/26 { */
/* [FXX_CR], move to kthread and speed up launching camera app */
#ifdef CONFIG_FIH_FXX
DECLARE_COMPLETION(mt9t112_firmware_init);
int mt9t112_af_finish = 0;
#endif
/* } FIH, Charles Huang, 2010/02/26 */

DEFINE_MUTEX(mt9t112_mut);

/* Micron MT9T112 Registers and their values */
/* Sensor Core Registers */
#define  REG_MT9T112_MODEL_ID 0x0000
#define  MT9T112_MODEL_ID     0x2682
#define  MT9T112_I2C_READ_SLAVE_ID     0x7A >> 1  
#define  MT9T112_I2C_WRITE_SLAVE_ID     0x7B >> 1  
/* FIH, Charles Huang, 2009/07/29 { */
/* [FXX_CR], Calculate AEC */
#ifdef CONFIG_FIH_FXX
#define MT9T112_CAPTURE_FRAMERATE 7.5
#define MT9T112_PREVIEW_FRAMERATE 30
int mt9t112_m_60Hz=0;
#endif
/* } FIH, Charles Huang, 2009/07/29 */

struct mt9t112_work {
	struct work_struct work;
};

static struct  mt9t112_work *mt9t112_sensorw;
static struct  i2c_client *mt9t112_client;

struct mt9t112_ctrl {
	const struct msm_camera_sensor_info *sensordata;
};

extern void brightness_onoff(int on);
static struct mt9t112_ctrl *mt9t112_ctrl;

/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#define MT9T112_USE_VFS
#ifdef MT9T112_USE_VFS
#define MT9T112_INITREG "initreg"
#define MT9T112_OEMREG "oemreg"
#define MT9T112_PREVIEWREG "previewreg"
#define MT9T112_SNAPREG "snapreg"
#define MT9T112_SNAPAEREG "snapaereg"
#define MT9T112_IQREG "iqreg"
#define MT9T112_LENSREG "lensreg"
#define MT9T112_WRITEREG "writereg"
#define MT9T112_GETREG "getreg"
#define MT9T112_MCLK "mclk"
#define MT9T112_MULTIPLE "multiple"
#define MT9T112_FLASHTIME "flashtime"

/* MAX buf is ???? */
#define MT9T112_MAX_VFS_INIT_INDEX 330
int mt9t112_use_vfs_init_setting=0;
struct mt9t112_i2c_reg_conf mt9t112_vfs_init_settings_tbl[MT9T112_MAX_VFS_INIT_INDEX];
uint16_t mt9t112_vfs_init_settings_tbl_size= ARRAY_SIZE(mt9t112_vfs_init_settings_tbl);

#define MT9T112_MAX_VFS_OEM_INDEX 330
int mt9t112_use_vfs_oem_setting=0;
struct mt9t112_i2c_reg_conf mt9t112_vfs_oem_settings_tbl[MT9T112_MAX_VFS_OEM_INDEX];
uint16_t mt9t112_vfs_oem_settings_tbl_size= ARRAY_SIZE(mt9t112_vfs_oem_settings_tbl);

#define MT9T112_MAX_VFS_PREVIEW_INDEX 330
int mt9t112_use_vfs_preview_setting=0;
struct mt9t112_i2c_reg_conf mt9t112_vfs_preview_settings_tbl[MT9T112_MAX_VFS_PREVIEW_INDEX];
uint16_t mt9t112_vfs_preview_settings_tbl_size= ARRAY_SIZE(mt9t112_vfs_preview_settings_tbl);

#define MT9T112_MAX_VFS_SNAP_INDEX 330
int mt9t112_use_vfs_snap_setting=0;
struct mt9t112_i2c_reg_conf mt9t112_vfs_snap_settings_tbl[MT9T112_MAX_VFS_SNAP_INDEX];
uint16_t mt9t112_vfs_snap_settings_tbl_size= ARRAY_SIZE(mt9t112_vfs_snap_settings_tbl);

#define MT9T112_MAX_VFS_SNAPAE_INDEX 330
int mt9t112_use_vfs_snapae_setting=0;
struct mt9t112_i2c_reg_conf mt9t112_vfs_snapae_settings_tbl[MT9T112_MAX_VFS_SNAPAE_INDEX];
uint16_t mt9t112_vfs_snapae_settings_tbl_size= ARRAY_SIZE(mt9t112_vfs_snapae_settings_tbl);

#define MT9T112_MAX_VFS_IQ_INDEX 330
int mt9t112_use_vfs_iq_setting=0;
struct mt9t112_i2c_reg_conf mt9t112_vfs_iq_settings_tbl[MT9T112_MAX_VFS_IQ_INDEX];
uint16_t mt9t112_vfs_iq_settings_tbl_size= ARRAY_SIZE(mt9t112_vfs_iq_settings_tbl);

#define MT9T112_MAX_VFS_LENS_INDEX 330
int mt9t112_use_vfs_lens_setting=0;
struct mt9t112_i2c_reg_conf mt9t112_vfs_lens_settings_tbl[MT9T112_MAX_VFS_LENS_INDEX];
uint16_t mt9t112_vfs_lens_settings_tbl_size= ARRAY_SIZE(mt9t112_vfs_lens_settings_tbl);

#define MT9T112_MAX_VFS_WRITEREG_INDEX 330
int mt9t112_use_vfs_writereg_setting=0;
struct mt9t112_i2c_reg_conf mt9t112_vfs_writereg_settings_tbl[MT9T112_MAX_VFS_IQ_INDEX];
uint16_t mt9t112_vfs_writereg_settings_tbl_size= ARRAY_SIZE(mt9t112_vfs_writereg_settings_tbl);

#define MT9T112_MAX_VFS_GETREG_INDEX 330
int mt9t112_use_vfs_getreg_setting=0;
struct mt9t112_i2c_reg_conf mt9t112_vfs_getreg_settings_tbl[MT9T112_MAX_VFS_GETREG_INDEX];
uint16_t mt9t112_vfs_getreg_settings_tbl_size= ARRAY_SIZE(mt9t112_vfs_getreg_settings_tbl);

int mt9t112_use_vfs_mclk_setting=0;
int mt9t112_use_vfs_multiple_setting=0;
int mt9t112_use_vfs_flashtime_setting=0;
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */

static DECLARE_WAIT_QUEUE_HEAD(mt9t112_wait_queue);
DECLARE_MUTEX(mt9t112_sem);

/*=============================================================*/
static int mt9t112_reset(const struct msm_camera_sensor_info *dev)
{
	int rc = 0;
	int HWID=FIH_READ_HWID_FROM_SMEM();
	gpio_tlmm_config(GPIO_CFG(0, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
	gpio_tlmm_config(GPIO_CFG(85, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		;
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		gpio_tlmm_config(GPIO_CFG(122, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
	}
	/* } FIH, IvanCHTang, 2010/6/1 */

	/* Switch enable */
	gpio_direction_output(85, 0);	
	msleep(10);
	rc = gpio_request(dev->sensor_pwd, "mt9t112");
	if (!rc) {
		rc = gpio_direction_output(dev->sensor_pwd, 0);
	}
	gpio_free(dev->sensor_pwd);
	
	rc = gpio_request(dev->sensor_reset, "mt9t112");
	if (!rc) {
		rc = gpio_direction_output(dev->sensor_reset, 0);
		msleep(20);
		rc = gpio_direction_output(dev->sensor_reset, 1);
	}
	gpio_free(dev->sensor_reset);
	
	CDBG("[MT9T112 3M]  gpio_get_value(0) = %d\n", gpio_get_value(0));
	CDBG("[MT9T112 3M]  gpio_get_value(31) = %d\n", gpio_get_value(31));
	CDBG("[MT9T112 3M]  gpio_get_value(85) = %d\n", gpio_get_value(85));
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		;
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		CDBG("[MT9T112 3M]  gpio_get_value(122) = %d\n", gpio_get_value(122));
	}
	/* } FIH, IvanCHTang, 2010/6/1 */
	   
	return rc;
}

static int32_t mt9t112_i2c_txdata(unsigned short saddr,
	unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
			.addr = saddr,
			.flags = 0,
			.len = length,
			.buf = txdata,
		},
	};

	//Try 10 times
	#if 1
	int i = 0;
	for(i = 1; i <= 10; i++)
	{
		if (i2c_transfer(mt9t112_client->adapter, msg, 1) < 0)
		{
			printk(KERN_ERR "mt9t112_i2c_txdata failed, try %d \n", i);
			continue;
		}
		else
			return 0;
	}
	printk(KERN_ERR "mt9t112_i2c_txdata failed\n");
	return -EIO;
	#else //original
	if (i2c_transfer(mt9t112_client->adapter, msg, 1) < 0) {
		printk(KERN_ERR "mt9t112_i2c_txdata failed, try again!\n");
		if (i2c_transfer(mt9t112_client->adapter, msg, 1) < 0) {
			printk(KERN_ERR "mt9t112_i2c_txdata failed\n");
			CDBG("mt9t112_i2c_txdata failed\n");
			return -EIO;
		}
	}

	return 0;
	#endif
}

static int32_t mt9t112_i2c_write(unsigned short saddr,
	unsigned short waddr, unsigned short wdata, enum mt9t112_width width)
{
	int32_t rc = -EFAULT;
	unsigned char buf[4];

	memset(buf, 0, sizeof(buf));
	switch (width) {
	case WORD_LEN: {
		buf[0] = (waddr & 0xFF00)>>8;
		buf[1] = (waddr & 0x00FF);
		buf[2] = (wdata & 0xFF00)>>8;
		buf[3] = (wdata & 0x00FF);

		rc = mt9t112_i2c_txdata(saddr, buf, 4);
	}
		break;

	case BYTE_LEN: {
		buf[0] = (waddr & 0xFF00)>>8;
		buf[1] = (waddr & 0x00FF);
		buf[2] = (uint8_t) wdata;
		rc = mt9t112_i2c_txdata(saddr, buf, 3);
	}
		break;

	default:
		break;
	}

	if (rc < 0)
		printk(KERN_ERR "i2c_write failed, addr = 0x%x, val = 0x%x!\n",
		waddr, wdata);

	return rc;
}

static int32_t mt9t112_i2c_rxdata(unsigned short saddr,
	unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
	{
		.addr   = saddr,
		.flags = 0,
		.len   = 2,
		.buf   = rxdata,
	},
	{
		.addr   = saddr,
		.flags = I2C_M_RD,
		.len   = length,
		.buf   = rxdata,
	},
	};

	if (i2c_transfer(mt9t112_client->adapter, msgs, 2) < 0) {
		printk(KERN_ERR "mt9t112_i2c_rxdata failed, try again!\n");
		if (i2c_transfer(mt9t112_client->adapter, msgs, 2) < 0) {
			printk(KERN_ERR "mt9t112_i2c_rxdata failed!\n");
			CDBG("mt9t112_i2c_rxdata failed!\n");
			return -EIO;
		}
	}

	return 0;
}

static int32_t mt9t112_i2c_read(unsigned short   saddr,
	unsigned short raddr, unsigned short *rdata, enum mt9t112_width width)
{
	int32_t rc = 0;
	unsigned char buf[4];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));

	switch (width) {
	case WORD_LEN: {
		buf[0] = (raddr & 0xFF00)>>8;
		buf[1] = (raddr & 0x00FF);

		rc = mt9t112_i2c_rxdata(saddr, buf, 2);
		if (rc < 0)
			return rc;

		*rdata = buf[0] << 8 | buf[1];
	}
		break;
	case BYTE_LEN: {
		buf[0] = (raddr & 0xFF00)>>8;
		buf[1] = (raddr & 0x00FF);

		rc = mt9t112_i2c_rxdata(saddr, buf, 1);
		if (rc < 0)
			return rc;

		*rdata = buf[0];
	}
		break;

	default:
		break;
	}

	if (rc < 0)
		CDBG("mt9t112_i2c_read failed!\n");

	return rc;
}

void mt9t112_set_value_by_bitmask(uint16_t bitset, uint16_t mask, uint16_t  *new_value)
{
	uint16_t org;

	org= *new_value;
	*new_value = (org&~mask) | (bitset & mask);
}

static int32_t mt9t112_i2c_write_table(
	struct mt9t112_i2c_reg_conf const *reg_conf_tbl,
	int num_of_items_in_table)
{
	int i;
	int32_t rc = 0;

	for (i = 0; i < num_of_items_in_table; i++) {
		/* illegal addr */
		if (reg_conf_tbl->waddr== 0xFFFF)
			break;
		if (reg_conf_tbl->mask == 0xFFFF)
		{
			rc = mt9t112_i2c_write(MT9T112_I2C_WRITE_SLAVE_ID,
				reg_conf_tbl->waddr, reg_conf_tbl->wdata,
				reg_conf_tbl->width);
		}else{
			uint16_t reg_value = 0;
			rc = mt9t112_i2c_read(MT9T112_I2C_READ_SLAVE_ID,
				reg_conf_tbl->waddr, &reg_value, WORD_LEN);
			mt9t112_set_value_by_bitmask(reg_conf_tbl->wdata,reg_conf_tbl->mask,&reg_value);
			rc = mt9t112_i2c_write(MT9T112_I2C_WRITE_SLAVE_ID,
					reg_conf_tbl->waddr, reg_value, WORD_LEN);
		}
		
		if (rc < 0)
			break;
		if (reg_conf_tbl->mdelay_time != 0)
			msleep(reg_conf_tbl->mdelay_time);
		reg_conf_tbl++;
	}

	return rc;
}

#if 0
static int32_t mt9t112_set_lens_roll_off(void)
{
	int32_t rc = 0;
	rc = mt9t112_i2c_write_table(&mt9t112_regs.rftbl[0],
		mt9t112_regs.rftbl_size);
	return rc;
}
#endif

/************************
* MT9T112 setting sequence
* 1. [PLL]
* 2. [Timing]
* 3. [PATCH]
* 4. [LSC]
* 5. [AWB & CCM]
* 6. [GAMMA]
* 7. [AF]
* 8. [AE]
* 9. [Others]
************************/
static long mt9t112_reg_init(void)
{
#if 0
	int32_t i;
#endif
	long rc;
#if 0
	/* PLL Setup Start */
	rc = mt9t112_i2c_write_table(&mt9d112_regs.plltbl[0],
		mt9t112_regs.plltbl_size);

	if (rc < 0)
		return rc;
	/* PLL Setup End   */
#endif
printk(KERN_DEBUG "%s++ \r\n", __func__);
	/* Configure sensor for Preview mode and Snapshot mode */
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef MT9T112_USE_VFS
	if (mt9t112_use_vfs_init_setting)
		rc = mt9t112_i2c_write_table(&mt9t112_vfs_init_settings_tbl[0],
			mt9t112_vfs_init_settings_tbl_size);
	else
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */
		rc = mt9t112_i2c_write_table(&mt9t112_regs.init_settings_tbl[0],
			mt9t112_regs.init_settings_tbl_size);

	if (rc < 0)
		return rc;
/* FIH, Charles Huang, 2010/02/26 { */
/* [FXX_CR], move to kthread and speed up launching camera app */
#ifndef CONFIG_FIH_FXX
	rc = mt9t112_i2c_write_table(&mt9t112_regs.af_init_settings_tbl[0],
		mt9t112_regs.af_init_settings_tbl_size);

	if (rc < 0)
		return rc;
#endif
/* } FIH, Charles Huang, 2010/02/26 */

	/* Configure sensor for image quality settings */
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef MT9T112_USE_VFS
	if (mt9t112_use_vfs_iq_setting)
		rc = mt9t112_i2c_write_table(&mt9t112_vfs_iq_settings_tbl[0],
			mt9t112_vfs_iq_settings_tbl_size);
	else
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */
//	rc = mt9t112_i2c_write_table(&mt9t112_regs.iq_settings_tbl[0],
//		mt9t112_regs.iq_settings_tbl_size);


	if (rc < 0)
		return rc;

	/* Configure sensor for customer settings */
#if 0
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef MT9T112_USE_VFS
	if (mt9t112_use_vfs_oem_setting)
		rc = mt9t112_i2c_write_table(&vfs_oem_settings_tbl[0],
			vfs_oem_settings_tbl_size);
	else
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */
	rc = mt9t112_i2c_write_table(&mt9t112_regs.oem_settings_tbl[0],
		mt9t112_regs.oem_settings_tbl_size);

	if (rc < 0)
		return rc;
#endif
	// 20100623, remove by Ivan++
	//msleep(50);
	// 20100623, remove by Ivan--
/* FIH, Charles Huang, 2010/02/26 { */
/* [FXX_CR], move to kthread and speed up launching camera app */
#ifdef CONFIG_FIH_FXX
// 20100910, remove by Ivan - Don't trigger the thread to send the other setting
//	complete(&mt9t112_firmware_init);
#endif
/* } FIH, Charles Huang, 2010/02/26 */
	// 20100623, remove by Ivan++
	//msleep(50);
	// 20100623, remove by Ivan--
printk(KERN_DEBUG "%s-- \r\n", __func__);
#if 0
	/* Configure for Noise Reduction, Saturation and Aperture Correction */
	array_length = mt9t112_regs.noise_reduction_reg_settings_;

	for (i = 0; i < array_length; i++) {

		rc = mt9t112_i2c_write(mt9t112_client->addr,
		mt9t112_regs.noise_reduction_reg_settings[i].register_address,
		mt9t112_regs.noise_reduction_reg_settings[i].register_value,
			WORD_LEN);

		if (rc < 0)
			return rc;
	}

	/* Set Color Kill Saturation point to optimum value */
	rc =
	mt9t112_i2c_write(mt9t112_client->addr,
	0x35A4,
	0x0593,
	WORD_LEN);
	if (rc < 0)
		return rc;

	rc = mt9t112_i2c_write_table(&mt9t112_regs.stbl[0],
		mt9t112_regs.stbl_size);
	if (rc < 0)
		return rc;

	rc = mt9t112_set_lens_roll_off();
	if (rc < 0)
		return rc;
#endif

	return 0;
}

static long mt9t112_reg_init2(void)
{
	int rc = 0;
	printk(KERN_DEBUG "%s++ \r\n", __func__);
	// AF driver IC is WV511 if OTP id is 0x511
	if(mt9t112_otp_id == 0x0511)
	{
		// Patch for WV511
		rc = mt9t112_i2c_write_table(&mt9t112_regs.af_WV511_patch_settings_tbl[0],
									mt9t112_regs.af_WV511_patch_settings_tbl_size);
		if (rc < 0)
			return rc;
		//Use the LSC setting calibrated by MCNEX
		rc = mt9t112_i2c_write_table(&mt9t112_regs.oem_lsc_settings_tbl[0],
									mt9t112_regs.oem_lsc_settings_tbl_size);
		if (rc < 0)
			return rc;
		// Image Quality: [AWB & CCM], [GAMMA], [AE]
		rc = mt9t112_i2c_write_table(&mt9t112_regs.iq_settings_tbl[0],
									mt9t112_regs.iq_settings_tbl_size);
		if (rc < 0)
			return rc;
		// VCM WV511 setting
		rc = mt9t112_i2c_write_table(&mt9t112_regs.af_WV511_init_settings_tbl[0],
									mt9t112_regs.af_WV511_init_settings_tbl_size);
		if (rc < 0)
			return rc;
	}
	else
	{ 
		// Patch for A3904
		rc = mt9t112_i2c_write_table(&mt9t112_regs.af_patch_settings_tbl[0],
									mt9t112_regs.af_patch_settings_tbl_size);
		if (rc < 0)
			return rc;
		//Use the LSC setting calibrated by FIH optical
		rc = mt9t112_i2c_write_table(&mt9t112_regs.fih_lsc_settings_tbl[0],
									mt9t112_regs.fih_lsc_settings_tbl_size);
		if (rc < 0)
			return rc;
		// Image Quality: [AWB & CCM], [GAMMA], [AE]
		rc = mt9t112_i2c_write_table(&mt9t112_regs.iq_settings_tbl[0],
									mt9t112_regs.iq_settings_tbl_size);
		if (rc < 0)
			return rc;
		// VCM A3904 setting
		rc = mt9t112_i2c_write_table(&mt9t112_regs.af_init_settings_tbl[0],
									mt9t112_regs.af_init_settings_tbl_size);
	}

	// Other setting
	rc = mt9t112_i2c_write_table(&mt9t112_regs.other_settings_tbl[0],
									mt9t112_regs.other_settings_tbl_size);
	if (rc < 0)
		return rc;

	// sleep 100 msec
	msleep(100);
	mt9t112_af_finish = 1;
	printk(KERN_DEBUG "%s-- \r\n", __func__);
	return 0;
}

static long mt9t112_set_sensor_mode(int mode)
{
	long rc = 0;

	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		/* Configure sensor for Preview mode */
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef MT9T112_USE_VFS
		if (mt9t112_use_vfs_preview_setting)
			rc = mt9t112_i2c_write_table(&mt9t112_vfs_preview_settings_tbl[0],
				mt9t112_vfs_preview_settings_tbl_size);
		else
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */
			rc = mt9t112_i2c_write_table(&mt9t112_regs.preview_settings_tbl[0],
				mt9t112_regs.preview_settings_tbl_size);

		if (rc < 0)
			return rc;

	/* Configure sensor for customer settings */
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef MT9T112_USE_VFS
		if (mt9t112_use_vfs_oem_setting)
			rc = mt9t112_i2c_write_table(&mt9t112_vfs_oem_settings_tbl[0],
				mt9t112_vfs_oem_settings_tbl_size);
		else
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */
			rc = mt9t112_i2c_write_table(&mt9t112_regs.oem_settings_tbl[0],
				mt9t112_regs.oem_settings_tbl_size);

		if (rc < 0)
			return rc;

		msleep(5);
		break;

	case SENSOR_SNAPSHOT_MODE:

		//change resolution from VGA to QXSGA here
		/* Configure sensor for Snapshot mode */
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef MT9T112_USE_VFS
		if (mt9t112_use_vfs_snap_setting)
			rc = mt9t112_i2c_write_table(&mt9t112_vfs_snap_settings_tbl[0],
				mt9t112_vfs_snap_settings_tbl_size);
		else
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */
			rc = mt9t112_i2c_write_table(&mt9t112_regs.snapshot_settings_tbl[0],
				mt9t112_regs.snapshot_settings_tbl_size);

		if (rc < 0)
			return rc;

		msleep(5);


		break;

	default:
		return -EFAULT;
	}

	return 0;
}

static long mt9t112_set_effect(int mode, int effect)
{
	long rc = 0;

	CDBG("mt9t112_set_effect, mode = %d, effect = %d\n",
		mode, effect);

		printk(KERN_DEBUG "mt9t112_set_effect, mode = %d, effect = %d\n",
		mode, effect);
	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		/* Context A Special Effects */
		break;

	case SENSOR_SNAPSHOT_MODE:
		/* Context B Special Effects */
		break;

	default:
		break;
	}

	switch (effect) {
	case CAMERA_EFFECT_OFF: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.effect_none_settings_tbl[0],
				mt9t112_regs.effect_none_settings_tbl_size);

		if (rc < 0)
			return rc;
	}
			break;

	case CAMERA_EFFECT_MONO: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.effect_mono_settings_tbl[0],
				mt9t112_regs.effect_mono_settings_tbl_size);

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_EFFECT_NEGATIVE: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.effect_negative_settings_tbl[0],
				mt9t112_regs.effect_negative_settings_tbl_size);

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_EFFECT_SEPIA: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.effect_sepia_settings_tbl[0],
				mt9t112_regs.effect_sepia_settings_tbl_size);

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_EFFECT_BLUISH: {//Bluish
		struct mt9t112_i2c_reg_conf const mt9t112_effect_bluish_tbl[] = {
		};

		rc = mt9t112_i2c_write_table(&mt9t112_effect_bluish_tbl[0],
				ARRAY_SIZE(mt9t112_effect_bluish_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_EFFECT_REDDISH: {//Reddish
		struct mt9t112_i2c_reg_conf const mt9t112_effect_reddish_tbl[] = {
		};

		rc = mt9t112_i2c_write_table(&mt9t112_effect_reddish_tbl[0],
				ARRAY_SIZE(mt9t112_effect_reddish_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_EFFECT_GREENISH: {//Greenish
		struct mt9t112_i2c_reg_conf const mt9t112_effect_greenish_tbl[] = {
		};

		rc = mt9t112_i2c_write_table(&mt9t112_effect_greenish_tbl[0],
				ARRAY_SIZE(mt9t112_effect_greenish_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	default: {
		if (rc < 0)
			return rc;

		/*return -EFAULT;*/
		/* -EFAULT makes app fail */
		return 0;
	}
	}

	return rc;
}

static long mt9t112_set_wb(int mode, int wb)
{
	long rc = 0;

	CDBG("mt9t112_set_wb, mode = %d, wb = %d\n",
		mode, wb);

		printk(KERN_DEBUG "mt9t112_set_wb, mode = %d, wb = %d\n",
		mode, wb);

	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		/* Context A Special Effects */
		break;

	case SENSOR_SNAPSHOT_MODE:
		/* Context B Special Effects */
		break;

	default:
		break;
	}

	switch (wb) {
	case CAMERA_WB_AUTO: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.wb_auto_settings_tbl[0],
				mt9t112_regs.wb_auto_settings_tbl_size);

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_WB_INCANDESCENT: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.wb_incandescent_settings_tbl[0],
				mt9t112_regs.wb_incandescent_settings_tbl_size);

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_WB_FLUORESCENT: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.wb_fluorescent_settings_tbl[0],
				mt9t112_regs.wb_fluorescent_settings_tbl_size);

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_WB_DAYLIGHT: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.wb_daylight_settings_tbl[0],
				mt9t112_regs.wb_daylight_settings_tbl_size);

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_WB_CLOUDY_DAYLIGHT: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.wb_cloudy_settings_tbl[0],
				mt9t112_regs.wb_cloudy_settings_tbl_size);

		if (rc < 0)
			return rc;
	}
		break;

	default: {
		if (rc < 0)
			return rc;

		/*return -EFAULT;*/
		/* -EFAULT makes app fail */
		return 0;
	}
	}

	return rc;
}


static long mt9t112_set_brightness(int mode, int brightness)
{
	long rc = 0;

	CDBG("mt9t112_set_brightness, mode = %d, brightness = %d\n",
		mode, brightness);

		printk(KERN_DEBUG "mt9t112_set_brightness, mode = %d, brightness = %d\n",
		mode, brightness);
	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		/* Context A Special Effects */
		break;

	case SENSOR_SNAPSHOT_MODE:
		/* Context B Special Effects */
		break;

	default:
		break;
	}

	/* FIH, IvanCHTang, 2010/5/17 { */
	/* [FXX_CR], [Camera]  Brightness setting implemention */
	/* } FIH, IvanCHTang, 2010/5/17 */
	switch (brightness) {
	case CAMERA_BRIGHTNESS_0: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.brightness_0_settings_tbl[0],
				mt9t112_regs.brightness_0_settings_tbl_size);

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_BRIGHTNESS_1: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.brightness_1_settings_tbl[0],
				mt9t112_regs.brightness_1_settings_tbl_size);

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_BRIGHTNESS_2: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.brightness_2_settings_tbl[0],
				mt9t112_regs.brightness_2_settings_tbl_size);

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_BRIGHTNESS_3: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.brightness_3_settings_tbl[0],
				mt9t112_regs.brightness_3_settings_tbl_size);

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_BRIGHTNESS_4: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.brightness_4_settings_tbl[0],
				mt9t112_regs.brightness_4_settings_tbl_size);

		if (rc < 0)
			return rc;
	} 
		break;

	case CAMERA_BRIGHTNESS_5: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.brightness_5_settings_tbl[0],
				mt9t112_regs.brightness_5_settings_tbl_size);

		if (rc < 0)
			return rc;
	} 
		break;

	case CAMERA_BRIGHTNESS_6: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.brightness_6_settings_tbl[0],
				mt9t112_regs.brightness_6_settings_tbl_size);

		if (rc < 0)
			return rc;
	} 
		break;

	case CAMERA_BRIGHTNESS_7:
	case CAMERA_BRIGHTNESS_8:
	case CAMERA_BRIGHTNESS_9:
	case CAMERA_BRIGHTNESS_10: {
		struct mt9t112_i2c_reg_conf const mt9t112_brightness_10_tbl[] = {

		};

		rc = mt9t112_i2c_write_table(&mt9t112_brightness_10_tbl[0],
				ARRAY_SIZE(mt9t112_brightness_10_tbl));

		if (rc < 0)
			return rc;
	} 
		break;

	default: {
		if (rc < 0)
			return rc;

		/*return -EFAULT;*/
		/* -EFAULT makes app fail */
		return 0;
	}
	}

	return rc;
}

/* FIH, IvanCHTang, 2010/6/8 { */
/* [FXX_CR], Add camera features for FM6 - [FM6#0004] */
/* Sharpness Funciton */
static long mt9t112_set_sharpness(int mode, int sharpness)
{
	long rc = 0;

	CDBG("mt9t112_set_sharpness, mode = %d, sharpness = %d\n",
		mode, sharpness);

		printk(KERN_DEBUG "mt9t112_set_sharpness, mode = %d, sharpness = %d\n",
		mode, sharpness);
	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		/* Context A Special Effects */
		break;

	case SENSOR_SNAPSHOT_MODE:
		/* Context B Special Effects */
		break;

	default:
		break;
	}

	switch (sharpness) {
	case CAMERA_SHARPNESS_ZERO: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.sharpness_0_settings_tbl[0],
									mt9t112_regs.sharpness_0_settings_tbl_size);

		if (rc < 0)
			return rc;

	}
		break;

	case CAMERA_SHARPNESS_POSITIVE_1: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.sharpness_1_settings_tbl[0],
									mt9t112_regs.sharpness_1_settings_tbl_size);

		if (rc < 0)
			return rc;

	}
		break;

	case CAMERA_SHARPNESS_POSITIVE_2: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.sharpness_2_settings_tbl[0],
									mt9t112_regs.sharpness_2_settings_tbl_size);

		if (rc < 0)
			return rc;

	}
		break;
	// Not ready++
	#if (0)
	case CAMERA_SHARPNESS_POSITIVE_3: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.sharpness_3_settings_tbl[0],
								mt9t112_regs.sharpness_3_settings_tbl_size);

		if (rc < 0)
			return rc;

	}
		break;

	case CAMERA_SHARPNESS_POSITIVE_4: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.sharpness_4_settings_tbl[0],
								mt9t112_regs.sharpness_4_settings_tbl_size);

		if (rc < 0)
			return rc;

	}
		break;
	#endif
	// Not ready--
	default: {
		if (rc < 0)
			return rc;

		return -EFAULT;
	}
	}

	return rc;
}

/* Contrast Function */
static long mt9t112_set_contrast(int mode, int contrast)
{
	long rc = 0;

	CDBG("mt9t112_set_contrast, mode = %d, contrast = %d\n",
		mode, contrast);

		printk(KERN_DEBUG "mt9t112_set_contrast, mode = %d, contrast = %d\n",
		mode, contrast);

	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		/* Context A Special Effects */
		break;

	case SENSOR_SNAPSHOT_MODE:
		/* Context B Special Effects */
		break;

	default:
		break;
	}

	switch (contrast) {
	case CAMERA_CONTRAST_MINUS_2: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.contrast_0_settings_tbl[0],
				mt9t112_regs.contrast_0_settings_tbl_size);

		if (rc < 0)
			return rc;

	}
			break;

	case CAMERA_CONTRAST_MINUS_1: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.contrast_1_settings_tbl[0],
				mt9t112_regs.contrast_1_settings_tbl_size);

		if (rc < 0)
			return rc;

	}
		break;

	case CAMERA_CONTRAST_ZERO: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.contrast_2_settings_tbl[0],
				mt9t112_regs.contrast_2_settings_tbl_size);

		if (rc < 0)
			return rc;

	}
		break;

	case CAMERA_CONTRAST_POSITIVE_1: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.contrast_3_settings_tbl[0],
				mt9t112_regs.contrast_3_settings_tbl_size);

		if (rc < 0)
			return rc;

	}
		break;

	case CAMERA_CONTRAST_POSITIVE_2: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.contrast_4_settings_tbl[0],
				mt9t112_regs.contrast_4_settings_tbl_size);

		if (rc < 0)
			return rc;

	}
		break;

	default: {
		if (rc < 0)
			return rc;

		return -EFAULT;
	}
	}

	return rc;
}

/* Saturation fucntion */
static long mt9t112_set_saturation(int mode, int saturation)
{
	long rc = 0;

	CDBG("mt9t112_set_saturation, mode = %d, saturation = %d\n",
		mode, saturation);

		printk(KERN_DEBUG "mt9t112_set_saturation, mode = %d, saturation = %d\n",
		mode, saturation);
	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		/* Context A Special Effects */
		break;

	case SENSOR_SNAPSHOT_MODE:
		/* Context B Special Effects */
		break;

	default:
		break;
	}

	switch (saturation) {
	case CAMERA_SATURATION_MINUS_2: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.saturation_0_settings_tbl[0],
				mt9t112_regs.saturation_0_settings_tbl_size);

		if (rc < 0)
			return rc;

	}
			break;

	case CAMERA_SATURATION_MINUS_1: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.saturation_1_settings_tbl[0],
				mt9t112_regs.saturation_1_settings_tbl_size);

		if (rc < 0)
			return rc;

	}
		break;

	case CAMERA_SATURATION_ZERO: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.saturation_2_settings_tbl[0],
				mt9t112_regs.saturation_2_settings_tbl_size);

		if (rc < 0)
			return rc;

	}
		break;

	case CAMERA_SATURATION_POSITIVE_1: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.saturation_3_settings_tbl[0],
				mt9t112_regs.saturation_3_settings_tbl_size);

		if (rc < 0)
			return rc;

	}
		break;

	case CAMERA_SATURATION_POSITIVE_2: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.saturation_4_settings_tbl[0],
				mt9t112_regs.saturation_4_settings_tbl_size);

		if (rc < 0)
			return rc;

	}
		break;

	default: {
		if (rc < 0)
			return rc;

		return -EFAULT;
	}
	}

	return rc;
}

/* Auto-exposure Function */
static long mt9t112_set_meteringmod(int mode, int meteringmod)
{
	long rc = 0;

	CDBG("mt9t112_set_meteringmod, mode = %d, meteringmod = %d\n",
		mode, meteringmod);

		printk(KERN_DEBUG "mt9t112_set_meteringmod, mode = %d, meteringmod = %d\n",
		mode, meteringmod);

	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		/* Context A Special Effects */
		break;

	case SENSOR_SNAPSHOT_MODE:
		/* Context B Special Effects */
		break;

	default:
		break;
	}

	switch (meteringmod) {
	case CAMERA_AVERAGE_METERING: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.metering_frame_average_settings_tbl[0],
				mt9t112_regs.metering_frame_average_settings_tbl_size);

		if (rc < 0)
			return rc;

	}
			break;

	case CAMERA_CENTER_METERING: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.metering_center_weighted_settings_tbl[0],
				mt9t112_regs.metering_center_weighted_settings_tbl_size);

		if (rc < 0)
			return rc;

	}
		break;

	case CAMERA_SPOT_METERING: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.metering_spot_settings_tbl[0],
				mt9t112_regs.metering_spot_settings_tbl_size);

		if (rc < 0)
			return rc;

	}
		break;

	default: {
		if (rc < 0)
			return rc;

		return -EFAULT;
	}
	}

	return rc;
}
/* } FIH, IvanCHTang, 2010/6/8 */

static long mt9t112_set_antibanding(int mode, int antibanding)
{
	long rc = 0;

	CDBG("mt9t112_set_antibanding, mode = %d, antibanding = %d\n",
		mode, antibanding);

		printk(KERN_DEBUG "mt9t112_set_antibanding, mode = %d, antibanding = %d\n",
		mode, antibanding);

	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		/* Context A Special Effects */
		break;

	case SENSOR_SNAPSHOT_MODE:
		/* Context B Special Effects */
		break;

	default:
		break;
	}

	switch (antibanding) {
	case CAMERA_ANTIBANDING_OFF: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.anti_band_off_settings_tbl[0],
				mt9t112_regs.anti_band_off_settings_tbl_size);

		if (rc < 0)
			return rc;

		mt9t112_m_60Hz=0;
	}
			break;

	case CAMERA_ANTIBANDING_60HZ: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.anti_band_60hz_settings_tbl[0],
				mt9t112_regs.anti_band_60hz_settings_tbl_size);

		if (rc < 0)
			return rc;

		mt9t112_m_60Hz=1;
	}
		break;

	case CAMERA_ANTIBANDING_50HZ: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.anti_band_50hz_settings_tbl[0],
				mt9t112_regs.anti_band_50hz_settings_tbl_size);

		if (rc < 0)
			return rc;

		mt9t112_m_60Hz=0;
	}
		break;

	case CAMERA_ANTIBANDING_AUTO: {
		rc = mt9t112_i2c_write_table(&mt9t112_regs.anti_band_auto_settings_tbl[0],
				mt9t112_regs.anti_band_auto_settings_tbl_size);

		if (rc < 0)
			return rc;

		mt9t112_m_60Hz=0;
	}
		break;

	default: {
		if (rc < 0)
			return rc;

		mt9t112_m_60Hz=0;
		return -EINVAL;
	}
	}

	return rc;
}

/* FIH, Charles Huang, 2009/11/04 { */
/* [FXX_CR], af function  */
#ifdef CONFIG_FIH_FXX
static long mt9t112_set_autofocus(int mode, int autofocus)
{
	long rc = 0;

	CDBG("mt9t112_set_autofocus, mode = %d, autofocus = %d\n",
		mode, autofocus);

		printk(KERN_DEBUG "mt9t112_set_autofocus, mode = %d, autofocus = %d\n",
		mode, autofocus);
	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		/* Context A Special Effects */
		break;

	case SENSOR_SNAPSHOT_MODE:
		/* Context B Special Effects */
		break;

	default:
		break;
	}

	switch (autofocus) {
	/* FIH, IvanCHTang, 2010/6/9 { */
	/* [FXX_CR], Add camera features for FM6 - [FM6#0004] */
	#if (0) // Not ready to use++
	case CAMERA_AUTOFOCUS_WINDOW_FULL:
	{
		struct mt9t112_i2c_reg_conf const mt9t112_autofocus_window_full_tbl[] = {
			{0x098E, 0x3005, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0x0008, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3007, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0x0000, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3009, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0x0100, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x300B, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0x00C1, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3022, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0xDFFF, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3022, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0xFFFF, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3020, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0xFFDF, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3020, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0xFFFF, WORD_LEN, 0, 0xFFFF},
		};
		rc = mt9t112_i2c_write_table(&mt9t112_autofocus_window_full_tbl[0],
				ARRAY_SIZE(mt9t112_autofocus_window_full_tbl));

		if (rc < 0)
			return rc;
	}
		break;
	case CAMERA_AUTOFOCUS_WINDOW_FULL_CORNER:
	{
		struct mt9t112_i2c_reg_conf const mt9t112_autofocus_window_full_corner_tbl[] = {
			{0x098E, 0x3005, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0x0008, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3007, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0x0000, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3009, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0x0100, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x300B, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0x00C1, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3022, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0xF7FF, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3022, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0xD7FF, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3020, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0xFFF7, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3020, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0xFFD7, WORD_LEN, 0, 0xFFFF},
		};
		rc = mt9t112_i2c_write_table(&mt9t112_autofocus_window_full_corner_tbl[0],
				ARRAY_SIZE(mt9t112_autofocus_window_full_corner_tbl));

		if (rc < 0)
			return rc;
	}
		break;
	case CAMERA_AUTOFOCUS_WINDOW_MIDDLE:
	{
		struct mt9t112_i2c_reg_conf const mt9t112_autofocus_window_middle_tbl[] = {
			{0x098E, 0x3005, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0x00FD, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3007, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0x00C2, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3009, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0x0083, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x300B, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0x0062, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3022, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0xDFFF, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3022, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0xFFFF, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3020, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0xFFDF, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3020, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0xFFFF, WORD_LEN, 0, 0xFFFF},
		};
		rc = mt9t112_i2c_write_table(&mt9t112_autofocus_window_middle_tbl[0],
				ARRAY_SIZE(mt9t112_autofocus_window_middle_tbl));

		if (rc < 0)
			return rc;
	}
		break;
	case CAMERA_AUTOFOCUS_WINDOW_CENTER:
	{
		struct mt9t112_i2c_reg_conf const mt9t112_autofocus_window_center_tbl[] = {
			{0x098E, 0x3005, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0x0178, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3007, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0x012D, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3009, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0x0044, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x300B, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0x002E, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3022, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0xDFFF, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3022, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0xFFFF, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3020, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0xFFDF, WORD_LEN, 0, 0xFFFF},
			{0x098E, 0x3020, WORD_LEN, 0, 0xFFFF},
			{0x0990, 0xFFFF, WORD_LEN, 0, 0xFFFF},
		};
		rc = mt9t112_i2c_write_table(&mt9t112_autofocus_window_center_tbl[0],
				ARRAY_SIZE(mt9t112_autofocus_window_center_tbl));

		if (rc < 0)
			return rc;
	}
		break;
	#endif // Not ready to use--
	/* } FIH, IvanCHTang, 2010/6/9 */		
	case CAMERA_AUTOFOCUS:
	default: {
		/* FIH, IvanCHTang, 2010/5/27 { */
		/* [FXX_CR], (FM6.B-425) [Camera]The auto-focus is slow and stay at the blurred screen. - [FM6#0001] */
		#define ENABLE_AF_POLLING 1
		#if ENABLE_AF_POLLING
		unsigned short af_status = 0u;
		//unsigned short af_position = 0u;
		unsigned short af_timeout = 0;
		#endif // ENABLE_AF_POLLING
		/* } FIH, IvanCHTang, 2010/5/27 */
		struct mt9t112_i2c_reg_conf const mt9t112_autofocus_tbl[] = {
		    {0x098E, 0x3003, WORD_LEN, 1, 0xFFFF},
		    {0x0990, 0x0002, WORD_LEN, 1, 0xFFFF},
		    {0x098E, 0xB019, WORD_LEN, 1, 0xFFFF},
		    {0x0990, 0x0001, WORD_LEN, 1, 0xFFFF},
		};
		if (mt9t112_af_finish)
			rc = mt9t112_i2c_write_table(&mt9t112_autofocus_tbl[0],
					ARRAY_SIZE(mt9t112_autofocus_tbl));

		/* FIH, IvanCHTang, 2010/5/27 { */
		/* [FXX_CR], (FM6.B-425) [Camera]The auto-focus is slow and stay at the blurred screen. - [FM6#0001] */
		#if ENABLE_AF_POLLING
		CDBG("[MT9T112] Auto Focus++ \n");
		while(af_timeout < 10){
			// 1.Get the AF status
			mt9t112_i2c_write(MT9T112_I2C_WRITE_SLAVE_ID, 0x098E, 0x3000, WORD_LEN);
			mt9t112_i2c_read(MT9T112_I2C_READ_SLAVE_ID, 0x0990, &af_status, WORD_LEN);
			// 2.Get the position
			//mt9t112_i2c_write(MT9T112_I2C_WRITE_SLAVE_ID, 0x098E, 0x3024, WORD_LEN);
			//mt9t112_i2c_read(MT9T112_I2C_READ_SLAVE_ID, 0x0990, &af_position, WORD_LEN);
			//printk(KERN_DEBUG "[Camera] IvanCHTang: af_status (0x%x), af_position (0x%x)\r\n", af_status, af_position);

			CDBG("[MT9T112] af_status : 0x%x \n", af_status);
			// Skip the 1st time to read af_status is "0"
			if((!af_status) && (af_timeout > 0))
				break;

			// Delay 100 ms and read AF status again.
			msleep(100);
			af_timeout++;
		}
		CDBG("[MT9T112] Auto Focus-- \n");
		#endif // ENABLE_AF_POLLING
		/* } FIH, IvanCHTang, 2010/5/27 */

		if (rc < 0)
			return rc;
	}
			break;
	}

	return rc;
}
#endif
/* } FIH, Charles Huang, 2009/11/04 */

static int mt9t112_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
	uint16_t model_id = 0;
	int rc = 0;

	/* OV suggested Power up block End */
	/* Read the Model ID of the sensor */
	/* Read REG_MT9T112_MODEL_ID_HI & REG_MT9T112_MODEL_ID_LO */
	rc = mt9t112_i2c_read(MT9T112_I2C_READ_SLAVE_ID,
		REG_MT9T112_MODEL_ID, &model_id, WORD_LEN);
	if (rc < 0)
		goto init_probe_fail;

	CDBG("mt9t112 model_id = 0x%x\n", model_id);

	/* Check if it matches it with the value in Datasheet */
	if (model_id != MT9T112_MODEL_ID) {
		rc = -EFAULT;
		goto init_probe_fail;
	}

	rc = mt9t112_reg_init();
	if (rc < 0)
		goto init_probe_fail;

	return rc;

init_probe_fail:
	return rc;
}

int mt9t112_sensor_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	int HWID=FIH_READ_HWID_FROM_SMEM();
	struct vreg *vreg_gp1;
	struct vreg *vreg_msme2;
	/* } FIH, IvanCHTang, 2010/6/1 */
/* FIH, Charles Huang, 2010/03/01 { */
/* [FXX_CR], Add power onoff vreg */
#ifdef CONFIG_FIH_FXX
	/* VDD 3V */
	struct vreg *vreg_rftx;
	/* DCORE 1.5V */
	struct vreg *vreg_gp2;
	/* ACORE 2.8V */
	struct vreg *vreg_gp3;
#endif
/* } FIH, Charles Huang, 2010/03/01 */

	/* FIH, IvanCHTang, 2010/5/28 { */
	/* [FXX_CR], (FM6.B-591) [Stability][Camera]Camera ANR when enter camera then suspend mode quickly. - [FM6#0002] */
	#if WAKELOCK_SUSPEND
	CDBG("[MT9T112] wake_lock_init : WAKE_LOCK_SUSPEND \r\n");
	wake_lock_init(&mt9t112_wake_lock_suspend, WAKE_LOCK_SUSPEND, "mt9t112");
	CDBG("[MT9T112] wake_lock \r\n");
	wake_lock(&mt9t112_wake_lock_suspend);
	#endif // [end] #if WAKELOCK_SUSPEND
	/* } FIH, IvanCHTang, 2010/5/28 */

	mt9t112_ctrl = kzalloc(sizeof(struct mt9t112_ctrl), GFP_KERNEL);
	if (!mt9t112_ctrl) {
		CDBG("mt9t112_init failed!\n");
		rc = -ENOMEM;
		goto init_done;
	}

	if (data)
		mt9t112_ctrl->sensordata = data;

/* FIH, Charles Huang, 2010/03/01 { */
/* [FXX_CR], Add power onoff vreg */
#ifdef CONFIG_FIH_FXX
	/* turn on power */

	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		vreg_gp1 = vreg_get(NULL, "gp1");
		vreg_set_level(vreg_gp1, 2800);
		vreg_enable(vreg_gp1);
		/* FIH, IvanCHTang, 2010/6/30 { */
		/* [FXX_CR], [camera] DVDD (msme2 -> mpp1) & VGA_PD (mpp22 -> gpio<108>) for FN6 PR2 - [FM6#0010] */
		if(HWID>=CMCS_FN6_PR2)
			mpp_config_digital_out(mpp_1, MPP_CFG(MPP_DLOGIC_LVL_MSMP, MPP_DLOGIC_OUT_CTRL_HIGH));
		else
		{
			vreg_msme2 = vreg_get(NULL, "msme2");
			vreg_set_level(vreg_msme2, 1500);
			vreg_enable(vreg_msme2);
		}
		/* } FIH, IvanCHTang, 2010/6/30 */
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		vreg_rftx = vreg_get(NULL, "rftx");
		vreg_set_level(vreg_rftx, 2800);
		vreg_enable(vreg_rftx);
	
		vreg_gp2 = vreg_get(NULL, "gp2");
		vreg_set_level(vreg_gp2, 1800);
		vreg_enable(vreg_gp2);
	}
	/* } FIH, IvanCHTang, 2010/6/1 */

	vreg_gp3 = vreg_get(NULL, "gp3");
	vreg_set_level(vreg_gp3, 2800);
	vreg_enable(vreg_gp3);
#endif
/* } FIH, Charles Huang, 2010/03/01 */

	rc = mt9t112_reset(data);
	if (rc < 0) {
		CDBG("reset failed!\n");
		goto init_fail;
	}

	/* EVB CAMIF cannont handle in 24MHz */
	/* EVB use 12.8MHz */
	/* R4215 couldn't use too high clk */
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef MT9T112_USE_VFS
	if (mt9t112_use_vfs_mclk_setting!=0)
		msm_camio_clk_rate_set(mt9t112_use_vfs_mclk_setting);
	else
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */

	msm_camio_clk_rate_set(24000000);
	msleep(25);

	msm_camio_camif_pad_reg_reset();

	rc = mt9t112_sensor_init_probe(data);
	if (rc < 0) {
		CDBG("mt9t112_sensor_init failed!\n");
		goto init_fail;
	}

init_done:
	return rc;

init_fail:
	kfree(mt9t112_ctrl);
	return rc;
}

static int mt9t112_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&mt9t112_wait_queue);
	return 0;
}

int mt9t112_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	long   rc = 0;

	if (copy_from_user(
				&cfg_data,
				(void *)argp,
				sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	/* down(&mt9t112_sem); */

	CDBG("mt9t112_sensor_config, cfgtype = %d, mode = %d\n",
		cfg_data.cfgtype, cfg_data.mode);

	/* FIH, IvanCHTang, 2010/9/10 { */
	/* [FXX_CR], [Camera] Fix CTS failed on the timeout of initializing camera - [] */
	// Send the other setting when AP set the camera effecs at first time.
	// But, Must to skip the set_mode at the 1st time (HAL createInstance will call this.)
	#ifdef CONFIG_FIH_FXX
	if(!mt9t112_af_finish && cfg_data.cfgtype != CFG_SET_MODE)
		if(mt9t112_reg_init2() < 0)
			printk(KERN_ERR "mt9t112_reg_init2 failed!!!\r\n");

	#endif
	/* } FIH, IvanCHTang, 2010/9/10 */
	
	switch (cfg_data.cfgtype) {
	case CFG_SET_MODE:
		rc = mt9t112_set_sensor_mode(
					cfg_data.mode);
		break;

	case CFG_SET_EFFECT:
		rc = mt9t112_set_effect(
					cfg_data.mode,
					cfg_data.cfg.effect);
		break;

	case CFG_START:
		rc = -EFAULT;
		break;
		
	case CFG_PWR_UP:
		rc = -EFAULT;
		break;

	case CFG_PWR_DOWN:
		rc = -EFAULT;
		break;

	case CFG_WRITE_EXPOSURE_GAIN:
		rc = -EFAULT;
		break;

	case CFG_MOVE_FOCUS:
		rc = -EFAULT;
		break;

	case CFG_REGISTER_TO_REAL_GAIN:
		rc = -EFAULT;
		break;

	case CFG_REAL_TO_REGISTER_GAIN:
		rc = -EFAULT;
		break;

	case CFG_SET_FPS:
		rc = -EFAULT;
		break;

	case CFG_SET_PICT_FPS:
		rc = -EFAULT;
		break;

	case CFG_SET_BRIGHTNESS:
		rc = mt9t112_set_brightness(
					cfg_data.mode,
					cfg_data.cfg.brightness);
		break;

	/* FIH, IvanCHTang, 2010/6/8 { */
	/* [FXX_CR], Add camera features for FM6 - [FM6#0004] */
	#ifdef CONFIG_FIH_FXX
	case CFG_SET_SHARPNESS:
		rc = mt9t112_set_sharpness(
					cfg_data.mode,
					cfg_data.cfg.sharpness);
		break;

	case CFG_SET_SATURATION:
		rc = mt9t112_set_saturation(
					cfg_data.mode,
					cfg_data.cfg.saturation);
		break;

	case CFG_SET_METERINGMOD:
		rc = mt9t112_set_meteringmod(
					cfg_data.mode,
					cfg_data.cfg.meteringmod);
		break;

	case CFG_SET_CONTRAST:
		rc = mt9t112_set_contrast(
					cfg_data.mode,
					cfg_data.cfg.contrast);
		break;
	#endif
	/* } FIH, IvanCHTang, 2010/6/8 */

	case CFG_SET_EXPOSURE_MODE:
		rc = -EFAULT;
		break;

	case CFG_SET_WB:
		rc = mt9t112_set_wb(
					cfg_data.mode,
					cfg_data.cfg.wb);
		break;

	case CFG_SET_ANTIBANDING:
		rc = mt9t112_set_antibanding(
					cfg_data.mode,
					cfg_data.cfg.antibanding);
		break;

	case CFG_SET_EXP_GAIN:
		rc = -EFAULT;
		break;

	case CFG_SET_PICT_EXP_GAIN:
		rc = -EFAULT;
		break;

	case CFG_SET_LENS_SHADING:
		rc = -EFAULT;
		break;

	case CFG_GET_PICT_FPS:
		rc = -EFAULT;
		break;

	case CFG_GET_PREV_L_PF:
		rc = -EFAULT;
		break;

	case CFG_GET_PREV_P_PL:
		rc = -EFAULT;
		break;

	case CFG_GET_PICT_L_PF:
		rc = -EFAULT;
		break;

	case CFG_GET_PICT_P_PL:
		rc = -EFAULT;
		break;

	case CFG_GET_AF_MAX_STEPS:
		rc = -EFAULT;
		break;

	case CFG_GET_PICT_MAX_EXP_LC:
		rc = -EFAULT;
		break;

/* [FXX_CR], af function  */
#ifdef CONFIG_FIH_FXX
	case CFG_SET_AUTOFOCUS:
		rc = mt9t112_set_autofocus(
					cfg_data.mode,
					cfg_data.cfg.autofocus);
		break;
#endif
/* } FIH, Charles Huang, 2009/11/04 */

	default:
		rc = -EINVAL;
		break;
	}

	/* up(&mt9t112_sem); */

	return rc;
}

int mt9t112_sensor_release(void)
{
	int rc = 0;
	printk(KERN_DEBUG "%s: E\r\n", __func__);
	/* FIH, IvanCHTang, 2010/8/16 { */
	/* [FXX_CR], [Camera] Add the SW standby for VCM WV511 - [FM6#0014] */
	#ifdef CONFIG_FIH_FXX
	#if (1) //EN_WV511_STANDBY
	// Don't set all source pin & reset pin.
	#else // EN_WV511_STANDBY
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	int HWID=FIH_READ_HWID_FROM_SMEM();
	struct vreg *vreg_gp1;
	struct vreg *vreg_msme2;
	/* } FIH, IvanCHTang, 2010/6/1 */
/* FIH, Charles Huang, 2010/03/01 { */
/* [FXX_CR], Add power onoff vreg */
#ifdef CONFIG_FIH_FXX
	/* VDD 3V */
	struct vreg *vreg_rftx;
	/* DCORE 1.5V */
	struct vreg *vreg_gp2;
	/* ACORE 2.8V */
	struct vreg *vreg_gp3;
#endif
/* } FIH, Charles Huang, 2010/03/01 */
	#endif // EN_WV511_STANDBY
	#endif
	/* } FIH, IvanCHTang, 2010/8/16 */
	const struct msm_camera_sensor_info *dev;
	/* down(&mt9t112_sem); */

	// Reset the init state.
	mt9t112_af_finish = 0;

	/* FIH, IvanCHTang, 2010/5/28 { */
	/* [FXX_CR], (FM6.B-591) [Stability][Camera]Camera ANR when enter camera then suspend mode quickly. - [FM6#0002] */
	#if WAKELOCK_SUSPEND
	CDBG("[MT9T112] wake_unlock \r\n");
	wake_unlock(&mt9t112_wake_lock_suspend);
	CDBG("[MT9T112] wake_lock_destroy \r\n");
	wake_lock_destroy(&mt9t112_wake_lock_suspend);
	#endif // [end] #if WAKELOCK_SUSPEND
	/* } FIH, IvanCHTang, 2010/5/28 */

	mutex_lock(&mt9t112_mut);

	/* FIH, IvanCHTang, 2010/8/16 { */
	/* [FXX_CR], [Camera] Add the SW standby for VCM WV511 - [FM6#0014] */
	#ifdef CONFIG_FIH_FXX
	#if (1) //EN_WV511_STANDBY
	// WV511 SW standby cmd
	if(mt9t112_otp_id == 0x511)
		mt9t112_i2c_write_table(&mt9t112_regs.af_WV511_standby_settings_tbl[0],
								mt9t112_regs.af_WV511_standby_settings_tbl_size);

	// Don't set all source pin & reset pin.
	dev = mt9t112_ctrl->sensordata;
	rc = gpio_request(dev->sensor_reset, "mt9t112");
	if (!rc) {
		rc = gpio_direction_output(dev->sensor_reset, 0);
	}
	gpio_free(dev->sensor_reset);

	rc = gpio_request(dev->sensor_pwd, "mt9t112");
	if (!rc) {
		rc = gpio_direction_output(dev->sensor_pwd, 1);
	}
	gpio_free(dev->sensor_pwd);

	#else // EN_WV511_STANDBY
	CDBG("[MT9T112 3M]  gpio_get_value(0) = %d\n", gpio_get_value(0));
	CDBG("[MT9T112 3M]  gpio_get_value(31) = %d\n", gpio_get_value(31));
	CDBG("[MT9T112 3M]  gpio_get_value(85) = %d\n", gpio_get_value(85));
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		;
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		CDBG("[MT9T112 3M]  gpio_get_value(122) = %d\n", gpio_get_value(122));
	}
	/* } FIH, IvanCHTang, 2010/6/1 */

       gpio_tlmm_config(GPIO_CFG(0, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), GPIO_ENABLE);
       gpio_tlmm_config(GPIO_CFG(85, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), GPIO_ENABLE);

/* FIH, Charles Huang, 2010/03/01 { */
/* [FXX_CR], Add power onoff vreg */
#ifdef CONFIG_FIH_FXX
	/* turn on power */

	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		vreg_gp1 = vreg_get(NULL, "gp1");
		vreg_disable(vreg_gp1);

		/* FIH, IvanCHTang, 2010/6/30 { */
		/* [FXX_CR], [camera] DVDD (msme2 -> mpp1) & VGA_PD (mpp22 -> gpio<108>) for FN6 PR2 - [FM6#0010] */
		if(HWID>=CMCS_FN6_PR2)
			mpp_config_digital_out(mpp_1, MPP_CFG(MPP_DLOGIC_LVL_MSMP, MPP_DLOGIC_OUT_CTRL_LOW));
		else
		{
			vreg_msme2 = vreg_get(NULL, "msme2");
			vreg_disable(vreg_msme2);
		}
		/* } FIH, IvanCHTang, 2010/6/30 */
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		vreg_rftx = vreg_get(NULL, "rftx");
		vreg_disable(vreg_rftx);

		vreg_gp2 = vreg_get(NULL, "gp2");
		vreg_disable(vreg_gp2);
	}
	/* } FIH, IvanCHTang, 2010/6/1 */

	vreg_gp3 = vreg_get(NULL, "gp3");
	vreg_disable(vreg_gp3);
#endif
/* } FIH, Charles Huang, 2010/03/01 */
	#endif // EN_WV511_STANDBY
	#endif
	/* } FIH, IvanCHTang, 2010/8/16 */

	kfree(mt9t112_ctrl);
	mt9t112_ctrl = NULL;
	/* up(&mt9t112_sem); */
	mutex_unlock(&mt9t112_mut);
	return rc;
}

/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef MT9T112_USE_VFS
void mt9t112_get_param(const char *buf, size_t count, struct mt9t112_i2c_reg_conf *tbl, 
	unsigned short tbl_size, int *use_setting)
{
	unsigned short waddr;
	unsigned short wdata;
	enum mt9t112_width width;
	unsigned short mdelay_time;
	unsigned short mask;
	char param1[10],param2[10],param3[10],param4[10];
	int read_count;
	const char *pstr;
	int vfs_index=0;
	pstr=buf;

	CDBG("count=%d\n",count);
	do
	{
		read_count=sscanf(pstr,"%4s,%4s,%4s,%4s",param1,param2,param3,param4);

      		//CDBG("pstr=%s\n",pstr);
      		//CDBG("read_count=%d,count=%d\n",read_count,count);
		if (read_count ==4)
		{
			waddr=simple_strtoul(param1,NULL,16);
			wdata=simple_strtoul(param2,NULL,16);
			width=0;
			mdelay_time=simple_strtoul(param3,NULL,16);;
			mask=simple_strtoul(param4,NULL,16);
				
			tbl[vfs_index].waddr= waddr;
			tbl[vfs_index].wdata= wdata;
			tbl[vfs_index].width= width;
			tbl[vfs_index].mdelay_time= mdelay_time;
			tbl[vfs_index].mask= mask;
			vfs_index++;

			if (vfs_index == tbl_size)
			{
				CDBG("Just match MAX_VFS_INDEX\n");
				*use_setting=1;
			}else if (vfs_index > tbl_size)
			{
				CDBG("Out of range MAX_VFS_INDEX\n");
				*use_setting=0;
				break;
			}
			
       		//CDBG("param1=%s,param2=%s,param3=%s,param4=%s\n",param1,param2,param3,param3);
       		//CDBG("waddr=0x%04X,wdata=0x%04X,width=%d,mdelay_time=%d,mask=0x%04X\n",waddr,wdata,width,mdelay_time,mask);
		}else{
			tbl[vfs_index].waddr= 0xFFFF;
			tbl[vfs_index].wdata= 0xFFFF;
			tbl[vfs_index].width= 0;
			tbl[vfs_index].mdelay_time= 0xFFFF;
			tbl[vfs_index].mask= 0xFFFF;
			*use_setting=1;
			break;
		}
		/* get next line */
		pstr=strchr(pstr, '\n');
		if (pstr==NULL)
			break;
		pstr++;
	}while(read_count!=0);


}

static ssize_t mt9t112_write_initreg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	mt9t112_get_param(buf, count, &mt9t112_vfs_init_settings_tbl[0], mt9t112_vfs_init_settings_tbl_size, &mt9t112_use_vfs_init_setting);
	return count;
}

static ssize_t mt9t112_write_oemtreg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	mt9t112_get_param(buf, count, &mt9t112_vfs_oem_settings_tbl[0], mt9t112_vfs_oem_settings_tbl_size, &mt9t112_use_vfs_oem_setting);
	return count;
}

static ssize_t mt9t112_write_previewreg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	mt9t112_get_param(buf, count, &mt9t112_vfs_preview_settings_tbl[0], mt9t112_vfs_preview_settings_tbl_size, &mt9t112_use_vfs_preview_setting);
	return count;
}

static ssize_t mt9t112_write_snapreg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	mt9t112_get_param(buf, count, &mt9t112_vfs_snap_settings_tbl[0], mt9t112_vfs_snap_settings_tbl_size, &mt9t112_use_vfs_snap_setting);
	return count;
}

static ssize_t mt9t112_write_snapaereg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	mt9t112_get_param(buf, count, &mt9t112_vfs_snapae_settings_tbl[0], mt9t112_vfs_snapae_settings_tbl_size, &mt9t112_use_vfs_snapae_setting);
	return count;
}

static ssize_t mt9t112_write_iqreg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	mt9t112_get_param(buf, count, &mt9t112_vfs_iq_settings_tbl[0], mt9t112_vfs_iq_settings_tbl_size, &mt9t112_use_vfs_iq_setting);
	return count;
}

static ssize_t mt9t112_write_lensreg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	mt9t112_get_param(buf, count, &mt9t112_vfs_lens_settings_tbl[0], mt9t112_vfs_lens_settings_tbl_size, &mt9t112_use_vfs_lens_setting);
	return count;
}

static ssize_t mt9t112_write_reg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	long rc = 0;

	mt9t112_get_param(buf, count, &mt9t112_vfs_writereg_settings_tbl[0], mt9t112_vfs_writereg_settings_tbl_size, &mt9t112_use_vfs_writereg_setting);
	if (mt9t112_use_vfs_writereg_setting)
	{
		rc = mt9t112_i2c_write_table(&mt9t112_vfs_writereg_settings_tbl[0],
			mt9t112_vfs_writereg_settings_tbl_size);
		mt9t112_use_vfs_writereg_setting =0;
	}
	return count;
}

static ssize_t mt9t112_setrange(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	mt9t112_get_param(buf, count, &mt9t112_vfs_getreg_settings_tbl[0], mt9t112_vfs_getreg_settings_tbl_size, &mt9t112_use_vfs_getreg_setting);
	return count;
}

static ssize_t mt9t112_getrange(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i,rc;
	char *str = buf;

	if (mt9t112_use_vfs_getreg_setting)
	{
		for (i=0;i<=mt9t112_vfs_getreg_settings_tbl_size;i++)
		{
			if (mt9t112_vfs_getreg_settings_tbl[i].waddr==0xFFFF)
				break;

			rc = mt9t112_i2c_read(MT9T112_I2C_READ_SLAVE_ID,
				mt9t112_vfs_getreg_settings_tbl[i].waddr, &(mt9t112_vfs_getreg_settings_tbl[i].wdata), WORD_LEN);
			CDBG("mt9t112 reg 0x%4X = 0x%2X\n", mt9t112_vfs_getreg_settings_tbl[i].waddr, mt9t112_vfs_getreg_settings_tbl[i].wdata);

			str += sprintf(str, "%04X,%4X,%2X\n", mt9t112_vfs_getreg_settings_tbl[i].waddr, 
				mt9t112_vfs_getreg_settings_tbl[i].wdata, 
				mt9t112_vfs_getreg_settings_tbl[i].mask);

			if (rc <0)
				break;
		}
	}
	return (str - buf);
}

static ssize_t mt9t112_setmclk(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf,"%d",&mt9t112_use_vfs_mclk_setting);
	return count;
}

static ssize_t mt9t112_getmclk(struct device *dev, struct device_attribute *attr, char *buf)
{
	return (sprintf(buf,"%d\n",mt9t112_use_vfs_mclk_setting));
}

static ssize_t mt9t112_setmultiple(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf,"%d",&mt9t112_use_vfs_multiple_setting);
	return count;
}

static ssize_t mt9t112_getmultiple(struct device *dev, struct device_attribute *attr, char *buf)
{
	return (sprintf(buf,"%d\n",mt9t112_use_vfs_multiple_setting));
}

static ssize_t mt9t112_setflashtime(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf,"%d",&mt9t112_use_vfs_flashtime_setting);
	return count;
}

static ssize_t mt9t112_getflashtime(struct device *dev, struct device_attribute *attr, char *buf)
{
	return (sprintf(buf,"%d\n",mt9t112_use_vfs_flashtime_setting));
}

DEVICE_ATTR(initreg_mt9t112, 0666, NULL, mt9t112_write_initreg);
DEVICE_ATTR(oemreg_mt9t112, 0666, NULL, mt9t112_write_oemtreg);
DEVICE_ATTR(previewreg_mt9t112, 0666, NULL, mt9t112_write_previewreg);
DEVICE_ATTR(snapreg_mt9t112, 0666, NULL, mt9t112_write_snapreg);
DEVICE_ATTR(snapregae_mt9t112, 0666, NULL, mt9t112_write_snapaereg);
DEVICE_ATTR(iqreg_mt9t112, 0666, NULL, mt9t112_write_iqreg);
DEVICE_ATTR(lensreg_mt9t112, 0666, NULL, mt9t112_write_lensreg);
DEVICE_ATTR(writereg_mt9t112, 0666, NULL, mt9t112_write_reg);
DEVICE_ATTR(getreg_mt9t112, 0666, mt9t112_getrange, mt9t112_setrange);
DEVICE_ATTR(mclk_mt9t112, 0666, mt9t112_getmclk, mt9t112_setmclk);
DEVICE_ATTR(multiple_mt9t112, 0666, mt9t112_getmultiple, mt9t112_setmultiple);
DEVICE_ATTR(flashtime_mt9t112, 0666, mt9t112_getflashtime, mt9t112_setflashtime);

static int create_attributes(struct i2c_client *client)
{
	int rc;

	dev_attr_initreg_mt9t112.attr.name = MT9T112_INITREG;
	dev_attr_oemreg_mt9t112.attr.name = MT9T112_OEMREG;
	dev_attr_previewreg_mt9t112.attr.name = MT9T112_PREVIEWREG;
	dev_attr_snapreg_mt9t112.attr.name = MT9T112_SNAPREG;
	dev_attr_snapregae_mt9t112.attr.name = MT9T112_SNAPAEREG;
	dev_attr_iqreg_mt9t112.attr.name = MT9T112_IQREG;
	dev_attr_lensreg_mt9t112.attr.name = MT9T112_LENSREG;
	dev_attr_writereg_mt9t112.attr.name = MT9T112_WRITEREG;
	dev_attr_getreg_mt9t112.attr.name = MT9T112_GETREG;
	dev_attr_mclk_mt9t112.attr.name = MT9T112_MCLK;
	dev_attr_multiple_mt9t112.attr.name = MT9T112_MULTIPLE;
	dev_attr_flashtime_mt9t112.attr.name = MT9T112_FLASHTIME;

	rc = device_create_file(&client->dev, &dev_attr_initreg_mt9t112);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create mt9t112 attribute \"initreg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}

	rc = device_create_file(&client->dev, &dev_attr_oemreg_mt9t112);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create mt9t112 attribute \"oemreg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}

	rc = device_create_file(&client->dev, &dev_attr_previewreg_mt9t112);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create mt9t112 attribute \"previewreg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}

	rc = device_create_file(&client->dev, &dev_attr_snapreg_mt9t112);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create mt9t112 attribute \"snapreg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_snapregae_mt9t112);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create mt9t112 attribute \"snapregae\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_iqreg_mt9t112);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create mt9t112 attribute \"iqreg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_lensreg_mt9t112);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create mt9t112 attribute \"lensreg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_writereg_mt9t112);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create mt9t112 attribute \"writereg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_getreg_mt9t112);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create mt9t112 attribute \"getreg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_mclk_mt9t112);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create mt9t112 attribute \"mclk\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_multiple_mt9t112);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create mt9t112 attribute \"multiple\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_flashtime_mt9t112);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create mt9t112 attribute \"flashtime\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	return rc;
}

static int remove_attributes(struct i2c_client *client)
{
	device_remove_file(&client->dev, &dev_attr_initreg_mt9t112);
	device_remove_file(&client->dev, &dev_attr_oemreg_mt9t112);
	device_remove_file(&client->dev, &dev_attr_previewreg_mt9t112);
	device_remove_file(&client->dev, &dev_attr_snapreg_mt9t112);
	device_remove_file(&client->dev, &dev_attr_snapregae_mt9t112);
	device_remove_file(&client->dev, &dev_attr_iqreg_mt9t112);
	device_remove_file(&client->dev, &dev_attr_lensreg_mt9t112);
	device_remove_file(&client->dev, &dev_attr_writereg_mt9t112);
	device_remove_file(&client->dev, &dev_attr_getreg_mt9t112);
	device_remove_file(&client->dev, &dev_attr_mclk_mt9t112);
	device_remove_file(&client->dev, &dev_attr_multiple_mt9t112);
	device_remove_file(&client->dev, &dev_attr_flashtime_mt9t112);

	return 0;
}
#endif /* MT9T112_USE_VFS */
#endif
/* } FIH, Charles Huang, 2009/06/24 */

/* FIH, IvanCHTang, 2010/7/16 { */
/* [FXX_CR], [MT9T112] Read OTP ID for different VCM driver IC - [FM6#0011] */
static unsigned short mt9t112_get_OTP_id(void)
{
	unsigned short otp_id = 0x0;

	// [Driver_IC_ID_Read]
	// Command
	mt9t112_i2c_write(MT9T112_I2C_WRITE_SLAVE_ID, 0x098E, 0x6031, WORD_LEN);
	mt9t112_i2c_write(MT9T112_I2C_WRITE_SLAVE_ID, 0x0990, 0x0100, WORD_LEN);
	mt9t112_i2c_write(MT9T112_I2C_WRITE_SLAVE_ID, 0x098E, 0x6037, WORD_LEN);
	mt9t112_i2c_write(MT9T112_I2C_WRITE_SLAVE_ID, 0x0990, 0x0001, WORD_LEN);
	msleep(1000);
	mt9t112_i2c_write(MT9T112_I2C_WRITE_SLAVE_ID, 0x098E, 0x6037, WORD_LEN);
	mt9t112_i2c_write(MT9T112_I2C_WRITE_SLAVE_ID, 0x0990, 0xA010, WORD_LEN);
	msleep(100);
	// OTP value
	mt9t112_i2c_write(MT9T112_I2C_WRITE_SLAVE_ID, 0x098E, 0x6033, WORD_LEN);
	mt9t112_i2c_read(MT9T112_I2C_READ_SLAVE_ID, 0x0990, &otp_id, WORD_LEN);
	printk(KERN_DEBUG "[MT9T112] OTP id : 0x%x \r\n", otp_id);

	return otp_id;
}
/* } FIH, IvanCHTang, 2010/7/16 */

/* FIH, Charles Huang, 2010/02/26 { */
/* [FXX_CR], move to kthread and speed up launching camera app */
#ifdef CONFIG_FIH_FXX
static int mt9t112_firmware_thread(void *arg)
{
	int ret = 0;

	daemonize("mt9t112_firmware_thread");

	while (1) {
		wait_for_completion(&mt9t112_firmware_init);
		CAM_USER_G0(KERN_INFO "%s: Got init signal\n", __func__);
		printk(KERN_INFO "%s++ \r\n", __func__);
		// Other setting
		ret = mt9t112_i2c_write_table(&mt9t112_regs.other_settings_tbl[0],
									mt9t112_regs.other_settings_tbl_size);
		/* FIH, IvanCHTang, 2010/6/15 { */
		/* [FXX_CR], [Camera] MT9T112 for new VCM driver IC setting - [FM6#0007] */
		// Patch init. settings
		#if (1)
		// Get the OTP id & set mt9t112_otp_is_Read is true
		if(!mt9t112_otp_is_Read)
		{
			mt9t112_otp_id = mt9t112_get_OTP_id();
			mt9t112_otp_is_Read = true;
		}
		msleep(3000); // 20100628, Delay for waiting AP setting

		/* FIH, IvanCHTang, 2010/7/16 { */
		/* [FXX_CR], [MT9T112] Read OTP ID for different VCM driver IC - [FM6#0011] */
		// AF driver IC is WV511 if OTP id is 0x511
		if(mt9t112_otp_id == 0x0511)
		{
			// Patch for WV511
			printk(KERN_DEBUG "af_WV511_patch_settings_tbl++ \r\n");
			ret = mt9t112_i2c_write_table(&mt9t112_regs.af_WV511_patch_settings_tbl[0],
										mt9t112_regs.af_WV511_patch_settings_tbl_size);
			printk(KERN_DEBUG "af_WV511_patch_settings_tbl-- \r\n");
			//Use the LSC setting calibrated by MCNEX
			printk(KERN_DEBUG "oem_lsc_settings_tbl++ \r\n");
			ret = mt9t112_i2c_write_table(&mt9t112_regs.oem_lsc_settings_tbl[0],
										mt9t112_regs.oem_lsc_settings_tbl_size);
			printk(KERN_DEBUG "oem_lsc_settings_tbl-- \r\n");
			// VCM WV511 setting
			printk(KERN_DEBUG "af_WV511_init_settings_tbl++ \r\n");
			ret = mt9t112_i2c_write_table(&mt9t112_regs.af_WV511_init_settings_tbl[0],
										mt9t112_regs.af_WV511_init_settings_tbl_size);
			printk(KERN_DEBUG "af_WV511_init_settings_tbl-- \r\n");
		}
		else
		{ 
			// Patch for A3904
			printk(KERN_DEBUG "af_patch_settings_tbl++ \r\n");
			ret = mt9t112_i2c_write_table(&mt9t112_regs.af_patch_settings_tbl[0],
										mt9t112_regs.af_patch_settings_tbl_size);
			printk(KERN_DEBUG "af_patch_settings_tbl-- \r\n");
			//Use the LSC setting calibrated by FIH optical
			printk(KERN_DEBUG "fih_lsc_settings_tbl++ \r\n");
			ret = mt9t112_i2c_write_table(&mt9t112_regs.fih_lsc_settings_tbl[0],
										mt9t112_regs.fih_lsc_settings_tbl_size);
			printk(KERN_DEBUG "fih_lsc_settings_tbl-- \r\n");
			// VCM A3904 setting
			printk(KERN_DEBUG "af_init_settings_tbl++ \r\n");
			ret = mt9t112_i2c_write_table(&mt9t112_regs.af_init_settings_tbl[0],
										mt9t112_regs.af_init_settings_tbl_size);
			printk(KERN_DEBUG "af_init_settings_tbl-- \r\n");
		}
		/* } FIH, IvanCHTang, 2010/7/16 */
		#endif
		/* } FIH, IvanCHTang, 2010/6/15 */
		//msleep(100);
		mt9t112_af_finish = 1;
		if (ret < 0)
			printk(KERN_INFO "firmware fail");
		printk(KERN_INFO "%s-- \r\n", __func__);
	}
	
    return 0;
}
#endif
/* } FIH, Charles Huang, 2010/02/26 */

static int mt9t112_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int rc = 0;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	mt9t112_sensorw =
		kzalloc(sizeof(struct mt9t112_work), GFP_KERNEL);

	if (!mt9t112_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, mt9t112_sensorw);
	mt9t112_init_client(client);
	mt9t112_client = client;
	
	CDBG("mt9t112_probe successed!\n");

	return 0;

probe_failure:
	kfree(mt9t112_sensorw);
	mt9t112_sensorw = NULL;
	CDBG("mt9t112_probe failed!\n");
	return rc;
}

static int __exit mt9t112_i2c_remove(struct i2c_client *client)
{
	struct mt9t112_work *sensorw = i2c_get_clientdata(client);

/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef MT9T112_USE_VFS
	remove_attributes(client);
#endif /* MT9T112_USE_VFS */
#endif
/* } FIH, Charles Huang, 2009/06/24 */

	free_irq(client->irq, sensorw);
	mt9t112_client = NULL;
	mt9t112_sensorw = NULL;
	mt9t112_af_finish = 0;
	kfree(sensorw);
	return 0;
}

#ifdef CONFIG_PM
static int mt9t112_suspend(struct i2c_client *client, pm_message_t mesg)
{
/* FIH, Charles Huang, 2009/06/25 { */
/* [FXX_CR], suspend/resume for pm */
#ifdef CONFIG_FIH_FXX
	/* sensor_pwd pin gpio31 */
	gpio_direction_output(31,1);
#endif
/* } FIH, Charles Huang, 2009/06/25 */
	return 0;
}

static int mt9t112_resume(struct i2c_client *client)
{
/* FIH, Charles Huang, 2009/06/25 { */
/* [FXX_CR], suspend/resume for pm */
#ifdef CONFIG_FIH_FXX
	/* sensor_pwd pin gpio31 */
	/* Handle by sensor initialization */
	/* workable setting for waste power while resuming */
	gpio_direction_output(31,1);
#endif
/* } FIH, Charles Huang, 2009/06/25 */
	return 0;
}
#else
# define mt9t112_suspend NULL
# define mt9t112_resume  NULL
#endif

static const struct i2c_device_id mt9t112_i2c_id[] = {
	{ "mt9t112", 0},
	{ },
};

static struct i2c_driver mt9t112_i2c_driver = {
	.id_table = mt9t112_i2c_id,
	.probe  = mt9t112_i2c_probe,
	.remove = __exit_p(mt9t112_i2c_remove),
	.suspend  	= mt9t112_suspend,
	.resume   	= mt9t112_resume,
	.driver = {
		.name = "mt9t112",
	},
};

static int mt9t112_sensor_probe(const struct msm_camera_sensor_info *info,
				struct msm_sensor_ctrl *s)
{
	int rc = i2c_add_driver(&mt9t112_i2c_driver);
	uint16_t model_id = 0;
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	struct vreg *vreg_gp1;
	struct vreg *vreg_msme2;
	/* } FIH, IvanCHTang, 2010/6/1 */
#if 1/*mr7*/
	int HWID=FIH_READ_HWID_FROM_SMEM();
#endif
/* FIH, Charles Huang, 2010/03/01 { */
/* [FXX_CR], Add power onoff vreg */
#ifdef CONFIG_FIH_FXX
	/* VDD 3V */
	struct vreg *vreg_rftx;
	/* DCORE 1.5V */
	struct vreg *vreg_gp2;
	/* ACORE 2.8V */
	struct vreg *vreg_gp3;
#endif
/* } FIH, Charles Huang, 2010/03/01 */

	if (rc < 0 || mt9t112_client == NULL) {
		rc = -ENOTSUPP;
		goto probe_done;
	}

/* FIH, Charles Huang, 2010/03/01 { */
/* [FXX_CR], Add power onoff vreg */
#ifdef CONFIG_FIH_FXX
	/* turn on power */

	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		vreg_gp1 = vreg_get(NULL, "gp1");
		vreg_set_level(vreg_gp1, 2800);
		vreg_enable(vreg_gp1);

		/* FIH, IvanCHTang, 2010/6/30 { */
		/* [FXX_CR], [camera] DVDD (msme2 -> mpp1) & VGA_PD (mpp22 -> gpio<108>) for FN6 PR2 - [FM6#0010] */
		if(HWID>=CMCS_FN6_PR2)
			mpp_config_digital_out(mpp_1, MPP_CFG(MPP_DLOGIC_LVL_MSMP, MPP_DLOGIC_OUT_CTRL_HIGH));
		else
		{
			vreg_msme2 = vreg_get(NULL, "msme2");
			vreg_set_level(vreg_msme2, 1500);
			vreg_enable(vreg_msme2);
		}
		/* } FIH, IvanCHTang, 2010/6/30 */	
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		vreg_rftx = vreg_get(NULL, "rftx");
		vreg_set_level(vreg_rftx, 2800);
		vreg_enable(vreg_rftx);

		vreg_gp2 = vreg_get(NULL, "gp2");
		vreg_set_level(vreg_gp2, 1800);
		vreg_enable(vreg_gp2);
	}
	/* } FIH, IvanCHTang, 2010/6/1 */

	vreg_gp3 = vreg_get(NULL, "gp3");
	vreg_set_level(vreg_gp3, 2800);
	vreg_enable(vreg_gp3);
#endif
/* } FIH, Charles Huang, 2010/03/01 */

	rc = mt9t112_reset(info);
	if (rc < 0) {
		CDBG("reset failed!\n");
		goto probe_fail;
	}

	/* EVB CAMIF cannont handle in 24MHz */
	/* EVB use 12.8MHz */
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef MT9T112_USE_VFS
	if (mt9t112_use_vfs_mclk_setting!=0)
		msm_camio_clk_rate_set(mt9t112_use_vfs_mclk_setting);
	else
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */
#if 0/*mr7*/
	if (HWID==CMCS_HW_VER_EVB1)
		msm_camio_clk_rate_set(12288000);
	else
#endif
		msm_camio_clk_rate_set(24000000);
	msleep(25);

	/* OV suggested Power up block End */
	/* Read the Model ID of the sensor */
	/* Read REG_MT9T112_MODEL_ID_HI & REG_MT9T112_MODEL_ID_LO */
	rc = mt9t112_i2c_read(MT9T112_I2C_READ_SLAVE_ID,
		REG_MT9T112_MODEL_ID, &model_id, WORD_LEN);
	if (rc < 0)
		goto probe_fail;

	CDBG("mt9t112 model_id = 0x%x\n", model_id);

	/* Check if it matches it with the value in Datasheet */
	if (model_id != MT9T112_MODEL_ID) {
		rc = -EFAULT;
		goto probe_fail;
	}
	/* FIH, IvanCHTang, 2010/8/16 { */
	/* [FXX_CR], [Camera] Add the SW standby for VCM WV511 - [FM6#0014] */
	#ifdef CONFIG_FIH_FXX
	#if EN_WV511_STANDBY
	// Don't set the level of reset & PWD pin until WV511 standby completely
	#else // EN_WV511_STANDBY
	rc = gpio_request(info->sensor_reset, "mt9t112");
	if (!rc) {
		rc = gpio_direction_output(info->sensor_reset, 0);
	}
	gpio_free(info->sensor_reset);

	rc = gpio_request(info->sensor_pwd, "mt9t112");
	if (!rc) {
		rc = gpio_direction_output(info->sensor_pwd, 1);
	}
	gpio_free(info->sensor_pwd);
	#endif // EN_WV511_STANDBY
	#endif
	/* } FIH, IvanCHTang, 2010/8/16 */

	CDBG("[MT9T112 3M]  gpio_get_value(0) = %d\n", gpio_get_value(0));
	CDBG("[MT9T112 3M]  gpio_get_value(31) = %d\n", gpio_get_value(31));
	CDBG("[MT9T112 3M]  gpio_get_value(85) = %d\n", gpio_get_value(85));
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		;
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		CDBG("[MT9T112 3M]  gpio_get_value(122) = %d\n", gpio_get_value(122));
	}
	/* } FIH, IvanCHTang, 2010/6/1 */
	   
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef MT9T112_USE_VFS
	rc = create_attributes(mt9t112_client);
	if (rc < 0) {
		dev_err(&mt9t112_client->dev, "%s: create attributes failed!! <%d>", __func__, rc);
		goto probe_done;
	}
#endif /* MT9T112_USE_VFS */
#endif
/* } FIH, Charles Huang, 2009/06/24 */
/* FIH, Charles Huang, 2010/02/26 { */
/* [FXX_CR], move to kthread and speed up launching camera app */
#ifdef CONFIG_FIH_FXX
// 20100910, remove by Ivan - Don't create the thread for sending the other setting
//	kernel_thread(mt9t112_firmware_thread, NULL, CLONE_FS | CLONE_FILES);
#endif
/* } FIH, Charles Huang, 2010/02/26 */

	s->s_init = mt9t112_sensor_init;
	s->s_release = mt9t112_sensor_release;
	s->s_config  = mt9t112_sensor_config;

probe_done:
	/* FIH, IvanCHTang, 2010/8/16 { */
	/* [FXX_CR], [Camera] Add the SW standby for VCM WV511 - [FM6#0014] */
	#ifdef CONFIG_FIH_FXX
	#if EN_WV511_STANDBY
#if 1
	
	// Initial setting
	mt9t112_i2c_write_table(&mt9t112_regs.init_settings_tbl[0],
							mt9t112_regs.init_settings_tbl_size);

	// Get the OTP id & set mt9t112_otp_is_Read is true
	mt9t112_otp_id = mt9t112_get_OTP_id();
	mt9t112_otp_is_Read = true;

	// The WV511 SW Standby
	if(mt9t112_otp_id == 0x0511)
	{
	// WV511 initialization
	mt9t112_i2c_write_table(&mt9t112_regs.af_WV511_init_settings_tbl[0],
							mt9t112_regs.af_WV511_init_settings_tbl_size);
	// WV511 SW standby cmd
	mt9t112_i2c_write_table(&mt9t112_regs.af_WV511_standby_settings_tbl[0],
							mt9t112_regs.af_WV511_standby_settings_tbl_size);
	}
	// Pull PWD pin HIGH to enter HW standby
	rc = gpio_request(info->sensor_pwd, "mt9t112");
	if (!rc) {
		rc = gpio_direction_output(info->sensor_pwd, 1);
	}
	gpio_free(info->sensor_pwd);

	// Pull reset pin LOW
	rc = gpio_request(info->sensor_reset, "mt9t112");
	if (!rc) {
		rc = gpio_direction_output(info->sensor_reset, 0);
	}
	gpio_free(info->sensor_reset);
#endif
	#else // EN_WV511_STANDBY
/* FIH, Charles Huang, 2010/03/01 { */
/* [FXX_CR], Add power onoff vreg */
#ifdef CONFIG_FIH_FXX
	/* turn off power */

	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		vreg_gp1 = vreg_get(NULL, "gp1");
		vreg_disable(vreg_gp1);

		/* FIH, IvanCHTang, 2010/6/30 { */
		/* [FXX_CR], [camera] DVDD (msme2 -> mpp1) & VGA_PD (mpp22 -> gpio<108>) for FN6 PR2 - [FM6#0010] */
		if(HWID>=CMCS_FN6_PR2)
			mpp_config_digital_out(mpp_1, MPP_CFG(MPP_DLOGIC_LVL_MSMP, MPP_DLOGIC_OUT_CTRL_LOW));
		else
		{
			vreg_msme2 = vreg_get(NULL, "msme2");
			vreg_disable(vreg_msme2);
		}
		/* } FIH, IvanCHTang, 2010/6/30 */
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		vreg_rftx = vreg_get(NULL, "rftx");
		vreg_disable(vreg_rftx);
	
		vreg_gp2 = vreg_get(NULL, "gp2");
		vreg_disable(vreg_gp2);
	}
	/* } FIH, IvanCHTang, 2010/6/1 */

	vreg_gp3 = vreg_get(NULL, "gp3");
	vreg_disable(vreg_gp3);
#endif
/* } FIH, Charles Huang, 2010/03/01 */
	#endif // EN_WV511_STANDBY
	#endif
	/* } FIH, IvanCHTang, 2010/8/16 */
	dev_info(&mt9t112_client->dev, "probe_done %s %s:%d\n", __FILE__, __func__, __LINE__);
	return rc;

probe_fail:
	dev_info(&mt9t112_client->dev, "probe_fail %s %s:%d\n", __FILE__, __func__, __LINE__);
	gpio_tlmm_config(GPIO_CFG(0, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
	gpio_direction_output(0,0);
	gpio_tlmm_config(GPIO_CFG(31, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
	gpio_direction_output(31,1);
#if 0/*mr7*/
	if (HWID>=CMCS_7627_EVB1)
	{
		/* Switch disable */
		gpio_tlmm_config(GPIO_CFG(121, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
		gpio_direction_output(121,1);
	}
#endif
	CDBG("[MT9T112 3M]  gpio_get_value(0) = %d\n", gpio_get_value(0));
	CDBG("[MT9T112 3M]  gpio_get_value(31) = %d\n", gpio_get_value(31));
	CDBG("[MT9T112 3M]  gpio_get_value(85) = %d\n", gpio_get_value(85));
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		;
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		CDBG("[MT9T112 3M]  gpio_get_value(122) = %d\n", gpio_get_value(122));
	}
	/* } FIH, IvanCHTang, 2010/6/1 */

/* FIH, Charles Huang, 2010/03/01 { */
/* [FXX_CR], Add power onoff vreg */
#ifdef CONFIG_FIH_FXX
	/* turn on power */

	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		vreg_gp1 = vreg_get(NULL, "gp1");
		vreg_disable(vreg_gp1);

		/* FIH, IvanCHTang, 2010/6/30 { */
		/* [FXX_CR], [camera] DVDD (msme2 -> mpp1) & VGA_PD (mpp22 -> gpio<108>) for FN6 PR2 - [FM6#0010] */
		if(HWID>=CMCS_FN6_PR2)
			mpp_config_digital_out(mpp_1, MPP_CFG(MPP_DLOGIC_LVL_MSMP, MPP_DLOGIC_OUT_CTRL_LOW));
		else
		{
			vreg_msme2 = vreg_get(NULL, "msme2");
			vreg_disable(vreg_msme2);
		}
		/* } FIH, IvanCHTang, 2010/6/30 */
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		vreg_rftx = vreg_get(NULL, "rftx");
		vreg_disable(vreg_rftx);
	
		vreg_gp2 = vreg_get(NULL, "gp2");
		vreg_disable(vreg_gp2);
	}
	/* } FIH, IvanCHTang, 2010/6/1 */

	vreg_gp3 = vreg_get(NULL, "gp3");
	vreg_disable(vreg_gp3);
#endif
/* } FIH, Charles Huang, 2010/03/01 */
	   
	return rc;
}

static int __mt9t112_probe(struct platform_device *pdev)
{
	return msm_camera_drv_start(pdev, mt9t112_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __mt9t112_probe,
	.driver = {
		.name = "msm_camera_mt9t112",
		.owner = THIS_MODULE,
	},
};

static int __init mt9t112_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(mt9t112_init);
