#ifndef PANJIT_I2C_H
#define PANJIT_I2C_H

#include <linux/ioctl.h>

struct panjit_i2c_platform_data {
	uint16_t version;
	int abs_x_min;
	int abs_x_max;
	int abs_y_min;
	int abs_y_max;
	int intr_gpio;
       int sk_intr_gpio;
	int (*power)(int on);
};

struct panjit_i2c_sensitivity {
	int x;
	int y;
};

struct panjit_i2c_resolution {
	int x;
	int y;
};

#define PANJIT_IOC_MAGIC    'E'
#define PANJIT_IOC_GFWVERSION   _IOR(PANJIT_IOC_MAGIC, 0, int)  /* get firmware version */
#define PANJIT_IOC_GPWSTATE     _IOR(PANJIT_IOC_MAGIC, 1, int)  /* get power state */
#define PANJIT_IOC_GORIENTATION _IOR(PANJIT_IOC_MAGIC, 2, int)  /* get orientation */
#define PANJIT_IOC_GRESOLUTION  _IOR(PANJIT_IOC_MAGIC, 3, int)  /* get resolution */
#define PANJIT_IOC_GDEEPSLEEP   _IOR(PANJIT_IOC_MAGIC, 4, int)  /* get deep sleep function status */
#define PANJIT_IOC_GFWID        _IOR(PANJIT_IOC_MAGIC, 5, int)  /* get firmware id */
#define PANJIT_IOC_GREPORTRATE  _IOR(PANJIT_IOC_MAGIC, 6, int)  /* get report rate */
#define PANJIT_IOC_GSENSITIVITY _IOR(PANJIT_IOC_MAGIC, 7, int)  /* get sensitivity setting */
#define PANJIT_IOC_SPWSTATE     _IOW(PANJIT_IOC_MAGIC, 8, int)  /* change power state */
#define PANJIT_IOC_SORIENTATION _IOW(PANJIT_IOC_MAGIC, 9, int)  /* change orientation */
#define PANJIT_IOC_SRESOLUTION  _IOW(PANJIT_IOC_MAGIC,10, int)  /* change resolution */
#define PANJIT_IOC_SDEEPSLEEP   _IOW(PANJIT_IOC_MAGIC,11, int)  /* enable or disable deep sleep function */
#define PANJIT_IOC_SREPORTRATE  _IOW(PANJIT_IOC_MAGIC,12, int)  /* change device report frequency */
#define PANJIT_IOC_SSENSITIVITY _IOR(PANJIT_IOC_MAGIC,13, int)  /* change sensitivity setting */
#define PANJIT_IOC_MAXNR    15

#endif
