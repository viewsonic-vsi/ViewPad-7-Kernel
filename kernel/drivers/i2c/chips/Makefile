#
# Makefile for miscellaneous I2C chip drivers.
#
# Do not add new drivers to this directory! It is DEPRECATED.
#
# Device drivers are better grouped according to the functionality they
# implement rather than to the bus they are connected to. In particular:
# * Hardware monitoring chip drivers go to drivers/hwmon
# * RTC chip drivers go to drivers/rtc
# * I/O expander drivers go to drivers/gpio
#

obj-$(CONFIG_SENSORS_AKM8973)	+= akm8973.o
obj-$(CONFIG_SENSORS_AKM8976)	+= akm8976.o
obj-$(CONFIG_DS1682)		+= ds1682.o
obj-$(CONFIG_SENSORS_PCA963X)	+= pca963x.o
obj-$(CONFIG_SENSORS_TSL2550)	+= tsl2550.o
obj-$(CONFIG_SENSORS_MT9T013)	+= mt9t013.o
###+++ FIH_FXX, Neo Chen, 2009/06/08, change backlight driver from max7302 to max8831
#obj-$(CONFIG_BACKLIGHT_LED_MAX8831) += max8831.o
obj-$(CONFIG_FIH_PROJECT_F0X) += max8831_f0x.o
obj-$(CONFIG_FIH_PROJECT_FM6) += max8831_fm6.o
###--- FIH_FXX, Neo Chen, 2009/06/08
obj-$(CONFIG_VP_A1026)		+= a1026.o
#+++ FIH_FXX +++  //Michael
obj-$(CONFIG_FIH_BACKLIGHT_LED_TCA6507) += tca6507.o
#+++ FIH_FXX +++  //Michael
###+FIH_FXX, Charles Huang, 2009/05/18
obj-$(CONFIG_FLASH_DRIVER_IC_AAT1272) += aat1272.o
###-FIH_FXX, Charles Huang, 2009/05/18

ifeq ($(CONFIG_I2C_DEBUG_CHIP),y)
EXTRA_CFLAGS += -DDEBUG
endif

