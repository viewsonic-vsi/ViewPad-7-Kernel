/*
 *     hm0356.c - Camera Sensor Driver
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
#include "hm0356.h"

#include <mach/msm_iomap.h>
#include <mach/msm_smd.h>
/* FIH, Charles Huang, 2010/03/01 { */
/* [FXX_CR], Add power onoff vreg */
#ifdef CONFIG_FIH_FXX
#include <mach/vreg.h>
#endif
/* } FIH, Charles Huang, 2010/03/01 */
/* FIH, IvanCHTang, 2010/6/1 { */
/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
#ifdef CONFIG_FIH_FXX
#include <mach/mpp.h>
static unsigned mpp_1 = 0;
static unsigned mpp_22 = 21;
#endif
/* } FIH, IvanCHTang, 2010/6/1 */

DEFINE_MUTEX(hm0356_mut);

/* Micron HM0356 Registers and their values */
/* Sensor Core Registers */
#define  REG_HM0356_MODEL_ID_HI 0x01
#define  REG_HM0356_MODEL_ID_LO 0x02
#define  HM0356_MODEL_ID     0x0356

#define  HM0356_I2C_READ_SLAVE_ID     0x68 >> 1  
#define  HM0356_I2C_WRITE_SLAVE_ID     0x69 >> 1  


struct hm0356_work {
	struct work_struct work;
};

static struct  hm0356_work *hm0356_sensorw;
static struct  i2c_client *hm0356_client;

struct hm0356_ctrl {
	const struct msm_camera_sensor_info *sensordata;
};

extern void brightness_onoff(int on);
static struct hm0356_ctrl *hm0356_ctrl;

/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#define HM0356_USE_VFS
#ifdef HM0356_USE_VFS
#define HM0356_INITREG "initreg"
#define HM0356_OEMREG "oemreg"
#define HM0356_PREVIEWREG "previewreg"
#define HM0356_SNAPREG "snapreg"
#define HM0356_SNAPAEREG "snapaereg"
#define HM0356_IQREG "iqreg"
#define HM0356_LENSREG "lensreg"
#define HM0356_WRITEREG "writereg"
#define HM0356_GETREG "getreg"
#define HM0356_MCLK "mclk"
#define HM0356_MULTIPLE "multiple"
#define HM0356_FLASHTIME "flashtime"

/* MAX buf is ???? */
#define HM0356_MAX_VFS_INIT_INDEX 330
int hm0356_use_vfs_init_setting=0;
struct hm0356_i2c_reg_conf hm0356_vfs_init_settings_tbl[HM0356_MAX_VFS_INIT_INDEX];
uint16_t hm0356_vfs_init_settings_tbl_size= ARRAY_SIZE(hm0356_vfs_init_settings_tbl);

#define HM0356_MAX_VFS_OEM_INDEX 330
int hm0356_use_vfs_oem_setting=0;
struct hm0356_i2c_reg_conf hm0356_vfs_oem_settings_tbl[HM0356_MAX_VFS_OEM_INDEX];
uint16_t hm0356_vfs_oem_settings_tbl_size= ARRAY_SIZE(hm0356_vfs_oem_settings_tbl);

#define HM0356_MAX_VFS_PREVIEW_INDEX 330
int hm0356_use_vfs_preview_setting=0;
struct hm0356_i2c_reg_conf hm0356_vfs_preview_settings_tbl[HM0356_MAX_VFS_PREVIEW_INDEX];
uint16_t hm0356_vfs_preview_settings_tbl_size= ARRAY_SIZE(hm0356_vfs_preview_settings_tbl);

#define HM0356_MAX_VFS_SNAP_INDEX 330
int hm0356_use_vfs_snap_setting=0;
struct hm0356_i2c_reg_conf hm0356_vfs_snap_settings_tbl[HM0356_MAX_VFS_SNAP_INDEX];
uint16_t hm0356_vfs_snap_settings_tbl_size= ARRAY_SIZE(hm0356_vfs_snap_settings_tbl);

#define HM0356_MAX_VFS_SNAPAE_INDEX 330
int hm0356_use_vfs_snapae_setting=0;
struct hm0356_i2c_reg_conf hm0356_vfs_snapae_settings_tbl[HM0356_MAX_VFS_SNAPAE_INDEX];
uint16_t hm0356_vfs_snapae_settings_tbl_size= ARRAY_SIZE(hm0356_vfs_snapae_settings_tbl);

#define HM0356_MAX_VFS_IQ_INDEX 330
int hm0356_use_vfs_iq_setting=0;
struct hm0356_i2c_reg_conf hm0356_vfs_iq_settings_tbl[HM0356_MAX_VFS_IQ_INDEX];
uint16_t hm0356_vfs_iq_settings_tbl_size= ARRAY_SIZE(hm0356_vfs_iq_settings_tbl);

#define HM0356_MAX_VFS_LENS_INDEX 330
int hm0356_use_vfs_lens_setting=0;
struct hm0356_i2c_reg_conf hm0356_vfs_lens_settings_tbl[HM0356_MAX_VFS_LENS_INDEX];
uint16_t hm0356_vfs_lens_settings_tbl_size= ARRAY_SIZE(hm0356_vfs_lens_settings_tbl);

#define HM0356_MAX_VFS_WRITEREG_INDEX 330
int hm0356_use_vfs_writereg_setting=0;
struct hm0356_i2c_reg_conf hm0356_vfs_writereg_settings_tbl[HM0356_MAX_VFS_IQ_INDEX];
uint16_t hm0356_vfs_writereg_settings_tbl_size= ARRAY_SIZE(hm0356_vfs_writereg_settings_tbl);

#define HM0356_MAX_VFS_GETREG_INDEX 330
int hm0356_use_vfs_getreg_setting=0;
struct hm0356_i2c_reg_conf hm0356_vfs_getreg_settings_tbl[HM0356_MAX_VFS_GETREG_INDEX];
uint16_t hm0356_vfs_getreg_settings_tbl_size= ARRAY_SIZE(hm0356_vfs_getreg_settings_tbl);

int hm0356_use_vfs_mclk_setting=0;
int hm0356_use_vfs_multiple_setting=0;
int hm0356_use_vfs_flashtime_setting=0;
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */

static DECLARE_WAIT_QUEUE_HEAD(hm0356_wait_queue);
DECLARE_MUTEX(hm0356_sem);

