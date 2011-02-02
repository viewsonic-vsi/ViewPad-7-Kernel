#define __FIH_EVENT_LOG__

#ifdef __FIH_EVENT_LOG__
#define PDC_LOG_MASK                                        						(0x40000000)
#define FATAL_LOG_MASK                                      					(0x80000000)
#define PRINTK_LOG                                          						(0x00000000)

//For Power on code
#define KERNEL_POWER_ON_CAUSE                               				(0x00AAA000 | PDC_LOG_MASK)
//For Power down code
#define POWER_DOWN_CODE_POWER_DOWN_POWERKEY                 	(0x00BBB001 | PDC_LOG_MASK)
#define POWER_DOWN_CODE_POWER_DOWN_BATTLOW                  	(0x00BBB002 | PDC_LOG_MASK)
#define POWER_DOWN_CODE_POWER_DOWN_CPM_REBOOT               	(0x00BBB003 | PDC_LOG_MASK)
#define POWER_DOWN_CODE_IOCTL_REBOOT                        			(0x00BBB004 | PDC_LOG_MASK)
#define POWER_DOWN_CODE_PHONE_HANG                          			(0x00BBB005 | PDC_LOG_MASK)
#define POWER_DOWN_CODE_POWER_ON_IMGUPD                     		(0x00BBB006 | PDC_LOG_MASK)
#define POWER_DOWN_CODE_MASTER_RESET_AUTO                   		(0x00BBB007 | PDC_LOG_MASK)
#define POWER_DOWN_CODE_MASTER_RESET_MANUAL                	 	(0x00BBB008 | PDC_LOG_MASK)
#define POWER_DOWN_CODE_MASTER_RESET_COMPLETE				(0x00BBB009 | PDC_LOG_MASK)
#define POWER_DOWN_CODE_MASTER_RESET_FAIL					(0x00BBB010 | PDC_LOG_MASK)
#define POWER_DOWN_CODE_MASTER_RESET_USER					(0x00BBB011 | PDC_LOG_MASK)
#define POWER_DOWN_CODE_REBOOT_RUU_START					(0x00BBB101 | PDC_LOG_MASK)
#define POWER_DOWN_CODE_REBOOT_RUU_COMPLETE				(0x00BBB102 | PDC_LOG_MASK)
#define POWER_DOWN_CODE_REBOOT_ARM9_FATAL_ERR			(0x00BBB103 | PDC_LOG_MASK)
#define POWER_DOWN_CODE_POWER_OFF_SUCCESS					(0x00BBBFFF | PDC_LOG_MASK)

#define KERNEL_DUMP_DEBUG_MESSAGE                           			(0x00000001)

//For RIL
#define RILGSM_LOG_                      									(0x00010001 | FATAL_LOG_MASK)
#define RILGSM_LOG_MSGQ_FAILURE								(0x00010002 | FATAL_LOG_MASK)
#define RILGSM_LOG_POWER_DOWN									(0x00010003 | FATAL_LOG_MASK)
#define RILGSM_LOG_REGSTATUS									(0x00010004 | FATAL_LOG_MASK)
#define RILGSM_LOG_SYSID											(0x00010005)
#define RILGSM_LOG_SYSTYPE										(0x00010006)
#define RILGSM_LOG_CC_MT											(0x00010007 | FATAL_LOG_MASK)
#define RILGSM_LOG_CC_MO											(0x00010008 | FATAL_LOG_MASK)
#define RILGSM_LOG_DS_START										(0x00010009 | FATAL_LOG_MASK)
#define RILGSM_LOG_DS_STOP										(0x0001000a | FATAL_LOG_MASK)
#define RILGSM_LOG_DS_STATUS									(0x0001000b)
#define RILGSM_LOG_SMS_WMS_HANDLERSP						(0x0001000c)
#define RILGSM_LOG_SMS_BCMMEVT									(0x0001000d | FATAL_LOG_MASK)
#define RILGSM_LOG_SMS_READY									(0x0001000e | FATAL_LOG_MASK)
#define RILGSM_LOG_SS_INVOKE									(0x0001000f)

