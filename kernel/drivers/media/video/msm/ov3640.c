/*
 *     ov3640.c - Camera Sensor Driver
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
#include "ov3640.h"

#include <mach/msm_iomap.h>
#include <mach/msm_smd.h>
/* FIH, IvanCHTang, 2010/6/30 { */
/* [FXX_CR], [camera] DVDD (msme2 -> mpp1) & VGA_PD (mpp22 -> gpio<108>) for FN6 PR2 - [FM6#0010] */
#ifdef CONFIG_FIH_FXX
#include <mach/mpp.h>
static unsigned mpp_1 = 0;
#endif
/* } FIH, IvanCHTang, 2010/6/30 */
/* FIH, Charles Huang, 2010/03/01 { */
/* [FXX_CR], Add power onoff vreg */
#ifdef CONFIG_FIH_FXX
#include <mach/vreg.h>
#endif
/* } FIH, Charles Huang, 2010/03/01 */
/* FIH, Charles Huang, 2010/02/26 { */
/* [FXX_CR], move to kthread and speed up launching camera app */
#ifdef CONFIG_FIH_FXX
DECLARE_COMPLETION(ov3640_firmware_init);
#endif
/* } FIH, Charles Huang, 2010/02/26 */

DEFINE_MUTEX(ov3640_mut);

/* Micron OV3640 Registers and their values */
/* Sensor Core Registers */
#define  REG_OV3640_MODEL_ID_HI 0x300A
#define  REG_OV3640_MODEL_ID_LO 0x300B
#define  OV3640_MODEL_ID     0x364C
#define  OV3640_I2C_READ_SLAVE_ID     0x78 >> 1  
#define  OV3640_I2C_WRITE_SLAVE_ID     0x79 >> 1  
/* FIH, Charles Huang, 2009/07/29 { */
/* [FXX_CR], Calculate AEC */
#ifdef CONFIG_FIH_FXX
#define OV3640_CAPTURE_FRAMERATE 7.5
#define OV3640_PREVIEW_FRAMERATE 30
int ov3640_m_60Hz=0;
#endif
/* } FIH, Charles Huang, 2009/07/29 */

struct ov3640_work {
	struct work_struct work;
};

static struct  ov3640_work *ov3640_sensorw;
static struct  i2c_client *ov3640_client;

struct ov3640_ctrl {
	const struct msm_camera_sensor_info *sensordata;
};

extern void brightness_onoff(int on);
static struct ov3640_ctrl *ov3640_ctrl;

/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#define OV3640_USE_VFS
#ifdef OV3640_USE_VFS
#define OV3640_INITREG "initreg"
#define OV3640_OEMREG "oemreg"
#define OV3640_PREVIEWREG "previewreg"
#define OV3640_SNAPREG "snapreg"
#define OV3640_SNAPAEREG "snapaereg"
#define OV3640_IQREG "iqreg"
#define OV3640_LENSREG "lensreg"
#define OV3640_WRITEREG "writereg"
#define OV3640_GETREG "getreg"
#define OV3640_MCLK "mclk"
#define OV3640_MULTIPLE "multiple"
#define OV3640_FLASHTIME "flashtime"

/* MAX buf is ???? */
#define OV3640_MAX_VFS_INIT_INDEX 330
int ov3640_use_vfs_init_setting=0;
struct ov3640_i2c_reg_conf ov3640_vfs_init_settings_tbl[OV3640_MAX_VFS_INIT_INDEX];
uint16_t ov3640_vfs_init_settings_tbl_size= ARRAY_SIZE(ov3640_vfs_init_settings_tbl);

#define OV3640_MAX_VFS_OEM_INDEX 330
int ov3640_use_vfs_oem_setting=0;
struct ov3640_i2c_reg_conf ov3640_vfs_oem_settings_tbl[OV3640_MAX_VFS_OEM_INDEX];
uint16_t ov3640_vfs_oem_settings_tbl_size= ARRAY_SIZE(ov3640_vfs_oem_settings_tbl);

#define OV3640_MAX_VFS_PREVIEW_INDEX 330
int ov3640_use_vfs_preview_setting=0;
struct ov3640_i2c_reg_conf ov3640_vfs_preview_settings_tbl[OV3640_MAX_VFS_PREVIEW_INDEX];
uint16_t ov3640_vfs_preview_settings_tbl_size= ARRAY_SIZE(ov3640_vfs_preview_settings_tbl);

#define OV3640_MAX_VFS_SNAP_INDEX 330
int ov3640_use_vfs_snap_setting=0;
struct ov3640_i2c_reg_conf ov3640_vfs_snap_settings_tbl[OV3640_MAX_VFS_SNAP_INDEX];
uint16_t ov3640_vfs_snap_settings_tbl_size= ARRAY_SIZE(ov3640_vfs_snap_settings_tbl);

#define OV3640_MAX_VFS_SNAPAE_INDEX 330
int ov3640_use_vfs_snapae_setting=0;
struct ov3640_i2c_reg_conf ov3640_vfs_snapae_settings_tbl[OV3640_MAX_VFS_SNAPAE_INDEX];
uint16_t ov3640_vfs_snapae_settings_tbl_size= ARRAY_SIZE(ov3640_vfs_snapae_settings_tbl);

#define OV3640_MAX_VFS_IQ_INDEX 330
int ov3640_use_vfs_iq_setting=0;
struct ov3640_i2c_reg_conf ov3640_vfs_iq_settings_tbl[OV3640_MAX_VFS_IQ_INDEX];
uint16_t ov3640_vfs_iq_settings_tbl_size= ARRAY_SIZE(ov3640_vfs_iq_settings_tbl);

#define OV3640_MAX_VFS_LENS_INDEX 330
int ov3640_use_vfs_lens_setting=0;
struct ov3640_i2c_reg_conf ov3640_vfs_lens_settings_tbl[OV3640_MAX_VFS_LENS_INDEX];
uint16_t ov3640_vfs_lens_settings_tbl_size= ARRAY_SIZE(ov3640_vfs_lens_settings_tbl);

#define OV3640_MAX_VFS_WRITEREG_INDEX 330
int ov3640_use_vfs_writereg_setting=0;
struct ov3640_i2c_reg_conf ov3640_vfs_writereg_settings_tbl[OV3640_MAX_VFS_IQ_INDEX];
uint16_t ov3640_vfs_writereg_settings_tbl_size= ARRAY_SIZE(ov3640_vfs_writereg_settings_tbl);

#define OV3640_MAX_VFS_GETREG_INDEX 330
int ov3640_use_vfs_getreg_setting=0;
struct ov3640_i2c_reg_conf ov3640_vfs_getreg_settings_tbl[OV3640_MAX_VFS_GETREG_INDEX];
uint16_t ov3640_vfs_getreg_settings_tbl_size= ARRAY_SIZE(ov3640_vfs_getreg_settings_tbl);

int ov3640_use_vfs_mclk_setting=0;
int ov3640_use_vfs_multiple_setting=0;
int ov3640_use_vfs_flashtime_setting=0;
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */

