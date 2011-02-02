/*
 *     ov7690_reg.c - Camera Sensor Driver
 *
 *     Copyright (C) 2009 Charles YS Huang <charlesyshuang@fihtdc.com>
 *     Copyright (C) 2008 FIH CO., Inc.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; version 2 of the License.
 */


#include "ov7690.h"
/* ov7690 initial setting: 640*480 30fps YUV */
struct ov7690_i2c_reg_conf const ov7690_init_settings_tbl[] = {
    {0x0012, 0x0080, BYTE_LEN, 0, 0xFFFF},
    {0x000C, 0x0016, BYTE_LEN, 0, 0xFFFF},
    {0x0048, 0x0042, BYTE_LEN, 0, 0xFFFF},
    {0x0041, 0x0043, BYTE_LEN, 0, 0xFFFF},
    {0x004C, 0x007B, BYTE_LEN, 0, 0xFFFF},
    {0x0081, 0x00FF, BYTE_LEN, 0, 0xFFFF},
    {0x0021, 0x0044, BYTE_LEN, 0, 0xFFFF},
    {0x0016, 0x0003, BYTE_LEN, 0, 0xFFFF},
    {0x0039, 0x0080, BYTE_LEN, 0, 0xFFFF},
    {0x001E, 0x00B1, BYTE_LEN, 0, 0xFFFF},
    {0x0012, 0x0000, BYTE_LEN, 0, 0xFFFF},
    {0x0082, 0x0003, BYTE_LEN, 0, 0xFFFF},
    {0x00D0, 0x0048, BYTE_LEN, 0, 0xFFFF},
    {0x0080, 0x007F, BYTE_LEN, 0, 0xFFFF},
    {0x003E, 0x0030, BYTE_LEN, 0, 0xFFFF},
    {0x0022, 0x0000, BYTE_LEN, 0, 0xFFFF},
    {0x0017, 0x0069, BYTE_LEN, 0, 0xFFFF},
    {0x0018, 0x00A4, BYTE_LEN, 0, 0xFFFF},
    {0x0019, 0x000C, BYTE_LEN, 0, 0xFFFF},
    {0x001A, 0x00F6, BYTE_LEN, 0, 0xFFFF},
    {0x00C8, 0x0002, BYTE_LEN, 0, 0xFFFF},
    {0x00C9, 0x0080, BYTE_LEN, 0, 0xFFFF},
    {0x00CA, 0x0001, BYTE_LEN, 0, 0xFFFF},
    {0x00CB, 0x00E0, BYTE_LEN, 0, 0xFFFF},
    {0x00CC, 0x0002, BYTE_LEN, 0, 0xFFFF},
    {0x00CD, 0x0080, BYTE_LEN, 0, 0xFFFF},
    {0x00CE, 0x0001, BYTE_LEN, 0, 0xFFFF},
    {0x00CF, 0x00E0, BYTE_LEN, 0, 0xFFFF},
    {0x0085, 0x0090, BYTE_LEN, 0, 0xFFFF},
    {0x0086, 0x0000, BYTE_LEN, 0, 0xFFFF},
    {0x0087, 0x0000, BYTE_LEN, 0, 0xFFFF},
    {0x0088, 0x0010, BYTE_LEN, 0, 0xFFFF},
    {0x0089, 0x0030, BYTE_LEN, 0, 0xFFFF},
    {0x008A, 0x0029, BYTE_LEN, 0, 0xFFFF},
    {0x008B, 0x0026, BYTE_LEN, 0, 0xFFFF},
    {0x00BB, 0x0080, BYTE_LEN, 0, 0xFFFF},
    {0x00BC, 0x0062, BYTE_LEN, 0, 0xFFFF},
    {0x00BD, 0x001E, BYTE_LEN, 0, 0xFFFF},
    {0x00BE, 0x0026, BYTE_LEN, 0, 0xFFFF},
    {0x00BF, 0x007B, BYTE_LEN, 0, 0xFFFF},
    {0x00C0, 0x00AC, BYTE_LEN, 0, 0xFFFF},
    {0x00C1, 0x001E, BYTE_LEN, 0, 0xFFFF},
    {0x00B7, 0x0005, BYTE_LEN, 0, 0xFFFF},
    {0x00B8, 0x0009, BYTE_LEN, 0, 0xFFFF},
    {0x00B9, 0x0000, BYTE_LEN, 0, 0xFFFF},
    {0x00BA, 0x0018, BYTE_LEN, 0, 0xFFFF},
    {0x005A, 0x004A, BYTE_LEN, 0, 0xFFFF},
    {0x005B, 0x009F, BYTE_LEN, 0, 0xFFFF},
    {0x005C, 0x0048, BYTE_LEN, 0, 0xFFFF},
    {0x005D, 0x0032, BYTE_LEN, 0, 0xFFFF},
    {0x0024, 0x007D, BYTE_LEN, 0, 0xFFFF},
    {0x0025, 0x006B, BYTE_LEN, 0, 0xFFFF},
    {0x0026, 0x00C3, BYTE_LEN, 0, 0xFFFF},
    {0x00A3, 0x000B, BYTE_LEN, 0, 0xFFFF},
    {0x00A4, 0x0015, BYTE_LEN, 0, 0xFFFF},
    {0x00A5, 0x002A, BYTE_LEN, 0, 0xFFFF},
    {0x00A6, 0x0051, BYTE_LEN, 0, 0xFFFF},
    {0x00A7, 0x0063, BYTE_LEN, 0, 0xFFFF},
    {0x00A8, 0x0074, BYTE_LEN, 0, 0xFFFF},
    {0x00A9, 0x0083, BYTE_LEN, 0, 0xFFFF},
    {0x00AA, 0x0091, BYTE_LEN, 0, 0xFFFF},
    {0x00AB, 0x009E, BYTE_LEN, 0, 0xFFFF},
    {0x00AC, 0x00AA, BYTE_LEN, 0, 0xFFFF},
    {0x00AD, 0x00BE, BYTE_LEN, 0, 0xFFFF},
    {0x00AE, 0x00CE, BYTE_LEN, 0, 0xFFFF},
    {0x00AF, 0x00E5, BYTE_LEN, 0, 0xFFFF},
    {0x00B0, 0x00F3, BYTE_LEN, 0, 0xFFFF},
    {0x00B1, 0x00FB, BYTE_LEN, 0, 0xFFFF},
    {0x00B2, 0x0006, BYTE_LEN, 0, 0xFFFF},
    {0x008E, 0x0092, BYTE_LEN, 0, 0xFFFF},
    {0x0096, 0x00FF, BYTE_LEN, 0, 0xFFFF},
    {0x0097, 0x0000, BYTE_LEN, 0, 0xFFFF},
    {0x008C, 0x005D, BYTE_LEN, 0, 0xFFFF},
    {0x008D, 0x0011, BYTE_LEN, 0, 0xFFFF},
    {0x008E, 0x0012, BYTE_LEN, 0, 0xFFFF},
    {0x008F, 0x0011, BYTE_LEN, 0, 0xFFFF},
    {0x0090, 0x0050, BYTE_LEN, 0, 0xFFFF},
    {0x0091, 0x0022, BYTE_LEN, 0, 0xFFFF},
    {0x0092, 0x00D1, BYTE_LEN, 0, 0xFFFF},
    {0x0093, 0x00A7, BYTE_LEN, 0, 0xFFFF},
    {0x0094, 0x0023, BYTE_LEN, 0, 0xFFFF},
    {0x0095, 0x003B, BYTE_LEN, 0, 0xFFFF},
    {0x0096, 0x00FF, BYTE_LEN, 0, 0xFFFF},
    {0x0097, 0x0000, BYTE_LEN, 0, 0xFFFF},
    {0x0098, 0x004A, BYTE_LEN, 0, 0xFFFF},
    {0x0099, 0x0046, BYTE_LEN, 0, 0xFFFF},
    {0x009A, 0x003D, BYTE_LEN, 0, 0xFFFF},
    {0x009B, 0x003A, BYTE_LEN, 0, 0xFFFF},
    {0x009C, 0x00F0, BYTE_LEN, 0, 0xFFFF},
    {0x009D, 0x00F0, BYTE_LEN, 0, 0xFFFF},
    {0x009E, 0x00F0, BYTE_LEN, 0, 0xFFFF},
    {0x009F, 0x00FF, BYTE_LEN, 0, 0xFFFF},
    {0x00A0, 0x0056, BYTE_LEN, 0, 0xFFFF},
    {0x00A1, 0x0055, BYTE_LEN, 0, 0xFFFF},
    {0x00A2, 0x0013, BYTE_LEN, 0, 0xFFFF},
    {0x0050, 0x009A, BYTE_LEN, 0, 0xFFFF},
    {0x0051, 0x0080, BYTE_LEN, 0, 0xFFFF},
    {0x0021, 0x0023, BYTE_LEN, 0, 0xFFFF},
    {0x0014, 0x0028, BYTE_LEN, 0, 0xFFFF},
    {0x0013, 0x00F7, BYTE_LEN, 0, 0xFFFF},
    {0x0011, 0x0000, BYTE_LEN, 0, 0xFFFF},
    {0x0015, 0x00B4, BYTE_LEN, 0, 0xFFFF},
};