//For USB
#define USBC_LOG_PLUG_IN											(0x00020001)
#define USBC_LOG_PLUG_OUT										(0x00020002)
#define USBC_LOG_USB_PLUG_IN                                				(0x00020003)
#define USBC_LOG_AC_PLUG_IN                                 					(0x00020004)
#define USBC_LOG_CHANGE_CLIENT                               				(0x00020005)
#define USBC_LOG_ENUMERATION_SUCCESS                               		(0x00020006)
#define USBC_LOG_STALL_DEFAULT_PIPE                       				(0x00020007 | FATAL_LOG_MASK)
#define USBC_LOG_STALL_IN_PIPE                              				(0x00020008 | FATAL_LOG_MASK)
#define USBC_LOG_STALL_OUT_PIPE                             				(0x00020009 | FATAL_LOG_MASK)
#define USBC_LOG_SET_CHARGING_CURRENT                 				(0x00020010)
#define USBC_LOG_DETECT_FLAG_ERROR                       				(0x00020011 | FATAL_LOG_MASK)
#define USBC_LOG_DETECT_UNKNOWN_STATUS               				(0x00020012 | FATAL_LOG_MASK)
#define USBC_LOG_UNLOAD_DRIVER                						(0x00020013)
#define USBC_LOG_ULPIVPReg_FAIL              							(0x00020014 | FATAL_LOG_MASK)
#define USBC_LOG_TRY_LPMMODE									(0x00020015)

//For Battery
#define BATTERY_LOG_BAT_CHECK_ERROR							(0x00040001| FATAL_LOG_MASK)
#define BATTERY_LOG_CHARGER_PLUG_IN							(0x00040002)
#define BATTERY_LOG_CHARGER_PLUG_OUT							(0x00040003)
#define BATTERY_LOG_CHARGER_CURRENT							(0x00040004)
#define BATTERY_LOG_TEMPERATURE_DANGEROUS					(0x00040005| FATAL_LOG_MASK)
#define BATTERY_LOG_TEMPERATURE_SAFE							(0x00040006| FATAL_LOG_MASK)
#define BATTERY_LOG_BAT_LEVEL_CHANGE							(0x00040007)
#define BATTERY_LOG_DS2482_RESET_FAIL							(0x00040008)
#define BATTERY_LOG_ONE_WIRE_RESET_FAIL						(0x00040009)
#define BATTERY_LOG_BAT_ID_WRONG								(0x0004000a| FATAL_LOG_MASK)
#define BATTERY_LOG_USB_CABLE_NOTIFY							(0x0004000b| FATAL_LOG_MASK)
#define BATTERY_LOG_SUSPEND_POWER_OFF						(0x0004000c| FATAL_LOG_MASK)

//For BT
#define BLUETOOTH_OPENPORT_ERROR								(0x00050001 | FATAL_LOG_MASK)
#define BLUETOOTH_INITIAL_FAIL									(0x00050002 | FATAL_LOG_MASK)
#define BLUETOOTH_INITIAL_FIXNV_FAIL							(0x00050003 )
#define BLUETOOTH_INITIAL_RUNTIMENV_FAIL						(0x00050004 )
#define BLUETOOTH_ADDR											(0x00050005 )
#define BLUETOOTH_QSOC_TYPE										(0x00050006 )
#define BLUETOOTH_READ_UNKNOWN_PKT							(0x00050007 )
#define BLUETOOTH_INITIAL_RETRY_BAUD							(0x00050008 )
#define BLUETOOTH_BAUDRATE										(0x00050009 )
#define BLUETOOTH_HCI_CLOSE_CONNECTION						(0x0005000A )
#define BLUETOOTH_CLEANUP_OPENCONNECTION					(0x0005000B )
#define BLUETOOTH_MSM_UART										(0x0005000C )

//For WIFI
#define WIFI_LOG_													(0x00060001 | FATAL_LOG_MASK)
#define WIFI_LOG_SET_POWER										(0x00060002)
#define WIFI_LOG_CHARGE_EVENT									(0x00060003)
#define WIFI_LOG_UNCHARGE_EVENT								(0x00060004)
#define WIFI_LOG_FW_DOWNLOAD_FAIL								(0x00060005 | FATAL_LOG_MASK)
#define WIFI_LOG_PS												(0x00060006)

//For GPS
#define GPS_LOG_													(0x00070001 | FATAL_LOG_MASK)
#define GPS_HW_ON													(0x00070002)
#define GPS_HW_OFF												(0x00070003)
#define GPS_ADD_CLIENT											(0x00070004)
#define GPS_REMOVE_CLIENT										(0x00070005)
#define GPS_SUSPEND												(0x00070006)
#define GPS_RESUME													(0x00070007)