static DECLARE_WAIT_QUEUE_HEAD(ov3640_wait_queue);
DECLARE_MUTEX(ov3640_sem);

/*=============================================================*/
static int ov3640_reset(const struct msm_camera_sensor_info *dev)
{
	int rc = 0;
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
		gpio_direction_output(85, 0);	
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
	rc = gpio_request(dev->sensor_pwd, "ov3640");
	if (!rc) {
		rc = gpio_direction_output(dev->sensor_pwd, 0);
	}
	gpio_free(dev->sensor_pwd);
	
	rc = gpio_request(dev->sensor_reset, "ov3640");
	if (!rc) {
		rc = gpio_direction_output(dev->sensor_reset, 0);
		msleep(20);
		rc = gpio_direction_output(dev->sensor_reset, 1);
	}
	gpio_free(dev->sensor_reset);
	
	CDBG("[OV3640 3M] gpio_get_value(0) = %d\n", gpio_get_value(0));
	CDBG("[OV3640 3M] gpio_get_value(31) = %d\n", gpio_get_value(31));
	CDBG("[OV3640 3M] gpio_get_value(85) = %d\n", gpio_get_value(85));
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		;
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		CDBG("[OV3640 3M]  gpio_get_value(122) = %d\n", gpio_get_value(122));
	}
	/* } FIH, IvanCHTang, 2010/6/1 */
	   
	return rc;
}

static int32_t ov3640_i2c_txdata(unsigned short saddr,
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

	if (i2c_transfer(ov3640_client->adapter, msg, 1) < 0) {
		printk(KERN_ERR "ov3640_i2c_txdata failed, try again!\n");
		if (i2c_transfer(ov3640_client->adapter, msg, 1) < 0) {
			printk(KERN_ERR "ov3640_i2c_txdata failed\n");
			CDBG("ov3640_i2c_txdata failed\n");
			return -EIO;
		}
	}

	return 0;
}

static int32_t ov3640_i2c_write(unsigned short saddr,
	unsigned short waddr, unsigned short wdata, enum ov3640_width width)
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

		rc = ov3640_i2c_txdata(saddr, buf, 4);
	}
		break;

	case BYTE_LEN: {
		buf[0] = (waddr & 0xFF00)>>8;
		buf[1] = (waddr & 0x00FF);
		buf[2] = (uint8_t) wdata;
		rc = ov3640_i2c_txdata(saddr, buf, 3);
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

static int32_t ov3640_i2c_rxdata(unsigned short saddr,
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

	if (i2c_transfer(ov3640_client->adapter, msgs, 2) < 0) {
		printk(KERN_ERR "ov3640_i2c_rxdata failed, try again!\n");
		if (i2c_transfer(ov3640_client->adapter, msgs, 2) < 0) {
			printk(KERN_ERR "ov3640_i2c_rxdata failed!\n");
			CDBG("ov3640_i2c_rxdata failed!\n");
			return -EIO;
		}
	}

	return 0;
}

static int32_t ov3640_i2c_read(unsigned short   saddr,
	unsigned short raddr, unsigned short *rdata, enum ov3640_width width)
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

		rc = ov3640_i2c_rxdata(saddr, buf, 2);
		if (rc < 0)
			return rc;

		*rdata = buf[0] << 8 | buf[1];
	}
		break;
	case BYTE_LEN: {
		buf[0] = (raddr & 0xFF00)>>8;
		buf[1] = (raddr & 0x00FF);

		rc = ov3640_i2c_rxdata(saddr, buf, 1);
		if (rc < 0)
			return rc;

		*rdata = buf[0];
	}
		break;

	default:
		break;
	}

	if (rc < 0)
		CDBG("ov3640_i2c_read failed!\n");

	return rc;
}

void ov3640_set_value_by_bitmask(uint16_t bitset, uint16_t mask, uint16_t  *new_value)
{
	uint16_t org;

	org= *new_value;
	*new_value = (org&~mask) | (bitset & mask);
}

