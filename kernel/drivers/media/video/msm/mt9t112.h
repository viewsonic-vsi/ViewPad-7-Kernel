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

#ifndef MT9T112_H
#define MT9T112_H
#include <linux/types.h>
#include <mach/camera.h>

extern struct mt9t112_reg mt9t112_regs;

enum mt9t112_width {
	WORD_LEN,
	BYTE_LEN
};

struct mt9t112_i2c_reg_conf {
	unsigned short waddr;
	unsigned short wdata;
	enum mt9t112_width width;
	unsigned short mdelay_time;
	unsigned short mask;
};


struct mt9t112_reg {
	// 1. Initialization
	struct mt9t112_i2c_reg_conf const *init_settings_tbl;
	uint16_t init_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *preview_settings_tbl;
	uint16_t preview_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *snapshot_settings_tbl;
	uint16_t snapshot_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *noise_reduction_reg_settings_tbl;
	uint16_t noise_reduction_reg_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *plltbl;
	uint16_t plltbl_size;
	struct mt9t112_i2c_reg_conf const *stbl;
	uint16_t stbl_size;
	struct mt9t112_i2c_reg_conf const *rftbl;
	uint16_t rftbl_size;
	struct mt9t112_i2c_reg_conf const *oem_settings_tbl;
	uint16_t oem_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *iq_settings_tbl;
	uint16_t iq_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *gseolens_settings_tbl;
	uint16_t gseolens_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *grdlens_settings_tbl;
	uint16_t grdlens_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *af_patch_settings_tbl;
	uint16_t af_patch_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *af_init_settings_tbl;
	uint16_t af_init_settings_tbl_size;
	/* FIH, IvanCHTang, 2010/6/15 { */
	/* [FXX_CR], [Camera] MT9T112 model detection - [FM6#0007] */
	struct mt9t112_i2c_reg_conf const *af_WV511_patch_settings_tbl;
	uint16_t af_WV511_patch_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *af_WV511_init_settings_tbl;
	uint16_t af_WV511_init_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *af_WV511_standby_settings_tbl;
	uint16_t af_WV511_standby_settings_tbl_size;
	/* } FIH, IvanCHTang, 2010/6/15 */
	struct mt9t112_i2c_reg_conf const *oem_lsc_settings_tbl;
	uint16_t oem_lsc_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *fih_lsc_settings_tbl;
	uint16_t fih_lsc_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *other_settings_tbl;
	uint16_t other_settings_tbl_size;
	// Feature(s)
	// 2. Anti Banding
	struct mt9t112_i2c_reg_conf const *anti_band_off_settings_tbl;
	uint16_t anti_band_off_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *anti_band_50hz_settings_tbl;
	uint16_t anti_band_50hz_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *anti_band_60hz_settings_tbl;
	uint16_t anti_band_60hz_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *anti_band_auto_settings_tbl;
	uint16_t anti_band_auto_settings_tbl_size;
	// 3. Effect
	struct mt9t112_i2c_reg_conf const *effect_none_settings_tbl;
	uint16_t effect_none_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *effect_mono_settings_tbl;
	uint16_t effect_mono_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *effect_sepia_settings_tbl;
	uint16_t effect_sepia_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *effect_negative_settings_tbl;
	uint16_t effect_negative_settings_tbl_size;
	// 4. Focus
	struct mt9t112_i2c_reg_conf const *focus_auto_settings_tbl;
	uint16_t focus_auto_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *focus_normal_settings_tbl;
	uint16_t focus_normal_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *focus_marco_settings_tbl;
	uint16_t focus_marco_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *focus_off_settings_tbl;
	uint16_t focus_off_settings_tbl_size;
	// 5. Metering
	struct mt9t112_i2c_reg_conf const *metering_frame_average_settings_tbl;
	uint16_t metering_frame_average_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *metering_center_weighted_settings_tbl;
	uint16_t metering_center_weighted_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *metering_spot_settings_tbl;
	uint16_t metering_spot_settings_tbl_size;
	// 6. White balance
	struct mt9t112_i2c_reg_conf const *wb_auto_settings_tbl;
	uint16_t wb_auto_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *wb_incandescent_settings_tbl;
	uint16_t wb_incandescent_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *wb_fluorescent_settings_tbl;
	uint16_t wb_fluorescent_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *wb_daylight_settings_tbl;
	uint16_t wb_daylight_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *wb_cloudy_settings_tbl;
	uint16_t wb_cloudy_settings_tbl_size;
	// 7. ISO
	// 8. Lens Shading
	// 9. Brightness
	struct mt9t112_i2c_reg_conf const *brightness_0_settings_tbl;
	uint16_t brightness_0_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *brightness_1_settings_tbl;
	uint16_t brightness_1_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *brightness_2_settings_tbl;
	uint16_t brightness_2_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *brightness_3_settings_tbl;
	uint16_t brightness_3_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *brightness_4_settings_tbl;
	uint16_t brightness_4_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *brightness_5_settings_tbl;
	uint16_t brightness_5_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *brightness_6_settings_tbl;
	uint16_t brightness_6_settings_tbl_size;
	// 10. Contrast
	struct mt9t112_i2c_reg_conf const *contrast_0_settings_tbl;
	uint16_t contrast_0_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *contrast_1_settings_tbl;
	uint16_t contrast_1_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *contrast_2_settings_tbl;
	uint16_t contrast_2_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *contrast_3_settings_tbl;
	uint16_t contrast_3_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *contrast_4_settings_tbl;
	uint16_t contrast_4_settings_tbl_size;
	// 11. Saturation
	struct mt9t112_i2c_reg_conf const *saturation_0_settings_tbl;
	uint16_t saturation_0_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *saturation_1_settings_tbl;
	uint16_t saturation_1_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *saturation_2_settings_tbl;
	uint16_t saturation_2_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *saturation_3_settings_tbl;
	uint16_t saturation_3_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *saturation_4_settings_tbl;
	uint16_t saturation_4_settings_tbl_size;
	// 12. Sharpness
	struct mt9t112_i2c_reg_conf const *sharpness_0_settings_tbl;
	uint16_t sharpness_0_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *sharpness_1_settings_tbl;
	uint16_t sharpness_1_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *sharpness_2_settings_tbl;
	uint16_t sharpness_2_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *sharpness_3_settings_tbl;
	uint16_t sharpness_3_settings_tbl_size;
	struct mt9t112_i2c_reg_conf const *sharpness_4_settings_tbl;
	uint16_t sharpness_4_settings_tbl_size;
	// 13. Zoom
};

#endif /* MT9T112_H */