/*=============================================================*/
static int hm0356_reset(const struct msm_camera_sensor_info *dev)
{
	int rc = 0;
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	//struct mpp *mpp_22;
	/* } FIH, IvanCHTang, 2010/6/1 */
#if 1/*mr7*/
	int HWID=FIH_READ_HWID_FROM_SMEM();
#endif
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
#if 0/*mr7*/
	if (HWID>=CMCS_7627_EVB1)
	{
#endif
		/* Switch enable */
		gpio_direction_output(85, 1);	
		msleep(10);
#if 0/*mr7*/
	}
#endif
#if 0/*mr7*/
	if (HWID==CMCS_HW_VER_EVB1 || HWID>=CMCS_7627_EVB1)
	{
		/* Clk switch */
		gpio_direction_output(17, 0);	
		msleep(10);
	}
#endif
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		/* FIH, IvanCHTang, 2010/7/7 { */
		/* [FXX_CR], [camera] DVDD (msme2 -> mpp1) & VGA_PD (mpp22 -> gpio<108>) for FN6 PR2 - [FM6#0010] */
		if(HWID>=CMCS_FN6_PR2)
		{
			gpio_tlmm_config(GPIO_CFG(108, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
			gpio_direction_output(108, 0);
		}
		else
			mpp_config_digital_out(mpp_22, MPP_CFG(MPP_DLOGIC_LVL_MSMP, MPP_DLOGIC_OUT_CTRL_LOW));
		/* } FIH, IvanCHTang, 2010/7/7 */
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		rc = gpio_request(dev->sensor_pwd, "hm0356");
		if (!rc) {
			rc = gpio_direction_output(dev->sensor_pwd, 0);
		}
		gpio_free(dev->sensor_pwd);
	}
	/* } FIH, IvanCHTang, 2010/6/1 */
#if 0	
	rc = gpio_request(dev->sensor_reset, "hm0356");
	if (!rc) {
		rc = gpio_direction_output(dev->sensor_reset, 0);
		msleep(20);
		rc = gpio_direction_output(dev->sensor_reset, 1);
	}
	gpio_free(dev->sensor_reset);
#endif	
	CDBG("[HM0356 VGA] gpio_get_value(0) = %d\n", gpio_get_value(0));
	CDBG("[HM0356 VGA] gpio_get_value(31) = %d\n", gpio_get_value(31));
	CDBG("[HM0356 VGA] gpio_get_value(85) = %d\n", gpio_get_value(85));
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		;
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		CDBG("[HM0356 VGA] gpio_get_value(122) = %d\n", gpio_get_value(122));
	}
	/* } FIH, IvanCHTang, 2010/6/1 */
	
	return rc;
}

static int32_t hm0356_i2c_txdata(unsigned short saddr,
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

	if (i2c_transfer(hm0356_client->adapter, msg, 1) < 0) {
		printk(KERN_ERR "hm0356_i2c_txdata failed, try again!\n");
		if (i2c_transfer(hm0356_client->adapter, msg, 1) < 0) {
			printk(KERN_ERR "hm0356_i2c_txdata failed\n");
			CDBG("hm0356_i2c_txdata failed\n");
			return -EIO;
		}
	}

	return 0;
}

static int32_t hm0356_i2c_write(unsigned short saddr,
	unsigned short waddr, unsigned short wdata, enum hm0356_width width)
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

		rc = hm0356_i2c_txdata(saddr, buf, 4);
	}
		break;

	case BYTE_LEN: {
		buf[0] = (uint8_t) waddr;
		buf[1] = (uint8_t) wdata;
		rc = hm0356_i2c_txdata(saddr, buf, 2);
	}
		break;

	default:
		break;
	}

	if (rc < 0)
		CDBG(
		"i2c_write failed, addr = 0x%x, val = 0x%x!\n",
		waddr, wdata);

	return rc;
}

static int32_t hm0356_i2c_rxdata(unsigned short saddr,
	unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
	{
		.addr   = saddr,
		.flags = 0,
		.len   = 1,
		.buf   = rxdata,
	},
	{
		.addr   = saddr,
		.flags = I2C_M_RD,
		.len   = length,
		.buf   = rxdata,
	},
	};

	if (i2c_transfer(hm0356_client->adapter, msgs, 2) < 0) {
		printk(KERN_ERR "hm0356_i2c_rxdata failed, try again!\n");
		if (i2c_transfer(hm0356_client->adapter, msgs, 2) < 0) {
			printk(KERN_ERR "hm0356_i2c_rxdata failed!\n");
			CDBG("hm0356_i2c_rxdata failed!\n");
			return -EIO;
		}
	}

	return 0;
}

static int32_t hm0356_i2c_read(unsigned short   saddr,
	unsigned short raddr, unsigned short *rdata, enum hm0356_width width)
{
	int32_t rc = 0;
	unsigned char buf[4];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));

	switch (width) {
	case WORD_LEN: {
		buf[0] = (uint8_t) raddr;

		rc = hm0356_i2c_rxdata(saddr, buf, 2);
		if (rc < 0)
			return rc;

		*rdata = buf[0] << 8 | buf[1];
	}
		break;
	case BYTE_LEN: {
		buf[0] = (uint8_t) raddr;

		rc = hm0356_i2c_rxdata(saddr, buf, 1);
		if (rc < 0)
			return rc;

		*rdata = buf[0];
	}
		break;

	default:
		break;
	}

	if (rc < 0)
		CDBG("hm0356_i2c_read failed!\n");

	return rc;
}

void hm0356_set_value_by_bitmask(uint16_t bitset, uint16_t mask, uint16_t  *new_value)
{
	uint16_t org;

	org= *new_value;
	*new_value = (org&~mask) | (bitset & mask);
}

