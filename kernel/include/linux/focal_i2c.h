#ifndef FOCAL_I2C_H
#define FOCAL_I2C_H

#include <linux/ioctl.h>

struct focal_i2c_platform_data {
	uint16_t version;
	int abs_x_min;
	int abs_x_max;
	int abs_y_min;
	int abs_y_max;
	int intr_gpio;
	int (*power)(int on);
};

struct focal_i2c_sensitivity {
	int x;
	int y;
};

struct focal_i2c_resolution {
	int x;
	int y;
};

#define FT5202_IOC_MAGIC    'E'
#define FT5202_IOC_GFWVERSION   _IOR(FT5202_IOC_MAGIC, 0, int)  /* get firmware version */
#define FT5202_IOC_GPWSTATE     _IOR(FT5202_IOC_MAGIC, 1, int)  /* get power state */
#define FT5202_IOC_GORIENTATION _IOR(FT5202_IOC_MAGIC, 2, int)  /* get orientation */
#define FT5202_IOC_GRESOLUTION  _IOR(FT5202_IOC_MAGIC, 3, int)  /* get resolution */
#define FT5202_IOC_GDEEPSLEEP   _IOR(FT5202_IOC_MAGIC, 4, int)  /* get deep sleep function status */
#define FT5202_IOC_GFWID        _IOR(FT5202_IOC_MAGIC, 5, int)  /* get firmware id */
#define FT5202_IOC_GREPORTRATE  _IOR(FT5202_IOC_MAGIC, 6, int)  /* get report rate */
#define FT5202_IOC_GSENSITIVITY _IOR(FT5202_IOC_MAGIC, 7, int)  /* get sensitivity setting */
#define FT5202_IOC_SPWSTATE     _IOW(FT5202_IOC_MAGIC, 8, int)  /* change power state */
#define FT5202_IOC_SORIENTATION _IOW(FT5202_IOC_MAGIC, 9, int)  /* change orientation */
#define FT5202_IOC_SRESOLUTION  _IOW(FT5202_IOC_MAGIC,10, int)  /* change resolution */
#define FT5202_IOC_SDEEPSLEEP   _IOW(FT5202_IOC_MAGIC,11, int)  /* enable or disable deep sleep function */
#define FT5202_IOC_SREPORTRATE  _IOW(FT5202_IOC_MAGIC,12, int)  /* change device report frequency */
#define FT5202_IOC_SSENSITIVITY _IOR(FT5202_IOC_MAGIC,13, int)  /* change sensitivity setting */
#define FT5202_IOC_MAXNR    15

/* FIH, Henry Juang, 2009/11/20 ++*/
/* [FXX_CR], Add for proximity driver to turn on/off BL and TP. */
int notify_from_proximity(bool bFlag);  //Added for test
/* FIH, Henry Juang, 2009/11/20 --*/
#endif