//For WAV
#define WAVEDEV_LOG_ERROR										(0x00080001| FATAL_LOG_MASK)
#define WAVEDEV_LOG_AUDIO_PATH									(0x00080002)
#define WAVEDEV_LOG_HEADSET										(0x00080003)
#define WAVEDEV_LOG_SPEAKERPHONE								(0x00080004)
#define WAVEDEV_LOG_BT											(0x00080005)
#define WAVEDEV_LOG_CALL_STATUS								(0x00080006)

//For DISPLAY
#define DISPLAY_LOG_PRIMARY										(0x00090001)
#define DISPLAY_LOG_SCRATCHPAD									(0x00090002)
#define DISPLAY_LOG_HALCAPS										(0x00090003)
#define DISPLAY_LOG_OVERLAY										(0x00090004)
#define DISPLAY_LOG_GPE_AHI										(0x00090005)
#define DISPLAY_LOG_AHI_PM										(0x00090006)
#define DISPLAY_LOG_AHI_DRV										(0x00090007)
#define DISPLAY_LOG_DISPLAY										(0x00090008)
#define DISPLAY_LOG_QCOMGPE										(0x00090009)
#define DISPLAY_LOG_MDP_INTERFACE								(0x0009000A)
#define DISPLAY_LOG_DISP2D										(0x0009000B)
#define BACKLIGHT_LOG												(0x00091001)
#define BACKLIGHT_ONOFF_LOG										(0x00091002)

//For SD
#define SDMMC_LOG_												(0x000A0001| FATAL_LOG_MASK)

//For KPD
#define KEYPAD_LOG_I2C_FAIL										(0x000B0001)
#define KEYPAD_LOG_IS_DEVICE_LOCK								(0x000B0002)
#define KEYPAD_LOG_FLUSH_DBG									(0x000B0003)
#define KEYPAD_LOG_KEYPADIC_NOSUSPEND						(0x000B0010)

//For CAM
//#define CAMERA_LOG_												(0x000C0001| FATAL_LOG_MASK)
#define CAMERA_LOG_CAMERA_PREPAREHW_DONE					(0x000C0001)  // 1:HW prepare done
#define CAMERA_LOG_CAMERA_PREPAREHW_FAIL					(0x000C0002 | FATAL_LOG_MASK)// 1: HW prepare failed
#define CAMERA_LOG_ESD_STATE									(0x000C0003)  //0: fail  1:stand by 2:finish srop prew. 3: finish recover.4:triggle ESD 5.finish triggle
#define CAMERA_LOG_STILL_IMAGE									(0x000C0004)   //0:fail 1:done  2:exceed size
#define CAMERA_LOG_CAMERA_UNATTENDED_STATE					(0x000C0005) //0:free untd. 1:req untd.
#define CAMERA_LOG_CAMERA_IOCONTROL							(0x000C0006)// 0: D0 4:D4 1:IO ctl opened.
#define CAMERA_LOG_CAMERA_FIRST_FRAME_CB						(0x000C0007) // 1:CB OK
#define CAMERA_LOG_CAMERA_ALLOCATE7MB						(0x000C0008) // 0:failed 1:successed 2:Free

//For TAUCH
#define TOUCH_LOG_												(0x000D0001 | FATAL_LOG_MASK)
#define TOUCH_LOG_SAMPLERATE									(0x000D0002)
#define TOUCH_LOG_BUSY											(0x000D0003 | FATAL_LOG_MASK)
#define TOUCH_LOG_DISABLE										(0x000D0004)

//For NLED
#define NLED_LOG_ 													(0x000E0001| FATAL_LOG_MASK)

//For SENSER
#define SENSER_LOG_												(0x000E0001| FATAL_LOG_MASK)
#define SENSER_LOG_ENABLE_STATE									(0x000E0010)

#define FMD_LOG_READ_ERROR										(0x000F0001)
#define FMD_LOG_WRITE_ERROR										(0x000F0002)
#define FMD_LOG_ERASE_ERROR										(0x000F0003)
#define FMD_LOG_WRITE_BOOTCFG									(0x000F0004)
#define FMD_LOG_WRITE_MODEM_FOTA								(0x000F0005)
#define FMD_LOG_WRITE_MASTER_RESET_FLAG						(0x000F0006)
#define FMD_LOG_WRITE_RESERVED									(0x000F0007)
#define FMD_LOG_READ_RESERVED									(0x000F0008)  
#define FMD_LOG_WRITE_BOOT_MODE								(0x000F0009) 

int eventlog(const char *fmt, ...);
#else
#define eventlog(x, ...) {}
#endif