static int32_t hm0356_i2c_write_table(
	struct hm0356_i2c_reg_conf const *reg_conf_tbl,
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
			rc = hm0356_i2c_write(HM0356_I2C_WRITE_SLAVE_ID,
				reg_conf_tbl->waddr, reg_conf_tbl->wdata,
				reg_conf_tbl->width);
		}else{
			uint16_t reg_value = 0;
			rc = hm0356_i2c_read(HM0356_I2C_READ_SLAVE_ID,
				reg_conf_tbl->waddr, &reg_value, BYTE_LEN);
			hm0356_set_value_by_bitmask(reg_conf_tbl->wdata,reg_conf_tbl->mask,&reg_value);
			rc = hm0356_i2c_write(HM0356_I2C_WRITE_SLAVE_ID,
					reg_conf_tbl->waddr, reg_value, BYTE_LEN);
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
static int32_t hm0356_set_lens_roll_off(void)
{
	int32_t rc = 0;
	rc = hm0356_i2c_write_table(&hm0356_regs.rftbl[0],
		hm0356_regs.rftbl_size);
	return rc;
}
#endif

static long hm0356_reg_init(void)
{
#if 0
	int32_t i;
#endif
	long rc;
#if 0
	/* PLL Setup Start */
	rc = hm0356_i2c_write_table(&mt9d112_regs.plltbl[0],
		hm0356_regs.plltbl_size);

	if (rc < 0)
		return rc;
	/* PLL Setup End   */
#endif

	/* Configure sensor for Preview mode and Snapshot mode */
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef HM0356_USE_VFS
	if (hm0356_use_vfs_init_setting)
		rc = hm0356_i2c_write_table(&hm0356_vfs_init_settings_tbl[0],
			hm0356_vfs_init_settings_tbl_size);
	else
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */
		rc = hm0356_i2c_write_table(&hm0356_regs.init_settings_tbl[0],
			hm0356_regs.init_settings_tbl_size);

	if (rc < 0)
		return rc;

	/* Configure sensor for image quality settings */
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef HM0356_USE_VFS
	if (hm0356_use_vfs_iq_setting)
		rc = hm0356_i2c_write_table(&hm0356_vfs_iq_settings_tbl[0],
			hm0356_vfs_iq_settings_tbl_size);
	else
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */
	rc = hm0356_i2c_write_table(&hm0356_regs.iq_settings_tbl[0],
		hm0356_regs.iq_settings_tbl_size);

	if (rc < 0)
		return rc;

/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef HM0356_USE_VFS
	if (hm0356_use_vfs_lens_setting)
		rc = hm0356_i2c_write_table(&hm0356_vfs_lens_settings_tbl[0],
			hm0356_vfs_lens_settings_tbl_size);
	else
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */
	if (rc < 0)
		return rc;

	/* Configure sensor for customer settings */
#if 0
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef HM0356_USE_VFS
	if (hm0356_use_vfs_oem_setting)
		rc = hm0356_i2c_write_table(&vfs_oem_settings_tbl[0],
			vfs_oem_settings_tbl_size);
	else
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */
	rc = hm0356_i2c_write_table(&hm0356_regs.oem_settings_tbl[0],
		hm0356_regs.oem_settings_tbl_size);

	if (rc < 0)
		return rc;
#endif
#if 0
	/* Configure for Noise Reduction, Saturation and Aperture Correction */
	array_length = hm0356_regs.noise_reduction_reg_settings_;

	for (i = 0; i < array_length; i++) {

		rc = hm0356_i2c_write(hm0356_client->addr,
		hm0356_regs.noise_reduction_reg_settings[i].register_address,
		hm0356_regs.noise_reduction_reg_settings[i].register_value,
			WORD_LEN);

		if (rc < 0)
			return rc;
	}

	/* Set Color Kill Saturation point to optimum value */
	rc =
	hm0356_i2c_write(hm0356_client->addr,
	0x35A4,
	0x0593,
	WORD_LEN);
	if (rc < 0)
		return rc;

	rc = hm0356_i2c_write_table(&hm0356_regs.stbl[0],
		hm0356_regs.stbl_size);
	if (rc < 0)
		return rc;

	rc = hm0356_set_lens_roll_off();
	if (rc < 0)
		return rc;
#endif

	return 0;
}

static long hm0356_set_sensor_mode(int mode)
{
	long rc = 0;

	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		/* Configure sensor for Preview mode */
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef HM0356_USE_VFS
		if (hm0356_use_vfs_preview_setting)
			rc = hm0356_i2c_write_table(&hm0356_vfs_preview_settings_tbl[0],
				hm0356_vfs_preview_settings_tbl_size);
		else
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */
			rc = hm0356_i2c_write_table(&hm0356_regs.preview_settings_tbl[0],
				hm0356_regs.preview_settings_tbl_size);

		if (rc < 0)
			return rc;

	/* Configure sensor for customer settings */
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef HM0356_USE_VFS
		if (hm0356_use_vfs_oem_setting)
			rc = hm0356_i2c_write_table(&hm0356_vfs_oem_settings_tbl[0],
				hm0356_vfs_oem_settings_tbl_size);
		else
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */
			rc = hm0356_i2c_write_table(&hm0356_regs.oem_settings_tbl[0],
				hm0356_regs.oem_settings_tbl_size);

		if (rc < 0)
			return rc;

		msleep(5);
		break;

	case SENSOR_SNAPSHOT_MODE:
		break;
	default:
		return -EFAULT;
	}

	return 0;
}