static int32_t ov3640_i2c_write_table(
	struct ov3640_i2c_reg_conf const *reg_conf_tbl,
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
			rc = ov3640_i2c_write(OV3640_I2C_WRITE_SLAVE_ID,
				reg_conf_tbl->waddr, reg_conf_tbl->wdata,
				reg_conf_tbl->width);
		}else{
			uint16_t reg_value = 0;
			rc = ov3640_i2c_read(OV3640_I2C_READ_SLAVE_ID,
				reg_conf_tbl->waddr, &reg_value, BYTE_LEN);
			ov3640_set_value_by_bitmask(reg_conf_tbl->wdata,reg_conf_tbl->mask,&reg_value);
			rc = ov3640_i2c_write(OV3640_I2C_WRITE_SLAVE_ID,
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
static int32_t ov3640_set_lens_roll_off(void)
{
	int32_t rc = 0;
	rc = ov3640_i2c_write_table(&ov3640_regs.rftbl[0],
		ov3640_regs.rftbl_size);
	return rc;
}
#endif

static long ov3640_reg_init(void)
{
#if 0
	int32_t i;
#endif
	long rc;
	uint16_t lens_id = 0;
#if 0
	/* PLL Setup Start */
	rc = ov3640_i2c_write_table(&mt9d112_regs.plltbl[0],
		ov3640_regs.plltbl_size);

	if (rc < 0)
		return rc;
	/* PLL Setup End   */
#endif

	/* Configure sensor for Preview mode and Snapshot mode */
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef OV3640_USE_VFS
	if (ov3640_use_vfs_init_setting)
		rc = ov3640_i2c_write_table(&ov3640_vfs_init_settings_tbl[0],
			ov3640_vfs_init_settings_tbl_size);
	else
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */
		rc = ov3640_i2c_write_table(&ov3640_regs.init_settings_tbl[0],
			ov3640_regs.init_settings_tbl_size);

	if (rc < 0)
		return rc;
/* FIH, Charles Huang, 2010/02/26 { */
/* [FXX_CR], move to kthread and speed up launching camera app */
#ifndef CONFIG_FIH_FXX
	rc = ov3640_i2c_write_table(&ov3640_regs.af_init_settings_tbl[0],
		ov3640_regs.af_init_settings_tbl_size);

	if (rc < 0)
		return rc;
#endif
/* } FIH, Charles Huang, 2010/02/26 */

	msleep(10);

	/* Configure sensor for image quality settings */
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef OV3640_USE_VFS
	if (ov3640_use_vfs_iq_setting)
		rc = ov3640_i2c_write_table(&ov3640_vfs_iq_settings_tbl[0],
			ov3640_vfs_iq_settings_tbl_size);
	else
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */
	rc = ov3640_i2c_write_table(&ov3640_regs.iq_settings_tbl[0],
		ov3640_regs.iq_settings_tbl_size);

	if (rc < 0)
		return rc;

/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef OV3640_USE_VFS
	if (ov3640_use_vfs_lens_setting)
		rc = ov3640_i2c_write_table(&ov3640_vfs_lens_settings_tbl[0],
			ov3640_vfs_lens_settings_tbl_size);
	else
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */
	if (0){
		rc = ov3640_i2c_read(OV3640_I2C_READ_SLAVE_ID,
			0x3D06, &lens_id, BYTE_LEN);

		if (rc < 0)
			return rc;

		if (lens_id==0x47)
			rc = ov3640_i2c_write_table(&ov3640_regs.gseolens_settings_tbl[0],
				ov3640_regs.gseolens_settings_tbl_size);
		else if (lens_id==0x46)
			rc = ov3640_i2c_write_table(&ov3640_regs.grdlens_settings_tbl[0],
				ov3640_regs.grdlens_settings_tbl_size);
		else
			rc = ov3640_i2c_write_table(&ov3640_regs.gseolens_settings_tbl[0],
				ov3640_regs.gseolens_settings_tbl_size);
	}
	if (rc < 0)
		return rc;

	/* Configure sensor for customer settings */
#if 0
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef OV3640_USE_VFS
	if (ov3640_use_vfs_oem_setting)
		rc = ov3640_i2c_write_table(&vfs_oem_settings_tbl[0],
			vfs_oem_settings_tbl_size);
	else
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */
	rc = ov3640_i2c_write_table(&ov3640_regs.oem_settings_tbl[0],
		ov3640_regs.oem_settings_tbl_size);

	if (rc < 0)
		return rc;
#endif
/* FIH, Charles Huang, 2010/02/26 { */
/* [FXX_CR], move to kthread and speed up launching camera app */
#ifdef CONFIG_FIH_FXX
	complete(&ov3640_firmware_init);
#endif
/* } FIH, Charles Huang, 2010/02/26 */
#if 0
	/* Configure for Noise Reduction, Saturation and Aperture Correction */
	array_length = ov3640_regs.noise_reduction_reg_settings_;

	for (i = 0; i < array_length; i++) {

		rc = ov3640_i2c_write(ov3640_client->addr,
		ov3640_regs.noise_reduction_reg_settings[i].register_address,
		ov3640_regs.noise_reduction_reg_settings[i].register_value,
			WORD_LEN);

		if (rc < 0)
			return rc;
	}

	/* Set Color Kill Saturation point to optimum value */
	rc =
	ov3640_i2c_write(ov3640_client->addr,
	0x35A4,
	0x0593,
	WORD_LEN);
	if (rc < 0)
		return rc;

	rc = ov3640_i2c_write_table(&ov3640_regs.stbl[0],
		ov3640_regs.stbl_size);
	if (rc < 0)
		return rc;

	rc = ov3640_set_lens_roll_off();
	if (rc < 0)
		return rc;
#endif

	return 0;
}

static long ov3640_set_sensor_mode(int mode)
{
	long rc = 0;

	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		/* Configure sensor for Preview mode */
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef OV3640_USE_VFS
		if (ov3640_use_vfs_preview_setting)
			rc = ov3640_i2c_write_table(&ov3640_vfs_preview_settings_tbl[0],
				ov3640_vfs_preview_settings_tbl_size);
		else
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */
			rc = ov3640_i2c_write_table(&ov3640_regs.preview_settings_tbl[0],
				ov3640_regs.preview_settings_tbl_size);

		if (rc < 0)
			return rc;

	/* Configure sensor for customer settings */
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef OV3640_USE_VFS
		if (ov3640_use_vfs_oem_setting)
			rc = ov3640_i2c_write_table(&ov3640_vfs_oem_settings_tbl[0],
				ov3640_vfs_oem_settings_tbl_size);
		else
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */
			rc = ov3640_i2c_write_table(&ov3640_regs.oem_settings_tbl[0],
				ov3640_regs.oem_settings_tbl_size);

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
#ifdef OV3640_USE_VFS
		if (ov3640_use_vfs_snap_setting)
			rc = ov3640_i2c_write_table(&ov3640_vfs_snap_settings_tbl[0],
				ov3640_vfs_snap_settings_tbl_size);
		else
#endif
#endif
/* } FIH, Charles Huang, 2009/06/24 */
			rc = ov3640_i2c_write_table(&ov3640_regs.snapshot_settings_tbl[0],
				ov3640_regs.snapshot_settings_tbl_size);

		if (rc < 0)
			return rc;

		msleep(5);


		break;

	default:
		return -EFAULT;
	}

	return 0;
}

static long ov3640_set_effect(int mode, int effect)
{
	long rc = 0;

	CDBG("ov3640_set_effect, mode = %d, effect = %d\n",
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
		struct ov3640_i2c_reg_conf const ov3640_effect_off_tbl[] = {
			{0x3355, 0x0000, BYTE_LEN, 0, 0x0058},
		};

		rc = ov3640_i2c_write_table(&ov3640_effect_off_tbl[0],
				ARRAY_SIZE(ov3640_effect_off_tbl));

		if (rc < 0)
			return rc;
	}
			break;

	case CAMERA_EFFECT_MONO: {//B&W
		struct ov3640_i2c_reg_conf const ov3640_effect_mono_tbl[] = {
			{0x3302, 0x0080, BYTE_LEN, 0, 0x0080},
			{0x3355, 0x0018, BYTE_LEN, 0, 0x0058},
			{0x335A, 0x0080, BYTE_LEN, 0, 0xFFFF},
			{0x335B, 0x0080, BYTE_LEN, 0, 0xFFFF},
		};

		rc = ov3640_i2c_write_table(&ov3640_effect_mono_tbl[0],
				ARRAY_SIZE(ov3640_effect_mono_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_EFFECT_NEGATIVE: {//Negative
		struct ov3640_i2c_reg_conf const ov3640_effect_negative_tbl[] = {
			{0x3302, 0x0080, BYTE_LEN, 0, 0x0080},
			{0x3355, 0x0040, BYTE_LEN, 0, 0x0058},
		};

		rc = ov3640_i2c_write_table(&ov3640_effect_negative_tbl[0],
				ARRAY_SIZE(ov3640_effect_negative_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_EFFECT_SEPIA: {
		struct ov3640_i2c_reg_conf const ov3640_effect_sepia_tbl[] = {
			{0x3302, 0x0080, BYTE_LEN, 0, 0x0080},
			{0x3355, 0x0018, BYTE_LEN, 0, 0x0058},
			{0x335A, 0x0040, BYTE_LEN, 0, 0xFFFF},
			{0x335B, 0x00A0, BYTE_LEN, 0, 0xFFFF},
		};

		rc = ov3640_i2c_write_table(&ov3640_effect_sepia_tbl[0],
				ARRAY_SIZE(ov3640_effect_sepia_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_EFFECT_BLUISH: {//Bluish
		struct ov3640_i2c_reg_conf const ov3640_effect_bluish_tbl[] = {
			{0x3302, 0x0080, BYTE_LEN, 0, 0x0080},
			{0x3355, 0x0018, BYTE_LEN, 0, 0x0058},
			{0x335A, 0x00A0, BYTE_LEN, 0, 0xFFFF},
			{0x335B, 0x0040, BYTE_LEN, 0, 0xFFFF},
		};

		rc = ov3640_i2c_write_table(&ov3640_effect_bluish_tbl[0],
				ARRAY_SIZE(ov3640_effect_bluish_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_EFFECT_REDDISH: {//Reddish
		struct ov3640_i2c_reg_conf const ov3640_effect_reddish_tbl[] = {
			{0x3302, 0x0080, BYTE_LEN, 0, 0x0080},
			{0x3355, 0x0018, BYTE_LEN, 0, 0x0058},
			{0x335A, 0x0080, BYTE_LEN, 0, 0xFFFF},
			{0x335B, 0x00C0, BYTE_LEN, 0, 0xFFFF},
		};

		rc = ov3640_i2c_write_table(&ov3640_effect_reddish_tbl[0],
				ARRAY_SIZE(ov3640_effect_reddish_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_EFFECT_GREENISH: {//Greenish
		struct ov3640_i2c_reg_conf const ov3640_effect_greenish_tbl[] = {
			{0x3302, 0x0080, BYTE_LEN, 0, 0x0080},
			{0x3355, 0x0018, BYTE_LEN, 0, 0x0058},
			{0x335A, 0x0060, BYTE_LEN, 0, 0xFFFF},
			{0x335B, 0x0060, BYTE_LEN, 0, 0xFFFF},
		};

		rc = ov3640_i2c_write_table(&ov3640_effect_greenish_tbl[0],
				ARRAY_SIZE(ov3640_effect_greenish_tbl));

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

static long ov3640_set_wb(int mode, int wb)
{
	long rc = 0;

	CDBG("ov3640_set_wb, mode = %d, wb = %d\n",
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
		struct ov3640_i2c_reg_conf const ov3640_wb_auto_tbl[] = {
			{0x332B, 0x0000, BYTE_LEN, 0, 0x0008},
		};

		rc = ov3640_i2c_write_table(&ov3640_wb_auto_tbl[0],
				ARRAY_SIZE(ov3640_wb_auto_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_WB_INCANDESCENT: {//TUNGSTEN
		struct ov3640_i2c_reg_conf const ov3640_wb_incandescent_tbl[] = {
			{0x332B, 0x0008, BYTE_LEN, 0, 0x0008},
			{0x33A7, 0x003F, BYTE_LEN, 0, 0xFFFF},
			{0x33A8, 0x0039, BYTE_LEN, 0, 0xFFFF},
			{0x33A9, 0x0061, BYTE_LEN, 0, 0xFFFF},
		};

		rc = ov3640_i2c_write_table(&ov3640_wb_incandescent_tbl[0],
				ARRAY_SIZE(ov3640_wb_incandescent_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_WB_FLUORESCENT: {//Office
		struct ov3640_i2c_reg_conf const ov3640_wb_fluorescent_tbl[] = {
			{0x332B, 0x0008, BYTE_LEN, 0, 0x0008},
			{0x33A7, 0x0046, BYTE_LEN, 0, 0xFFFF},
			{0x33A8, 0x0039, BYTE_LEN, 0, 0xFFFF},
			{0x33A9, 0x0053, BYTE_LEN, 0, 0xFFFF},
		};

		rc = ov3640_i2c_write_table(&ov3640_wb_fluorescent_tbl[0],
				ARRAY_SIZE(ov3640_wb_fluorescent_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_WB_DAYLIGHT: {//Sunny
		struct ov3640_i2c_reg_conf const ov3640_wb_daylight_tbl[] = {
			{0x332B, 0x0008, BYTE_LEN, 0, 0x0008},
			{0x33A7, 0x0057, BYTE_LEN, 0, 0xFFFF},
			{0x33A8, 0x0039, BYTE_LEN, 0, 0xFFFF},
			{0x33A9, 0x0040, BYTE_LEN, 0, 0xFFFF},
		};

		rc = ov3640_i2c_write_table(&ov3640_wb_daylight_tbl[0],
				ARRAY_SIZE(ov3640_wb_daylight_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_WB_CLOUDY_DAYLIGHT: {//Cloudy
		struct ov3640_i2c_reg_conf const ov3640_wb_cloudydaylight_tbl[] = {
			{0x332B, 0x0008, BYTE_LEN, 0, 0x0008},
			{0x33A7, 0x005C, BYTE_LEN, 0, 0xFFFF},
			{0x33A8, 0x0039, BYTE_LEN, 0, 0xFFFF},
			{0x33A9, 0x003C, BYTE_LEN, 0, 0xFFFF},
		};

		rc = ov3640_i2c_write_table(&ov3640_wb_cloudydaylight_tbl[0],
				ARRAY_SIZE(ov3640_wb_cloudydaylight_tbl));

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


static long ov3640_set_brightness(int mode, int brightness)
{
	long rc = 0;

	CDBG("ov3640_set_brightness, mode = %d, brightness = %d\n",
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
		struct ov3640_i2c_reg_conf const ov3640_brightness_0_tbl[] = {
			{0x3302, 0x0080, BYTE_LEN, 0, 0x0080},
			{0x335E, 0x0030, BYTE_LEN, 0, 0xFFFF},
			{0x3355, 0x0004, BYTE_LEN, 0, 0x0004},
			{0x3354, 0x0008, BYTE_LEN, 0, 0x0008},
		};

		rc = ov3640_i2c_write_table(&ov3640_brightness_0_tbl[0],
				ARRAY_SIZE(ov3640_brightness_0_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_BRIGHTNESS_1: {
		struct ov3640_i2c_reg_conf const ov3640_brightness_1_tbl[] = {
			{0x3302, 0x0080, BYTE_LEN, 0, 0x0080},
			{0x335E, 0x0020, BYTE_LEN, 0, 0xFFFF},
			{0x3355, 0x0004, BYTE_LEN, 0, 0x0004},
			{0x3354, 0x0008, BYTE_LEN, 0, 0x0008},
		};

		rc = ov3640_i2c_write_table(&ov3640_brightness_1_tbl[0],
				ARRAY_SIZE(ov3640_brightness_1_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_BRIGHTNESS_2: {
		struct ov3640_i2c_reg_conf const ov3640_brightness_2_tbl[] = {
			{0x3302, 0x0080, BYTE_LEN, 0, 0x0080},
			{0x335E, 0x0010, BYTE_LEN, 0, 0xFFFF},
			{0x3355, 0x0004, BYTE_LEN, 0, 0x0004},
			{0x3354, 0x0008, BYTE_LEN, 0, 0x0008},
		};

		rc = ov3640_i2c_write_table(&ov3640_brightness_2_tbl[0],
				ARRAY_SIZE(ov3640_brightness_2_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_BRIGHTNESS_3: {
		struct ov3640_i2c_reg_conf const ov3640_brightness_3_tbl[] = {
			{0x3302, 0x0080, BYTE_LEN, 0, 0x0080},
			{0x335E, 0x0000, BYTE_LEN, 0, 0xFFFF},
			{0x3355, 0x0004, BYTE_LEN, 0, 0x0004},
			{0x3354, 0x0000, BYTE_LEN, 0, 0x0008},
		};

		rc = ov3640_i2c_write_table(&ov3640_brightness_3_tbl[0],
				ARRAY_SIZE(ov3640_brightness_3_tbl));

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_BRIGHTNESS_4: {
		struct ov3640_i2c_reg_conf const ov3640_brightness_4_tbl[] = {
			{0x3302, 0x0080, BYTE_LEN, 0, 0x0080},
			{0x335E, 0x0010, BYTE_LEN, 0, 0xFFFF},
			{0x3355, 0x0004, BYTE_LEN, 0, 0x0004},
			{0x3354, 0x0000, BYTE_LEN, 0, 0x0008},
		};

		rc = ov3640_i2c_write_table(&ov3640_brightness_4_tbl[0],
				ARRAY_SIZE(ov3640_brightness_4_tbl));

		if (rc < 0)
			return rc;
	} 
		break;

	case CAMERA_BRIGHTNESS_5: {
		struct ov3640_i2c_reg_conf const ov3640_brightness_5_tbl[] = {
			{0x3302, 0x0080, BYTE_LEN, 0, 0x0080},
			{0x335E, 0x0020, BYTE_LEN, 0, 0xFFFF},
			{0x3355, 0x0004, BYTE_LEN, 0, 0x0004},
			{0x3354, 0x0000, BYTE_LEN, 0, 0x0008},
		};

		rc = ov3640_i2c_write_table(&ov3640_brightness_5_tbl[0],
				ARRAY_SIZE(ov3640_brightness_5_tbl));

		if (rc < 0)
			return rc;
	} 
		break;

	case CAMERA_BRIGHTNESS_6: {
		struct ov3640_i2c_reg_conf const ov3640_brightness_6_tbl[] = {
			{0x3302, 0x0080, BYTE_LEN, 0, 0x0080},
			{0x335E, 0x0030, BYTE_LEN, 0, 0xFFFF},
			{0x3355, 0x0004, BYTE_LEN, 0, 0x0004},
			{0x3354, 0x0000, BYTE_LEN, 0, 0x0008},
		};

		rc = ov3640_i2c_write_table(&ov3640_brightness_6_tbl[0],
				ARRAY_SIZE(ov3640_brightness_6_tbl));

		if (rc < 0)
			return rc;
	} 
		break;

	case CAMERA_BRIGHTNESS_7:
	case CAMERA_BRIGHTNESS_8:
	case CAMERA_BRIGHTNESS_9:
	case CAMERA_BRIGHTNESS_10: {
		struct ov3640_i2c_reg_conf const ov3640_brightness_10_tbl[] = {
			{0x3302, 0x0080, BYTE_LEN, 0, 0x0080},
			{0x335E, 0x0030, BYTE_LEN, 0, 0xFFFF},
			{0x3355, 0x0004, BYTE_LEN, 0, 0x0004},
			{0x3354, 0x0000, BYTE_LEN, 0, 0x0008},
		};

		rc = ov3640_i2c_write_table(&ov3640_brightness_10_tbl[0],
				ARRAY_SIZE(ov3640_brightness_10_tbl));

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

static long ov3640_set_antibanding(int mode, int antibanding)
{
	long rc = 0;

	CDBG("ov3640_set_antibanding, mode = %d, antibanding = %d\n",
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
		struct ov3640_i2c_reg_conf const ov3640_antibanding_off_tbl[] = {
		};

		rc = ov3640_i2c_write_table(&ov3640_antibanding_off_tbl[0],
				ARRAY_SIZE(ov3640_antibanding_off_tbl));

		if (rc < 0)
			return rc;

		ov3640_m_60Hz=0;
	}
			break;

	case CAMERA_ANTIBANDING_60HZ: {
		struct ov3640_i2c_reg_conf const ov3640_antibanding_60hz_tbl[] = {
		};

		rc = ov3640_i2c_write_table(&ov3640_antibanding_60hz_tbl[0],
				ARRAY_SIZE(ov3640_antibanding_60hz_tbl));

		if (rc < 0)
			return rc;

		ov3640_m_60Hz=1;
	}
		break;

	case CAMERA_ANTIBANDING_50HZ: {
		struct ov3640_i2c_reg_conf const ov3640_antibanding_60hz_tbl[] = {
		};

		rc = ov3640_i2c_write_table(&ov3640_antibanding_60hz_tbl[0],
				ARRAY_SIZE(ov3640_antibanding_60hz_tbl));

		if (rc < 0)
			return rc;

		ov3640_m_60Hz=0;
	}
		break;

	case CAMERA_ANTIBANDING_AUTO: {
		struct ov3640_i2c_reg_conf const ov3640_antibanding_60hz_tbl[] = {
		};

		rc = ov3640_i2c_write_table(&ov3640_antibanding_60hz_tbl[0],
				ARRAY_SIZE(ov3640_antibanding_60hz_tbl));

		if (rc < 0)
			return rc;

		ov3640_m_60Hz=0;
	}
		break;

	default: {
		if (rc < 0)
			return rc;

		ov3640_m_60Hz=0;
		return -EINVAL;
	}
	}

	return rc;
}

/* FIH, Charles Huang, 2009/11/04 { */
/* [FXX_CR], af function  */
#ifdef CONFIG_FIH_FXX
static long ov3640_set_autofocus(int mode, int autofocus)
{
	long rc = 0;

	CDBG("ov3640_set_autofocus, mode = %d, autofocus = %d\n",
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
	case CAMERA_AUTOFOCUS:
	default: {

		struct ov3640_i2c_reg_conf const ov3640_autofocus_tbl[] = {
			{0x3F00, 0x0008, BYTE_LEN, 1, 0xFFFF},
			{0x3F00, 0x0003, BYTE_LEN, 0, 0xFFFF},
		};

		rc = ov3640_i2c_write_table(&ov3640_autofocus_tbl[0],
				ARRAY_SIZE(ov3640_autofocus_tbl));

		if (rc < 0)
			return rc;
	}
			break;
	}

	return rc;
}
#endif
/* } FIH, Charles Huang, 2009/11/04 */

static int ov3640_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
	uint16_t model_id = 0;
	int rc = 0;

	/* OV suggested Power up block End */
	/* Read the Model ID of the sensor */
	/* Read REG_OV3640_MODEL_ID_HI & REG_OV3640_MODEL_ID_LO */
	rc = ov3640_i2c_read(OV3640_I2C_READ_SLAVE_ID,
		REG_OV3640_MODEL_ID_HI, &model_id, WORD_LEN);
	if (rc < 0)
		goto init_probe_fail;

	CDBG("ov3640 model_id = 0x%x\n", model_id);

	/* Check if it matches it with the value in Datasheet */
	if (model_id != OV3640_MODEL_ID) {
		rc = -EFAULT;
		goto init_probe_fail;
	}

	rc = ov3640_reg_init();
	if (rc < 0)
		goto init_probe_fail;

	return rc;

init_probe_fail:
	return rc;
}

int ov3640_sensor_init(const struct msm_camera_sensor_info *data)
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

	ov3640_ctrl = kzalloc(sizeof(struct ov3640_ctrl), GFP_KERNEL);
	if (!ov3640_ctrl) {
		CDBG("ov3640_init failed!\n");
		rc = -ENOMEM;
		goto init_done;
	}

	if (data)
		ov3640_ctrl->sensordata = data;

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

	rc = ov3640_reset(data);
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
#ifdef OV3640_USE_VFS
	if (ov3640_use_vfs_mclk_setting!=0)
		msm_camio_clk_rate_set(ov3640_use_vfs_mclk_setting);
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

	rc = ov3640_sensor_init_probe(data);
	if (rc < 0) {
		CDBG("ov3640_sensor_init failed!\n");
		goto init_fail;
	}

init_done:
	return rc;

init_fail:
	kfree(ov3640_ctrl);
	return rc;
}

static int ov3640_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&ov3640_wait_queue);
	return 0;
}

int ov3640_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	long   rc = 0;

	if (copy_from_user(
				&cfg_data,
				(void *)argp,
				sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	/* down(&ov3640_sem); */

	CDBG("ov3640_sensor_config, cfgtype = %d, mode = %d\n",
		cfg_data.cfgtype, cfg_data.mode);

	switch (cfg_data.cfgtype) {
	case CFG_SET_MODE:
		rc = ov3640_set_sensor_mode(
					cfg_data.mode);
		break;

	case CFG_SET_EFFECT:
		rc = ov3640_set_effect(
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
		rc = ov3640_set_brightness(
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
		rc = ov3640_set_wb(
					cfg_data.mode,
					cfg_data.cfg.wb);
		break;

	case CFG_SET_ANTIBANDING:
		rc = ov3640_set_antibanding(
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
		rc = ov3640_set_autofocus(
					cfg_data.mode,
					cfg_data.cfg.autofocus);
		break;
#endif
/* } FIH, Charles Huang, 2009/11/04 */

	default:
		rc = -EINVAL;
		break;
	}

	/* up(&ov3640_sem); */

	return rc;
}

int ov3640_sensor_release(void)
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
	const struct msm_camera_sensor_info *dev;
	/* down(&ov3640_sem); */
#if 0/*mr7*/
	int HWID=FIH_READ_HWID_FROM_SMEM();
#endif
	mutex_lock(&ov3640_mut);

	dev = ov3640_ctrl->sensordata;
	rc = gpio_request(dev->sensor_reset, "ov3640");
	if (!rc) {
		rc = gpio_direction_output(dev->sensor_reset, 0);
	}
	gpio_free(dev->sensor_reset);

	rc = gpio_request(dev->sensor_pwd, "ov3640");
	if (!rc) {
		rc = gpio_direction_output(dev->sensor_pwd, 1);
	}
	gpio_free(dev->sensor_pwd);
#if 0/*mr7*/
	if (HWID>=CMCS_7627_EVB1)
	{
		/* Switch disable */
		gpio_tlmm_config(GPIO_CFG(121, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
		gpio_direction_output(121, 1);	
		msleep(10);
	}
#endif
	CDBG("[OV3640 3M]  gpio_get_value(0) = %d\n", gpio_get_value(0));
	CDBG("[OV3640 3M]  gpio_get_value(31) = %d\n", gpio_get_value(31));
	CDBG("[OV3640 3M]  gpio_get_value(85) = %d\n", gpio_get_value(85));
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		;
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		CDBG("[OV3640 3M]  gpio_get_value(122) = %d\n", gpio_get_value(122));
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

	kfree(ov3640_ctrl);
	ov3640_ctrl = NULL;
	/* up(&ov3640_sem); */
	mutex_unlock(&ov3640_mut);
	return rc;
}

/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef OV3640_USE_VFS
void ov3640_get_param(const char *buf, size_t count, struct ov3640_i2c_reg_conf *tbl, 
	unsigned short tbl_size, int *use_setting)
{
	unsigned short waddr;
	unsigned short wdata;
	enum ov3640_width width;
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

static ssize_t ov3640_write_initreg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	ov3640_get_param(buf, count, &ov3640_vfs_init_settings_tbl[0], ov3640_vfs_init_settings_tbl_size, &ov3640_use_vfs_init_setting);
	return count;
}

static ssize_t ov3640_write_oemtreg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	ov3640_get_param(buf, count, &ov3640_vfs_oem_settings_tbl[0], ov3640_vfs_oem_settings_tbl_size, &ov3640_use_vfs_oem_setting);
	return count;
}

static ssize_t ov3640_write_previewreg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	ov3640_get_param(buf, count, &ov3640_vfs_preview_settings_tbl[0], ov3640_vfs_preview_settings_tbl_size, &ov3640_use_vfs_preview_setting);
	return count;
}

static ssize_t ov3640_write_snapreg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	ov3640_get_param(buf, count, &ov3640_vfs_snap_settings_tbl[0], ov3640_vfs_snap_settings_tbl_size, &ov3640_use_vfs_snap_setting);
	return count;
}

static ssize_t ov3640_write_snapaereg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	ov3640_get_param(buf, count, &ov3640_vfs_snapae_settings_tbl[0], ov3640_vfs_snapae_settings_tbl_size, &ov3640_use_vfs_snapae_setting);
	return count;
}

static ssize_t ov3640_write_iqreg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	ov3640_get_param(buf, count, &ov3640_vfs_iq_settings_tbl[0], ov3640_vfs_iq_settings_tbl_size, &ov3640_use_vfs_iq_setting);
	return count;
}

static ssize_t ov3640_write_lensreg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	ov3640_get_param(buf, count, &ov3640_vfs_lens_settings_tbl[0], ov3640_vfs_lens_settings_tbl_size, &ov3640_use_vfs_lens_setting);
	return count;
}

static ssize_t ov3640_write_reg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	long rc = 0;

	ov3640_get_param(buf, count, &ov3640_vfs_writereg_settings_tbl[0], ov3640_vfs_writereg_settings_tbl_size, &ov3640_use_vfs_writereg_setting);
	if (ov3640_use_vfs_writereg_setting)
	{
		rc = ov3640_i2c_write_table(&ov3640_vfs_writereg_settings_tbl[0],
			ov3640_vfs_writereg_settings_tbl_size);
		ov3640_use_vfs_writereg_setting =0;
	}
	return count;
}

static ssize_t ov3640_setrange(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	ov3640_get_param(buf, count, &ov3640_vfs_getreg_settings_tbl[0], ov3640_vfs_getreg_settings_tbl_size, &ov3640_use_vfs_getreg_setting);
	return count;
}

static ssize_t ov3640_getrange(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i,rc;
	char *str = buf;

	if (ov3640_use_vfs_getreg_setting)
	{
		for (i=0;i<=ov3640_vfs_getreg_settings_tbl_size;i++)
		{
			if (ov3640_vfs_getreg_settings_tbl[i].waddr==0xFFFF)
				break;

			rc = ov3640_i2c_read(OV3640_I2C_READ_SLAVE_ID,
				ov3640_vfs_getreg_settings_tbl[i].waddr, &(ov3640_vfs_getreg_settings_tbl[i].wdata), BYTE_LEN);
			CDBG("ov3640 reg 0x%4X = 0x%2X\n", ov3640_vfs_getreg_settings_tbl[i].waddr, ov3640_vfs_getreg_settings_tbl[i].wdata);

			str += sprintf(str, "%04X,%2X,%2X\n", ov3640_vfs_getreg_settings_tbl[i].waddr, 
				ov3640_vfs_getreg_settings_tbl[i].wdata, 
				ov3640_vfs_getreg_settings_tbl[i].mask);

			if (rc <0)
				break;
		}
	}
	return (str - buf);
}

static ssize_t ov3640_setmclk(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf,"%d",&ov3640_use_vfs_mclk_setting);
	return count;
}

static ssize_t ov3640_getmclk(struct device *dev, struct device_attribute *attr, char *buf)
{
	return (sprintf(buf,"%d\n",ov3640_use_vfs_mclk_setting));
}

static ssize_t ov3640_setmultiple(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf,"%d",&ov3640_use_vfs_multiple_setting);
	return count;
}

static ssize_t ov3640_getmultiple(struct device *dev, struct device_attribute *attr, char *buf)
{
	return (sprintf(buf,"%d\n",ov3640_use_vfs_multiple_setting));
}

static ssize_t ov3640_setflashtime(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf,"%d",&ov3640_use_vfs_flashtime_setting);
	return count;
}

static ssize_t ov3640_getflashtime(struct device *dev, struct device_attribute *attr, char *buf)
{
	return (sprintf(buf,"%d\n",ov3640_use_vfs_flashtime_setting));
}

DEVICE_ATTR(initreg_3640, 0666, NULL, ov3640_write_initreg);
DEVICE_ATTR(oemreg_3640, 0666, NULL, ov3640_write_oemtreg);
DEVICE_ATTR(previewreg_3640, 0666, NULL, ov3640_write_previewreg);
DEVICE_ATTR(snapreg_3640, 0666, NULL, ov3640_write_snapreg);
DEVICE_ATTR(snapregae_3640, 0666, NULL, ov3640_write_snapaereg);
DEVICE_ATTR(iqreg_3640, 0666, NULL, ov3640_write_iqreg);
DEVICE_ATTR(lensreg_3640, 0666, NULL, ov3640_write_lensreg);
DEVICE_ATTR(writereg_3640, 0666, NULL, ov3640_write_reg);
DEVICE_ATTR(getreg_3640, 0666, ov3640_getrange, ov3640_setrange);
DEVICE_ATTR(mclk_3640, 0666, ov3640_getmclk, ov3640_setmclk);
DEVICE_ATTR(multiple_3640, 0666, ov3640_getmultiple, ov3640_setmultiple);
DEVICE_ATTR(flashtime_3640, 0666, ov3640_getflashtime, ov3640_setflashtime);

static int create_attributes(struct i2c_client *client)
{
	int rc;

	dev_attr_initreg_3640.attr.name = OV3640_INITREG;
	dev_attr_oemreg_3640.attr.name = OV3640_OEMREG;
	dev_attr_previewreg_3640.attr.name = OV3640_PREVIEWREG;
	dev_attr_snapreg_3640.attr.name = OV3640_SNAPREG;
	dev_attr_snapregae_3640.attr.name = OV3640_SNAPAEREG;
	dev_attr_iqreg_3640.attr.name = OV3640_IQREG;
	dev_attr_lensreg_3640.attr.name = OV3640_LENSREG;
	dev_attr_writereg_3640.attr.name = OV3640_WRITEREG;
	dev_attr_getreg_3640.attr.name = OV3640_GETREG;
	dev_attr_mclk_3640.attr.name = OV3640_MCLK;
	dev_attr_multiple_3640.attr.name = OV3640_MULTIPLE;
	dev_attr_flashtime_3640.attr.name = OV3640_FLASHTIME;

	rc = device_create_file(&client->dev, &dev_attr_initreg_3640);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create ov3640 attribute \"initreg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}

	rc = device_create_file(&client->dev, &dev_attr_oemreg_3640);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create ov3640 attribute \"oemreg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}

	rc = device_create_file(&client->dev, &dev_attr_previewreg_3640);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create ov3640 attribute \"previewreg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}

	rc = device_create_file(&client->dev, &dev_attr_snapreg_3640);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create ov3640 attribute \"snapreg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_snapregae_3640);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create ov3640 attribute \"snapregae\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_iqreg_3640);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create ov3640 attribute \"iqreg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_lensreg_3640);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create ov3640 attribute \"lensreg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_writereg_3640);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create ov3640 attribute \"writereg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_getreg_3640);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create ov3640 attribute \"getreg\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_mclk_3640);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create ov3640 attribute \"mclk\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_multiple_3640);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create ov3640 attribute \"multiple\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	rc = device_create_file(&client->dev, &dev_attr_flashtime_3640);
	if (rc < 0) {
		dev_err(&client->dev, "%s: Create ov3640 attribute \"flashtime\" failed!! <%d>", __func__, rc);
		
		return rc; 
	}	

	return rc;
}

static int remove_attributes(struct i2c_client *client)
{
	device_remove_file(&client->dev, &dev_attr_initreg_3640);
	device_remove_file(&client->dev, &dev_attr_oemreg_3640);
	device_remove_file(&client->dev, &dev_attr_previewreg_3640);
	device_remove_file(&client->dev, &dev_attr_snapreg_3640);
	device_remove_file(&client->dev, &dev_attr_snapregae_3640);
	device_remove_file(&client->dev, &dev_attr_iqreg_3640);
	device_remove_file(&client->dev, &dev_attr_lensreg_3640);
	device_remove_file(&client->dev, &dev_attr_writereg_3640);
	device_remove_file(&client->dev, &dev_attr_getreg_3640);
	device_remove_file(&client->dev, &dev_attr_mclk_3640);
	device_remove_file(&client->dev, &dev_attr_multiple_3640);
	device_remove_file(&client->dev, &dev_attr_flashtime_3640);

	return 0;
}
#endif /* OV3640_USE_VFS */
#endif
/* } FIH, Charles Huang, 2009/06/24 */

/* FIH, Charles Huang, 2010/02/26 { */
/* [FXX_CR], move to kthread and speed up launching camera app */
#ifdef CONFIG_FIH_FXX
static int ov3640_firmware_thread(void *arg)
{
	int ret = 0;

	daemonize("ov3640_firmware_thread");

	while (1) {
		wait_for_completion(&ov3640_firmware_init);
		CAM_USER_G0(KERN_INFO "%s: Got init signal\n", __func__);
		ret = ov3640_i2c_write_table(&ov3640_regs.af_init_settings_tbl[0],
			ov3640_regs.af_init_settings_tbl_size);

		if (ret < 0)
			printk(KERN_INFO "firmware fail");
	}
	
    return 0;
}
#endif
/* } FIH, Charles Huang, 2010/02/26 */

static int ov3640_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int rc = 0;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	ov3640_sensorw =
		kzalloc(sizeof(struct ov3640_work), GFP_KERNEL);

	if (!ov3640_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, ov3640_sensorw);
	ov3640_init_client(client);
	ov3640_client = client;
	
	CDBG("ov3640_probe successed!\n");

	return 0;

probe_failure:
	kfree(ov3640_sensorw);
	ov3640_sensorw = NULL;
	CDBG("ov3640_probe failed!\n");
	return rc;
}

static int __exit ov3640_i2c_remove(struct i2c_client *client)
{
	struct ov3640_work *sensorw = i2c_get_clientdata(client);

/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef OV3640_USE_VFS
	remove_attributes(client);
#endif /* OV3640_USE_VFS */
#endif
/* } FIH, Charles Huang, 2009/06/24 */

	free_irq(client->irq, sensorw);
	ov3640_client = NULL;
	ov3640_sensorw = NULL;
	kfree(sensorw);
	return 0;
}

#ifdef CONFIG_PM
static int ov3640_suspend(struct i2c_client *client, pm_message_t mesg)
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

static int ov3640_resume(struct i2c_client *client)
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
# define ov3640_suspend NULL
# define ov3640_resume  NULL
#endif

static const struct i2c_device_id ov3640_i2c_id[] = {
	{ "ov3640", 0},
	{ },
};

static struct i2c_driver ov3640_i2c_driver = {
	.id_table = ov3640_i2c_id,
	.probe  = ov3640_i2c_probe,
	.remove = __exit_p(ov3640_i2c_remove),
	.suspend  	= ov3640_suspend,
	.resume   	= ov3640_resume,
	.driver = {
		.name = "ov3640",
	},
};

static int ov3640_sensor_probe(const struct msm_camera_sensor_info *info,
				struct msm_sensor_ctrl *s)
{
	int rc = i2c_add_driver(&ov3640_i2c_driver);
	uint16_t model_id = 0;
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	int HWID=FIH_READ_HWID_FROM_SMEM();
	struct vreg *vreg_gp1;
	struct vreg *vreg_msme2;
	/* } FIH, IvanCHTang, 2010/6/1 */
#if 0/*mr7*/
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

	if (rc < 0 || ov3640_client == NULL) {
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

	rc = ov3640_reset(info);
	if (rc < 0) {
		CDBG("reset failed!\n");
		goto probe_fail;
	}

	/* EVB CAMIF cannont handle in 24MHz */
	/* EVB use 12.8MHz */
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef OV3640_USE_VFS
	if (ov3640_use_vfs_mclk_setting!=0)
		msm_camio_clk_rate_set(ov3640_use_vfs_mclk_setting);
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
	/* Read REG_OV3640_MODEL_ID_HI & REG_OV3640_MODEL_ID_LO */
	rc = ov3640_i2c_read(OV3640_I2C_READ_SLAVE_ID,
		REG_OV3640_MODEL_ID_HI, &model_id, WORD_LEN);
	if (rc < 0)
		goto probe_fail;

	CDBG("ov3640 model_id = 0x%x\n", model_id);

	/* Check if it matches it with the value in Datasheet */
	if (model_id != OV3640_MODEL_ID) {
		rc = -EFAULT;
		goto probe_fail;
	}

	rc = gpio_request(info->sensor_reset, "ov3640");
	if (!rc) {
		rc = gpio_direction_output(info->sensor_reset, 0);
	}
	gpio_free(info->sensor_reset);

	rc = gpio_request(info->sensor_pwd, "ov3640");
	if (!rc) {
		rc = gpio_direction_output(info->sensor_pwd, 1);
	}
	gpio_free(info->sensor_pwd);
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
	CDBG("[OV3640 3M]  gpio_get_value(0) = %d\n", gpio_get_value(0));
	CDBG("[OV3640 3M]  gpio_get_value(31) = %d\n", gpio_get_value(31));
	CDBG("[OV3640 3M]  gpio_get_value(85) = %d\n", gpio_get_value(85));
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		;
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		CDBG("[OV3640 3M]  gpio_get_value(122) = %d\n", gpio_get_value(122));
	}
	/* } FIH, IvanCHTang, 2010/6/1 */
	   
/* FIH, Charles Huang, 2009/06/24 { */
/* [FXX_CR], add VFS */
#ifdef CONFIG_FIH_FXX
#ifdef OV3640_USE_VFS
	rc = create_attributes(ov3640_client);
	if (rc < 0) {
		dev_err(&ov3640_client->dev, "%s: create attributes failed!! <%d>", __func__, rc);
		goto probe_done;
	}
#endif /* OV3640_USE_VFS */
#endif
/* } FIH, Charles Huang, 2009/06/24 */
/* FIH, Charles Huang, 2010/02/26 { */
/* [FXX_CR], move to kthread and speed up launching camera app */
#ifdef CONFIG_FIH_FXX
	kernel_thread(ov3640_firmware_thread, NULL, CLONE_FS | CLONE_FILES);
#endif
/* } FIH, Charles Huang, 2010/02/26 */

	s->s_init = ov3640_sensor_init;
	s->s_release = ov3640_sensor_release;
	s->s_config  = ov3640_sensor_config;

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

	dev_info(&ov3640_client->dev, "probe_done %s %s:%d\n", __FILE__, __func__, __LINE__);
	return rc;

probe_fail:
	dev_info(&ov3640_client->dev, "probe_fail %s %s:%d\n", __FILE__, __func__, __LINE__);
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
	CDBG("[OV3640 3M]  gpio_get_value(0) = %d\n", gpio_get_value(0));
	CDBG("[OV3640 3M]  gpio_get_value(31) = %d\n", gpio_get_value(31));
	CDBG("[OV3640 3M]  gpio_get_value(85) = %d\n", gpio_get_value(85));
	/* FIH, IvanCHTang, 2010/6/1 { */
	/* [FXX_CR], Set CAMIF_PD_VGA by HWID for FM6/FN6 - [FM6#0003] */
	if((HWID>=CMCS_FN6_PR1)&&(HWID<=CMCS_FN6_MP1))
	{
		;
	}
	else if((HWID>=CMCS_128_FM6_PR1)&&(HWID<=CMCS_125_FM6_MP))
	{
		CDBG("[OV3640 3M]  gpio_get_value(122) = %d\n", gpio_get_value(122));
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

static int __ov3640_probe(struct platform_device *pdev)
{
	return msm_camera_drv_start(pdev, ov3640_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __ov3640_probe,
	.driver = {
		.name = "msm_camera_ov3640",
		.owner = THIS_MODULE,
	},
};

static int __init ov3640_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(ov3640_init);