/* preview_setting 3M to VGA Key*/
struct ov7690_i2c_reg_conf const ov7690_preview_settings_tbl[] = {
};      
        
/* snapshot VGA to 3M key */
struct ov7690_i2c_reg_conf const ov7690_snapshot_settings_tbl[] = {
};

static struct ov7690_i2c_reg_conf const ov7690_noise_reduction_reg_settings_array[] = {
};

struct ov7690_i2c_reg_conf const ov7690_pll_setup_tbl[] = {
};

/* Refresh Sequencer */
struct ov7690_i2c_reg_conf const ov7690_sequencer_tbl[] = {
};

struct ov7690_i2c_reg_conf const ov7690_lens_roll_off_tbl[] = {
};

struct ov7690_i2c_reg_conf const ov7690_oem_settings_tbl[] = {
};

struct ov7690_i2c_reg_conf const ov7690_iq_settings_tbl[] = {
};

struct ov7690_i2c_reg_conf const ov7690_gseolens_settings_tbl[] = {
};

struct ov7690_i2c_reg_conf const ov7690_grdlens_settings_tbl[] = {
};

struct ov7690_reg ov7690_regs = {
	.init_settings_tbl = &ov7690_init_settings_tbl[0],
	.init_settings_tbl_size = ARRAY_SIZE(ov7690_init_settings_tbl),
	.preview_settings_tbl=&ov7690_preview_settings_tbl[0],
	.preview_settings_tbl_size=ARRAY_SIZE(ov7690_preview_settings_tbl),
	.snapshot_settings_tbl=&ov7690_snapshot_settings_tbl[0],
	.snapshot_settings_tbl_size=ARRAY_SIZE(ov7690_snapshot_settings_tbl),
	.noise_reduction_reg_settings_tbl = &ov7690_noise_reduction_reg_settings_array[0],
	.noise_reduction_reg_settings_tbl_size =ARRAY_SIZE(ov7690_noise_reduction_reg_settings_array),
	.plltbl = &ov7690_pll_setup_tbl[0],
	.plltbl_size = ARRAY_SIZE(ov7690_pll_setup_tbl),
	.stbl = &ov7690_sequencer_tbl[0],
	.stbl_size = ARRAY_SIZE(ov7690_sequencer_tbl),
	.rftbl = &ov7690_lens_roll_off_tbl[0],
	.rftbl_size = ARRAY_SIZE(ov7690_lens_roll_off_tbl),
	.oem_settings_tbl = &ov7690_oem_settings_tbl[0],
	.oem_settings_tbl_size = ARRAY_SIZE(ov7690_oem_settings_tbl),
	.iq_settings_tbl = &ov7690_iq_settings_tbl[0],
	.iq_settings_tbl_size = ARRAY_SIZE(ov7690_iq_settings_tbl),
	.gseolens_settings_tbl = &ov7690_gseolens_settings_tbl[0],
	.gseolens_settings_tbl_size = ARRAY_SIZE(ov7690_gseolens_settings_tbl),
	.grdlens_settings_tbl = &ov7690_grdlens_settings_tbl[0],
	.grdlens_settings_tbl_size = ARRAY_SIZE(ov7690_grdlens_settings_tbl),
};