static long hm0356_set_effect(int mode, int effect)
{
	long rc = 0;

	CDBG("hm0356_set_effect, mode = %d, effect = %d\n",
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
	case CAMERA_EFFECT_OFF: {//Normal
		struct hm0356_i2c_reg_conf const hm0356_effect_off_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_effect_off_tbl[0],
				ARRAY_SIZE(hm0356_effect_off_tbl));

		if (rc < 0)
			return rc;
	}
			break;

	case CAMERA_EFFECT_MONO: {//B&W
		struct hm0356_i2c_reg_conf const hm0356_effect_mono_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_effect_mono_tbl[0],
				ARRAY_SIZE(hm0356_effect_mono_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_EFFECT_NEGATIVE: {//Negative
		struct hm0356_i2c_reg_conf const hm0356_effect_negative_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_effect_negative_tbl[0],
				ARRAY_SIZE(hm0356_effect_negative_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_EFFECT_SEPIA: {
		struct hm0356_i2c_reg_conf const hm0356_effect_sepia_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_effect_sepia_tbl[0],
				ARRAY_SIZE(hm0356_effect_sepia_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_EFFECT_BLUISH: {//Bluish
		struct hm0356_i2c_reg_conf const hm0356_effect_bluish_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_effect_bluish_tbl[0],
				ARRAY_SIZE(hm0356_effect_bluish_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_EFFECT_REDDISH: {//Reddish
		struct hm0356_i2c_reg_conf const hm0356_effect_reddish_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_effect_reddish_tbl[0],
				ARRAY_SIZE(hm0356_effect_reddish_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_EFFECT_GREENISH: {//Greenish
		struct hm0356_i2c_reg_conf const hm0356_effect_greenish_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_effect_greenish_tbl[0],
				ARRAY_SIZE(hm0356_effect_greenish_tbl));

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

static long hm0356_set_wb(int mode, int wb)
{
	long rc = 0;

	CDBG("hm0356_set_wb, mode = %d, wb = %d\n",
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
		struct hm0356_i2c_reg_conf const hm0356_wb_auto_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_wb_auto_tbl[0],
				ARRAY_SIZE(hm0356_wb_auto_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_WB_INCANDESCENT: {//TUNGSTEN
		struct hm0356_i2c_reg_conf const hm0356_wb_incandescent_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_wb_incandescent_tbl[0],
				ARRAY_SIZE(hm0356_wb_incandescent_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_WB_FLUORESCENT: {//Office
		struct hm0356_i2c_reg_conf const hm0356_wb_fluorescent_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_wb_fluorescent_tbl[0],
				ARRAY_SIZE(hm0356_wb_fluorescent_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_WB_DAYLIGHT: {//Sunny
		struct hm0356_i2c_reg_conf const hm0356_wb_daylight_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_wb_daylight_tbl[0],
				ARRAY_SIZE(hm0356_wb_daylight_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_WB_CLOUDY_DAYLIGHT: {//Cloudy
		struct hm0356_i2c_reg_conf const hm0356_wb_cloudydaylight_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_wb_cloudydaylight_tbl[0],
				ARRAY_SIZE(hm0356_wb_cloudydaylight_tbl));

		if (rc < 0)
			return rc;

		rc = hm0356_i2c_write(HM0356_I2C_WRITE_SLAVE_ID,
				0x3405, 0x45, BYTE_LEN);

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


static long hm0356_set_brightness(int mode, int brightness)
{
	long rc = 0;

	CDBG("hm0356_set_brightness, mode = %d, brightness = %d\n",
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

	switch (brightness) {
	case CAMERA_BRIGHTNESS_0: {
		struct hm0356_i2c_reg_conf const hm0356_brightness_0_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_brightness_0_tbl[0],
				ARRAY_SIZE(hm0356_brightness_0_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_BRIGHTNESS_1: {
		struct hm0356_i2c_reg_conf const hm0356_brightness_1_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_brightness_1_tbl[0],
				ARRAY_SIZE(hm0356_brightness_1_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_BRIGHTNESS_2: {
		struct hm0356_i2c_reg_conf const hm0356_brightness_2_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_brightness_2_tbl[0],
				ARRAY_SIZE(hm0356_brightness_2_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_BRIGHTNESS_3: {
		struct hm0356_i2c_reg_conf const hm0356_brightness_3_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_brightness_3_tbl[0],
				ARRAY_SIZE(hm0356_brightness_3_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_BRIGHTNESS_4: {
		struct hm0356_i2c_reg_conf const hm0356_brightness_4_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_brightness_4_tbl[0],
				ARRAY_SIZE(hm0356_brightness_4_tbl));

		if (rc < 0)
			return rc;
	} 
		break;

	case CAMERA_BRIGHTNESS_5: {
		struct hm0356_i2c_reg_conf const hm0356_brightness_5_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_brightness_5_tbl[0],
				ARRAY_SIZE(hm0356_brightness_5_tbl));

		if (rc < 0)
			return rc;
	} 
		break;

	case CAMERA_BRIGHTNESS_6: {
		struct hm0356_i2c_reg_conf const hm0356_brightness_6_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_brightness_6_tbl[0],
				ARRAY_SIZE(hm0356_brightness_6_tbl));

		if (rc < 0)
			return rc;
	} 
		break;

	case CAMERA_BRIGHTNESS_7:
	case CAMERA_BRIGHTNESS_8:
	case CAMERA_BRIGHTNESS_9:
	case CAMERA_BRIGHTNESS_10: {
		struct hm0356_i2c_reg_conf const hm0356_brightness_10_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_brightness_10_tbl[0],
				ARRAY_SIZE(hm0356_brightness_10_tbl));

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

static long hm0356_set_antibanding(int mode, int antibanding)
{
	long rc = 0;

	CDBG("hm0356_set_antibanding, mode = %d, antibanding = %d\n",
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
		struct hm0356_i2c_reg_conf const hm0356_antibanding_off_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_antibanding_off_tbl[0],
				ARRAY_SIZE(hm0356_antibanding_off_tbl));

		if (rc < 0)
			return rc;

	}
			break;

	case CAMERA_ANTIBANDING_60HZ: {
		struct hm0356_i2c_reg_conf const hm0356_antibanding_60hz_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_antibanding_60hz_tbl[0],
				ARRAY_SIZE(hm0356_antibanding_60hz_tbl));

		if (rc < 0)
			return rc;

	}
		break;

	case CAMERA_ANTIBANDING_50HZ: {
		struct hm0356_i2c_reg_conf const hm0356_antibanding_60hz_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_antibanding_60hz_tbl[0],
				ARRAY_SIZE(hm0356_antibanding_60hz_tbl));

		if (rc < 0)
			return rc;

	}
		break;

	case CAMERA_ANTIBANDING_AUTO: {
		struct hm0356_i2c_reg_conf const hm0356_antibanding_60hz_tbl[] = {
		};

		rc = hm0356_i2c_write_table(&hm0356_antibanding_60hz_tbl[0],
				ARRAY_SIZE(hm0356_antibanding_60hz_tbl));

		if (rc < 0)
			return rc;

	}
		break;

	default: {
		if (rc < 0)
			return rc;

		return -EINVAL;
	}
	}

	return rc;
}

static int hm0356_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
	uint16_t model_id = 0;
/* FIH, Henry Juang, 2010/11/01 { */
/* [FXX_CR], Add retry to prevent VGA blocked.*/
	int rc = 0,i=0,total=0;
/* FIH, Henry Juang, 2010/11/01 } */

	/* OV suggested Power up block End */
	/* Read the Model ID of the sensor */
	/* Read REG_HM0356_MODEL_ID_HI & REG_HM0356_MODEL_ID_LO */
	rc = hm0356_i2c_read(HM0356_I2C_READ_SLAVE_ID,
		REG_HM0356_MODEL_ID_HI, &model_id, BYTE_LEN);
/* FIH, Henry Juang, 2010/11/01 { */
/* [FXX_CR], Add retry to prevent VGA blocked.*/	
	while(rc<0 && i<10) 
	{	
		gpio_tlmm_config(GPIO_CFG(122, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
		rc = gpio_request(data->sensor_pwd, "hm0356");
		if (!rc) {
			rc = gpio_direction_output(data->sensor_pwd, 1);
		}
		msleep(10);
		rc = gpio_direction_output(data->sensor_pwd, 0); 
		gpio_free(data->sensor_pwd);		
		total+=10+i*25;
		printk(KERN_WARNING"hm0356_sensor_init_probe failed and retry after sleep %d ms. %d times!,total=%d ms.\n ",10+i*25,i+1,total);
		msleep(10+i*25);
		rc = hm0356_i2c_read(HM0356_I2C_READ_SLAVE_ID,	REG_HM0356_MODEL_ID_HI, &model_id, BYTE_LEN);
		i++;
	}
/* FIH, Henry Juang, 2010/11/01 } */	
	
	if (rc < 0)
		goto init_probe_fail;

	CDBG("hm0356 model_id = 0x%x\n", model_id);

	rc = hm0356_i2c_read(HM0356_I2C_READ_SLAVE_ID,
		REG_HM0356_MODEL_ID_LO, &model_id, BYTE_LEN);
	if (rc < 0)
		goto init_probe_fail;

	CDBG("hm0356 model_id = 0x%x\n", model_id);

	/* Check if it matches it with the value in Datasheet */
	//if (model_id != HM0356_MODEL_ID) {
	//	rc = -EFAULT;
	//	goto init_probe_fail;
	//}

	/* Get version */
	//rc = hm0356_i2c_read(HM0356_I2C_READ_SLAVE_ID,
	//0x302A, &model_id, BYTE_LEN);
	//CDBG("hm0356 version reg 0x302A = 0x%x\n", model_id);

	rc = hm0356_reg_init();
	if (rc < 0)
		goto init_probe_fail;

	return rc;

init_probe_fail:
	return rc;
}

int hm0356_sensor_init(const struct msm_camera_sensor_info *data)
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

	hm0356_ctrl = kzalloc(sizeof(struct hm0356_ctrl), GFP_KERNEL);
	if (!hm0356_ctrl) {
		CDBG("hm0356_init failed!\n");
		rc = -ENOMEM;
		goto init_done;
	}

	if (data)
		hm0356_ctrl->sensordata = data;

/* FIH, Charles Huang, 2010/03/01 { */
/* [FXX_CR], Add power onoff vreg */
#ifdef CONFIG_FIH_FXX
	/* turn on power */

	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		vreg_gp1 = vreg_get(NULL, "gp1");
		vreg_set_level(vreg_gp1, 3000);
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
		vreg_set_level(vreg_rftx, 3000);
		vreg_enable(vreg_rftx);

		vreg_gp2 = vreg_get(NULL, "gp2");
		vreg_set_level(vreg_gp2, 1500);
		vreg_enable(vreg_gp2);
	}
	/* } FIH, IvanCHTang, 2010/6/1 */

	vreg_gp3 = vreg_get(NULL, "gp3");
	vreg_set_level(vreg_gp3, 2800);
	vreg_enable(vreg_gp3);
#endif
/* } FIH, Charles Huang, 2010/03/01 */

	rc = hm0356_reset(data);
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
#ifdef HM0356_USE_VFS
	if (hm0356_use_vfs_mclk_setting!=0)
		msm_camio_clk_rate_set(hm0356_use_vfs_mclk_setting);
	else
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */
#if 0/*mr7*/
	if (FIH_READ_HWID_FROM_SMEM()==CMCS_HW_VER_EVB1)
		msm_camio_clk_rate_set(12288000);
	else
#endif
		msm_camio_clk_rate_set(24000000);
	msleep(25);

	msm_camio_camif_pad_reg_reset();

	rc = hm0356_sensor_init_probe(data);
	if (rc < 0) {
		CDBG("hm0356_sensor_init failed!\n");
		goto init_fail;
	}

init_done:
	return rc;

init_fail:
	kfree(hm0356_ctrl);
	return rc;
}

static int hm0356_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&hm0356_wait_queue);
	return 0;
}

int hm0356_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	long   rc = 0;

	if (copy_from_user(
				&cfg_data,
				(void *)argp,
				sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	/* down(&hm0356_sem); */

	CDBG("hm0356_sensor_config, cfgtype = %d, mode = %d\n",
		cfg_data.cfgtype, cfg_data.mode);

	switch (cfg_data.cfgtype) {
	case CFG_SET_MODE:
		rc = hm0356_set_sensor_mode(
					cfg_data.mode);
		break;

	case CFG_SET_EFFECT:
		rc = hm0356_set_effect(
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
		rc = hm0356_set_brightness(
					cfg_data.mode,
					cfg_data.cfg.brightness);
		break;

	case CFG_SET_CONTRAST:
		rc = -EFAULT;
		break;

	case CFG_SET_EXPOSURE_MODE:
		rc = -EFAULT;
		break;

	case CFG_SET_WB:
		rc = hm0356_set_wb(
					cfg_data.mode,
					cfg_data.cfg.wb);
		break;

	case CFG_SET_ANTIBANDING:
		rc = hm0356_set_antibanding(
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

	case CFG_SET_LEDMOD:
		rc = -EFAULT;
		break;

	default:
		rc = -EINVAL;
		break;
	}

	/* up(&hm0356_sem); */

	return rc;
}

int hm0356_sensor_release(void)
{
	int rc = 0;
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	//struct mpp *mpp_22;
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
	const struct msm_camera_sensor_info *dev;
	/* down(&hm0356_sem); */
#if 1/*mr7*/
	int HWID=FIH_READ_HWID_FROM_SMEM();
#endif
	mutex_lock(&hm0356_mut);

	dev = hm0356_ctrl->sensordata;
#if 0
	rc = gpio_request(dev->sensor_reset, "hm0356");
	if (!rc) {
		rc = gpio_direction_output(dev->sensor_reset, 0);
	}
	gpio_free(dev->sensor_reset);
#endif
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		/* FIH, IvanCHTang, 2010/7/7 { */
		/* [FXX_CR], [camera] DVDD (msme2 -> mpp1) & VGA_PD (mpp22 -> gpio<108>) for FN6 PR2 - [FM6#0010] */
		if(HWID>=CMCS_FN6_PR2)
		{
			gpio_tlmm_config(GPIO_CFG(108, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
			gpio_direction_output(108, 1);
		}
		else
			mpp_config_digital_out(mpp_22, MPP_CFG(MPP_DLOGIC_LVL_MSMP, MPP_DLOGIC_OUT_CTRL_HIGH));
		/* } FIH, IvanCHTang, 2010/7/7 */
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		rc = gpio_request(dev->sensor_pwd, "hm0356");
		if (!rc) {
			rc = gpio_direction_output(dev->sensor_pwd, 1);
		}
		gpio_free(dev->sensor_pwd);
	}
	/* } FIH, IvanCHTang, 2010/6/1 */
#if 0/*mr7*/
	if (HWID>=CMCS_7627_EVB1)
	{
		/* Switch disable */
		gpio_tlmm_config(GPIO_CFG(121, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
		gpio_direction_output(121, 1);	
		msleep(10);
	}
#endif
	CDBG("[HM0356 VGA]  gpio_get_value(0) = %d\n", gpio_get_value(0));
	CDBG("[HM0356 VGA]  gpio_get_value(31) = %d\n", gpio_get_value(31));
	CDBG("[HM0356 VGA]  gpio_get_value(85) = %d\n", gpio_get_value(85));
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		;
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		CDBG("[HM0356 VGA] gpio_get_value(122) = %d\n", gpio_get_value(122));
	}
	/* } FIH, IvanCHTang, 2010/6/1 */
	   
       gpio_tlmm_config(GPIO_CFG(0, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), GPIO_ENABLE);
       gpio_tlmm_config(GPIO_CFG(85, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), GPIO_ENABLE);

/* FIH, Charles Huang, 2010/03/01 { */
/* [FXX_CR], Add power onoff vreg */
#ifdef CONFIG_FIH_FXX
	/* turn on power */

	/* FIH, IvanCHTang, 2010/6/3 { */
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
	/* } FIH, IvanCHTang, 2010/6/3 */

	vreg_gp3 = vreg_get(NULL, "gp3");
	vreg_disable(vreg_gp3);
#endif
/* } FIH, Charles Huang, 2010/03/01 */

	kfree(hm0356_ctrl);
	hm0356_ctrl = NULL;
	/* up(&hm0356_sem); */
	mutex_unlock(&hm0356_mut);
	return rc;
}

/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef HM0356_USE_VFS
void hm0356_get_param(const char *buf, size_t count, struct hm0356_i2c_reg_conf *tbl, 
	unsigned short tbl_size, int *use_setting)
{
	unsigned short waddr;
	unsigned short wdata;
	enum hm0356_width width;
	unsigned short mdelay_time;
	unsigned short mask;
	char param1[10],param2[10],param3[10];
	int read_count;
	const char *pstr;
	int vfs_index=0;
	pstr=buf;

	CDBG("count=%d\n",count);
	do
	{
		read_count=sscanf(pstr,"%4s,%2s,%2s",param1,param2,param3);

      		//CDBG("pstr=%s\n",pstr);
      		//CDBG("read_count=%d,count=%d\n",read_count,count);
		if (read_count ==3)
		{
			waddr=simple_strtoul(param1,NULL,16);
			wdata=simple_strtoul(param2,NULL,16);
			width=1;
			mdelay_time=0;
			mask=simple_strtoul(param3,NULL,16);
				
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
			
       		//CDBG("param1=%s,param2=%s,param3=%s\n",param1,param2,param3);
       		//CDBG("waddr=0x%04X,wdata=0x%04X,width=%d,mdelay_time=%d,mask=0x%04X\n",waddr,wdata,width,mdelay_time,mask);
		}else{
			tbl[vfs_index].waddr= 0xFFFF;
			tbl[vfs_index].wdata= 0xFFFF;
			tbl[vfs_index].width= 1;
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

static ssize_t hm0356_write_initreg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	hm0356_get_param(buf, count, &hm0356_vfs_init_settings_tbl[0], hm0356_vfs_init_settings_tbl_size, &hm0356_use_vfs_init_setting);
	return count;
}

static ssize_t hm0356_write_oemtreg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	hm0356_get_param(buf, count, &hm0356_vfs_oem_settings_tbl[0], hm0356_vfs_oem_settings_tbl_size, &hm0356_use_vfs_oem_setting);
	return count;
}

static ssize_t hm0356_write_previewreg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	hm0356_get_param(buf, count, &hm0356_vfs_preview_settings_tbl[0], hm0356_vfs_preview_settings_tbl_size, &hm0356_use_vfs_preview_setting);
	return count;
}

static ssize_t hm0356_write_snapreg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	hm0356_get_param(buf, count, &hm0356_vfs_snap_settings_tbl[0], hm0356_vfs_snap_settings_tbl_size, &hm0356_use_vfs_snap_setting);
	return count;
}

static ssize_t hm0356_write_snapaereg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	hm0356_get_param(buf, count, &hm0356_vfs_snapae_settings_tbl[0], hm0356_vfs_snapae_settings_tbl_size, &hm0356_use_vfs_snapae_setting);
	return count;
}

static ssize_t hm0356_write_iqreg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	hm0356_get_param(buf, count, &hm0356_vfs_iq_settings_tbl[0], hm0356_vfs_iq_settings_tbl_size, &hm0356_use_vfs_iq_setting);
	return count;
}

static ssize_t hm0356_write_lensreg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	hm0356_get_param(buf, count, &hm0356_vfs_lens_settings_tbl[0], hm0356_vfs_lens_settings_tbl_size, &hm0356_use_vfs_lens_setting);
	return count;
}

static ssize_t hm0356_write_reg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	long rc = 0;

	hm0356_get_param(buf, count, &hm0356_vfs_writereg_settings_tbl[0], hm0356_vfs_writereg_settings_tbl_size, &hm0356_use_vfs_writereg_setting);
	if (hm0356_use_vfs_writereg_setting)
	{
		rc = hm0356_i2c_write_table(&hm0356_vfs_writereg_settings_tbl[0],
			hm0356_vfs_writereg_settings_tbl_size);
		hm0356_use_vfs_writereg_setting =0;
	}
	return count;
}

static ssize_t hm0356_setrange(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	hm0356_get_param(buf, count, &hm0356_vfs_getreg_settings_tbl[0], hm0356_vfs_getreg_settings_tbl_size, &hm0356_use_vfs_getreg_setting);
	return count;
}

static ssize_t hm0356_getrange(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i,rc;
	char *str = buf;

	if (hm0356_use_vfs_getreg_setting)
	{
		for (i=0;i<=hm0356_vfs_getreg_settings_tbl_size;i++)
		{
			if (hm0356_vfs_getreg_settings_tbl[i].waddr==0xFFFF)
				break;

			rc = hm0356_i2c_read(HM0356_I2C_READ_SLAVE_ID,
				hm0356_vfs_getreg_settings_tbl[i].waddr, &(hm0356_vfs_getreg_settings_tbl[i].wdata), BYTE_LEN);
			CDBG("hm0356 reg 0x%4X = 0x%2X\n", hm0356_vfs_getreg_settings_tbl[i].waddr, hm0356_vfs_getreg_settings_tbl[i].wdata);

			str += sprintf(str, "%04X,%2X,%2X\n", hm0356_vfs_getreg_settings_tbl[i].waddr, 
				hm0356_vfs_getreg_settings_tbl[i].wdata, 
				hm0356_vfs_getreg_settings_tbl[i].mask);

			if (rc <0)
				break;
		}
	}
	return (str - buf);
}

static ssize_t hm0356_setmclk(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf,"%d",&hm0356_use_vfs_mclk_setting);
	return count;
}

static ssize_t hm0356_getmclk(struct device *dev, struct device_attribute *attr, char *buf)
{
	return (sprintf(buf,"%d\n",hm0356_use_vfs_mclk_setting));
}

static ssize_t hm0356_setmultiple(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf,"%d",&hm0356_use_vfs_multiple_setting);
	return count;
}

static ssize_t hm0356_getmultiple(struct device *dev, struct device_attribute *attr, char *buf)
{
	return (sprintf(buf,"%d\n",hm0356_use_vfs_multiple_setting));
}

static ssize_t hm0356_setflashtime(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf,"%d",&hm0356_use_vfs_flashtime_setting);
	return count;
}

static ssize_t hm0356_getflashtime(struct device *dev, struct device_attribute *attr, char *buf)
{
	return (sprintf(buf,"%d\n",hm0356_use_vfs_flashtime_setting));
}

DEVICE_ATTR(initreg_hm0356, 0666, NULL, hm0356_write_initreg);
DEVICE_ATTR(oemreg_hm0356, 0666, NULL, hm0356_write_oemtreg);
DEVICE_ATTR(previewreg_hm0356, 0666, NULL, hm0356_write_previewreg);
DEVICE_ATTR(snapreg_hm0356, 0666, NULL, hm0356_write_snapreg);
DEVICE_ATTR(snapregae_hm0356, 0666, NULL, hm0356_write_snapaereg);
DEVICE_ATTR(iqreg_hm0356, 0666, NULL, hm0356_write_iqreg);
DEVICE_ATTR(lensreg_hm0356, 0666, NULL, hm0356_write_lensreg);
DEVICE_ATTR(writereg_hm0356, 0666, NULL, hm0356_write_reg);
DEVICE_ATTR(getreg_hm0356, 0666, hm0356_getrange, hm0356_setrange);
DEVICE_ATTR(mclk_hm0356, 0666, hm0356_getmclk, hm0356_setmclk);
DEVICE_ATTR(multiple_hm0356, 0666, hm0356_getmultiple, hm0356_setmultiple);
DEVICE_ATTR(flashtime_hm0356, 0666, hm0356_getflashtime, hm0356_setflashtime);

static int create_attributes(struct i2c_client *client)
{
	int rc;

	dev_attr_initreg_hm0356.attr.name = HM0356_INITREG;
	dev_attr_oemreg_hm0356.attr.name = HM0356_OEMREG;
	dev_attr_previewreg_hm0356.attr.name = HM0356_PREVIEWREG;
	dev_attr_snapreg_hm0356.attr.name = HM0356_SNAPREG;
	dev_attr_snapregae_hm0356.attr.name = HM0356_SNAPAEREG;
	dev_attr_iqreg_hm0356.attr.name = HM0356_IQREG;
	dev_attr_lensreg_hm0356.attr.name = HM0356_LENSREG;
	dev_attr_writereg_hm0356.attr.name = HM0356_WRITEREG;
	dev_attr_getreg_hm0356.attr.name = HM0356_GETREG;
	dev_attr_mclk_hm0356.attr.name = HM0356_MCLK;
	dev_attr_multiple_hm0356.attr.name = HM0356_MULTIPLE;
	dev_attr_flashtime_hm0356.attr.name = HM0356_FLASHTIME;

	rc = device_create_file(&client->dev, &dev_attr_initreg_hm0356);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create hm0356 attribute \"initreg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}

	rc = device_create_file(&client->dev, &dev_attr_oemreg_hm0356);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create hm0356 attribute \"oemreg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}

	rc = device_create_file(&client->dev, &dev_attr_previewreg_hm0356);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create hm0356 attribute \"previewreg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}

	rc = device_create_file(&client->dev, &dev_attr_snapreg_hm0356);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create hm0356 attribute \"snapreg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_snapregae_hm0356);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create hm0356 attribute \"snapregae\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_iqreg_hm0356);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create hm0356 attribute \"iqreg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_lensreg_hm0356);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create hm0356 attribute \"lensreg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_writereg_hm0356);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create hm0356 attribute \"writereg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_getreg_hm0356);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create hm0356 attribute \"getreg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_mclk_hm0356);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create hm0356 attribute \"mclk\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_multiple_hm0356);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create hm0356 attribute \"multiple\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_flashtime_hm0356);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create hm0356 attribute \"flashtime\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	return rc;
}

static int remove_attributes(struct i2c_client *client)
{
	device_remove_file(&client->dev, &dev_attr_initreg_hm0356);
	device_remove_file(&client->dev, &dev_attr_oemreg_hm0356);
	device_remove_file(&client->dev, &dev_attr_previewreg_hm0356);
	device_remove_file(&client->dev, &dev_attr_snapreg_hm0356);
	device_remove_file(&client->dev, &dev_attr_snapregae_hm0356);
	device_remove_file(&client->dev, &dev_attr_iqreg_hm0356);
	device_remove_file(&client->dev, &dev_attr_lensreg_hm0356);
	device_remove_file(&client->dev, &dev_attr_writereg_hm0356);
	device_remove_file(&client->dev, &dev_attr_getreg_hm0356);
	device_remove_file(&client->dev, &dev_attr_mclk_hm0356);
	device_remove_file(&client->dev, &dev_attr_multiple_hm0356);
	device_remove_file(&client->dev, &dev_attr_flashtime_hm0356);

	return 0;
}
#endif /* HM0356_USE_VFS */
#endif
/* } FIH, Charles Huang, 2009/06/24 */

static int hm0356_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int rc = 0;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	hm0356_sensorw =
		kzalloc(sizeof(struct hm0356_work), GFP_KERNEL);

	if (!hm0356_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, hm0356_sensorw);
	hm0356_init_client(client);
	hm0356_client = client;
	
	CDBG("hm0356_probe successed!\n");

	return 0;

probe_failure:
	kfree(hm0356_sensorw);
	hm0356_sensorw = NULL;
	CDBG("hm0356_probe failed!\n");
	return rc;
}

static int __exit hm0356_i2c_remove(struct i2c_client *client)
{
	struct hm0356_work *sensorw = i2c_get_clientdata(client);

/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef HM0356_USE_VFS
	remove_attributes(client);
#endif /* HM0356_USE_VFS */
#endif
/* } FIH, Charles Huang, 2009/06/24 */

	free_irq(client->irq, sensorw);
	hm0356_client = NULL;
	hm0356_sensorw = NULL;
	kfree(sensorw);
	return 0;
}

#ifdef CONFIG_PM
static int hm0356_suspend(struct i2c_client *client, pm_message_t mesg)
{
/* FIH, Charles Huang, 2009/06/25 { */
/* [FXX_CR], suspend/resume for pm */
#ifdef CONFIG_FIH_FXX
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	//struct mpp *mpp_22;
	int HWID=FIH_READ_HWID_FROM_SMEM();
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		/* FIH, IvanCHTang, 2010/7/7 { */
		/* [FXX_CR], [camera] DVDD (msme2 -> mpp1) & VGA_PD (mpp22 -> gpio<108>) for FN6 PR2 - [FM6#0010] */
		if(HWID>=CMCS_FN6_PR2)
		{
			gpio_tlmm_config(GPIO_CFG(108, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
			gpio_direction_output(108, 1);
		}
		else
			mpp_config_digital_out(mpp_22, MPP_CFG(MPP_DLOGIC_LVL_MSMP, MPP_DLOGIC_OUT_CTRL_HIGH));
		/* } FIH, IvanCHTang, 2010/7/7 */
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		/* sensor_pwd pin gpio122 */
		gpio_direction_output(122,1);
	}
	/* } FIH, IvanCHTang, 2010/6/1 */
#endif
/* } FIH, Charles Huang, 2009/06/25 */
	return 0;
}

static int hm0356_resume(struct i2c_client *client)
{
/* FIH, Charles Huang, 2009/06/25 { */
/* [FXX_CR], suspend/resume for pm */
#ifdef CONFIG_FIH_FXX
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	//struct mpp *mpp_22;
	int HWID=FIH_READ_HWID_FROM_SMEM();
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		/* FIH, IvanCHTang, 2010/7/7 { */
		/* [FXX_CR], [camera] DVDD (msme2 -> mpp1) & VGA_PD (mpp22 -> gpio<108>) for FN6 PR2 - [FM6#0010] */
		if(HWID>=CMCS_FN6_PR2)
		{
			gpio_tlmm_config(GPIO_CFG(108, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
			gpio_direction_output(108, 1);
		}
		else
			mpp_config_digital_out(mpp_22, MPP_CFG(MPP_DLOGIC_LVL_MSMP, MPP_DLOGIC_OUT_CTRL_HIGH));
		/* } FIH, IvanCHTang, 2010/7/7 */
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		/* sensor_pwd pin gpio122 */
		/* Handle by sensor initialization */
		/* workable setting for waste power while resuming */
		gpio_direction_output(122,1);
	}
	/* } FIH, IvanCHTang, 2010/6/1 */
#endif
/* } FIH, Charles Huang, 2009/06/25 */
	return 0;
}
#else
# define hm0356_suspend NULL
# define hm0356_resume  NULL
#endif

static const struct i2c_device_id hm0356_i2c_id[] = {
	{ "hm0356", 0},
	{ },
};

static struct i2c_driver hm0356_i2c_driver = {
	.id_table = hm0356_i2c_id,
	.probe  = hm0356_i2c_probe,
	.remove = __exit_p(hm0356_i2c_remove),
	.suspend  	= hm0356_suspend,
	.resume   	= hm0356_resume,
	.driver = {
		.name = "hm0356",
	},
};

static int hm0356_sensor_probe(const struct msm_camera_sensor_info *info,
				struct msm_sensor_ctrl *s)
{
	int rc = i2c_add_driver(&hm0356_i2c_driver);
	uint16_t model_id = 0;
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	//struct mpp *mpp_22;
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

	if (rc < 0 || hm0356_client == NULL) {
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
		vreg_set_level(vreg_gp1, 3000);
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
		vreg_set_level(vreg_rftx, 3000);
		vreg_enable(vreg_rftx);

		vreg_gp2 = vreg_get(NULL, "gp2");
		vreg_set_level(vreg_gp2, 1500);
		vreg_enable(vreg_gp2);
	}
	/* } FIH, IvanCHTang, 2010/6/1 */

	vreg_gp3 = vreg_get(NULL, "gp3");
	vreg_set_level(vreg_gp3, 2800);
	vreg_enable(vreg_gp3);
#endif
/* } FIH, Charles Huang, 2010/03/01 */

	rc = hm0356_reset(info);
	if (rc < 0) {
		CDBG("reset failed!\n");
		goto probe_fail;
	}

	/* EVB CAMIF cannont handle in 24MHz */
	/* EVB use 12.8MHz */
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef HM0356_USE_VFS
	if (hm0356_use_vfs_mclk_setting!=0)
		msm_camio_clk_rate_set(hm0356_use_vfs_mclk_setting);
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
	/* Read REG_HM0356_MODEL_ID_HI & REG_HM0356_MODEL_ID_LO */
	rc = hm0356_i2c_read(HM0356_I2C_READ_SLAVE_ID,
		REG_HM0356_MODEL_ID_HI, &model_id, BYTE_LEN);
	if (rc < 0)
		goto probe_fail;

	CDBG("hm0356 model_id = 0x%x\n", model_id);

	rc = hm0356_i2c_read(HM0356_I2C_READ_SLAVE_ID,
		REG_HM0356_MODEL_ID_LO, &model_id, BYTE_LEN);
	if (rc < 0)
		goto probe_fail;

	CDBG("hm0356 model_id = 0x%x\n", model_id);

	/* Check if it matches it with the value in Datasheet */
	//if (model_id != HM0356_MODEL_ID) {
	//	rc = -EFAULT;
	//	goto probe_fail;
	//}
#if 0
	rc = gpio_request(info->sensor_reset, "hm0356");
	if (!rc) {
		rc = gpio_direction_output(info->sensor_reset, 0);
	}
	gpio_free(info->sensor_reset);
#endif
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		/* FIH, IvanCHTang, 2010/7/7 { */
		/* [FXX_CR], [camera] DVDD (msme2 -> mpp1) & VGA_PD (mpp22 -> gpio<108>) for FN6 PR2 - [FM6#0010] */
		if(HWID>=CMCS_FN6_PR2)
		{
			gpio_tlmm_config(GPIO_CFG(108, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
			gpio_direction_output(108, 1);
		}
		else
			mpp_config_digital_out(mpp_22, MPP_CFG(MPP_DLOGIC_LVL_MSMP, MPP_DLOGIC_OUT_CTRL_HIGH));
		/* } FIH, IvanCHTang, 2010/7/7 */
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		rc = gpio_request(info->sensor_pwd, "hm0356");
		if (!rc) {
			rc = gpio_direction_output(info->sensor_pwd, 1);
		}
		gpio_free(info->sensor_pwd);
	}
	/* } FIH, IvanCHTang, 2010/6/1 */
#if 0/*mr7*/
/* FIH, Charles Huang, 2009/06/09 { */
/* [FXX_CR], pull pwdn pin high */
#ifdef CONFIG_FIH_FXX
	if (HWID>=CMCS_7627_EVB1)
	{
		/* Switch disable */
		gpio_tlmm_config(GPIO_CFG(121, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
		gpio_direction_output(121,1);
	}
#endif
/* } FIH, Charles Huang, 2009/06/09 */
#endif
	CDBG("[HM0356 VGA]  gpio_get_value(0) = %d\n", gpio_get_value(0));
	CDBG("[HM0356 VGA]  gpio_get_value(31) = %d\n", gpio_get_value(31));
	CDBG("[HM0356 VGA]  gpio_get_value(85) = %d\n", gpio_get_value(85));
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		CDBG("[HM0356 VGA]  gpio_get_value(108) = %d\n", gpio_get_value(108));;
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		CDBG("[HM0356 VGA]  gpio_get_value(122) = %d\n", gpio_get_value(122));
	}
	/* } FIH, IvanCHTang, 2010/6/1 */
	   
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef HM0356_USE_VFS
	rc = create_attributes(hm0356_client);
	if (rc < 0) {
		dev_err(&hm0356_client->dev, "%s: create attributes failed!! <%d>", __func__, rc);
		goto probe_done;
	}
#endif /* HM0356_USE_VFS */
#endif
/* } FIH, Charles Huang, 2009/06/24 */

	s->s_init = hm0356_sensor_init;
	s->s_release = hm0356_sensor_release;
	s->s_config  = hm0356_sensor_config;

probe_done:
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

	dev_info(&hm0356_client->dev, "probe_done %s %s:%d\n", __FILE__, __func__, __LINE__);
	return rc;

probe_fail:
	dev_info(&hm0356_client->dev, "probe_fail %s %s:%d\n", __FILE__, __func__, __LINE__);
#if 0
	gpio_tlmm_config(GPIO_CFG(0, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
	gpio_direction_output(0,0);
#endif

#if 0/*mr7*/
	if (HWID>=CMCS_7627_EVB1)
	{
		/* Switch disable */
		gpio_tlmm_config(GPIO_CFG(121, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
		gpio_direction_output(121,1);
	}
#endif
	CDBG("[HM0356 VGA]  gpio_get_value(0) = %d\n", gpio_get_value(0));
	CDBG("[HM0356 VGA]  gpio_get_value(31) = %d\n", gpio_get_value(31));
	CDBG("[HM0356 VGA]  gpio_get_value(85) = %d\n", gpio_get_value(85));
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		CDBG("[HM0356 VGA]  gpio_get_value(108) = %d\n", gpio_get_value(108));
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		gpio_tlmm_config(GPIO_CFG(122, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
		gpio_direction_output(122,1);
		CDBG("[HM0356 VGA]  gpio_get_value(122) = %d\n", gpio_get_value(122));
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
		{
			mpp_config_digital_out(mpp_1, MPP_CFG(MPP_DLOGIC_LVL_MSMP, MPP_DLOGIC_OUT_CTRL_LOW));

			// CAMIF_VGA_PDN : HIGH
			gpio_tlmm_config(GPIO_CFG(108, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
			gpio_direction_output(108, 1);
		}
		else
		{
			vreg_msme2 = vreg_get(NULL, "msme2");
			vreg_disable(vreg_msme2);

			// CAMIF_VGA_PDN : HIGH
			mpp_config_digital_out(mpp_22, MPP_CFG(MPP_DLOGIC_LVL_MSMP, MPP_DLOGIC_OUT_CTRL_HIGH));
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

static int __hm0356_probe(struct platform_device *pdev)
{
	return msm_camera_drv_start(pdev, hm0356_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __hm0356_probe,
	.driver = {
		.name = "msm_camera_hm0356",
		.owner = THIS_MODULE,
	},
};

static int __init hm0356_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(hm0356_init);
