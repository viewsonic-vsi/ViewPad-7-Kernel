#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/compile.h>
#include <asm/gpio.h>
#include <mach/msm_iomap.h>
#include <mach/msm_smd.h>

//FIH, JamesKCTung, 2009/11/03 +++
/* Add dbgmask partition*/
#ifdef CONFIG_PRINTK
#include <asm/uaccess.h>
#include <mach/msm_iomap.h>
#include <linux/io.h>

/*FIHTDC, WillChen changes because PLOG address has changed by Tiger { */
//#define DBGMSKBUF (MSM_PLOG_BASE + 0x80000)
#define DBGMSKBUF	(MSM_PLOG_BASE2 + 0XFF000)
/*FIHTDC, WillChen changes because PLOG address has changed by Tiger } */

char * debug_mask = (char *) DBGMSKBUF;
static struct proc_dir_entry *mask_file;
#define	MASKSIZE   	4096
#endif
//FIH, JamesKCTung, 2009/11/03 ---

/* FIH, AudiPCHuang, 2009/06/05 { */
/* [ADQ.B-1440], For getting image version from splash.img*/
static char adq_image_version[32];
int JogballExist_pr1,JogballExist_pr2;

#define PLUS_X_GPIO     91
#define NEG_X_GPIO      88
#define PLUS_Y_GPIO     90
#define NEG_Y_GPIO      93

void set_image_version(char* img_ver)
{
	snprintf(adq_image_version, 25, "%s", img_ver);
}
EXPORT_SYMBOL(set_image_version);
/* } FIH, AudiPCHuang, 2009/06/05 */

static int proc_calc_metrics(char *page, char **start, off_t off,
				 int count, int *eof, int len)
{
	if (len <= off+count) *eof = 1;
	*start = page + off;
	len -= off;
	if (len>count) len = count;
	if (len<0) len = 0;
	return len;
}

static int build_version_read_proc(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{
	int len;

	len = snprintf(page, PAGE_SIZE, "%s\n", /*ADQ_IMAGE_VERSION*/adq_image_version);
	return proc_calc_metrics(page, start, off, count, eof, len);
}

static int device_model_read_proc(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{
	int len;
	int HWID = FIH_READ_ORIG_HWID_FROM_SMEM();
	char ver[24];

/* FIH, JiaHao, 2010/08/20 { */
	switch (HWID){
	case CMCS_HW_EVB1: //0x0
	case CMCS_HW_EVB2:
	case CMCS_HW_EVB3:
	case CMCS_HW_EVB4:
	case CMCS_HW_EVB5:
		strcpy(ver, "F02");
		break; 
	case CMCS_ORIG_RTP_PR1: //0x5
		if (JogballExist_pr1)
			strcpy(ver, "F10");
		else
			strcpy(ver, "F02");
		break; 
	case CMCS_ORIG_CTP_PR1: //0xd
		if (JogballExist_pr1)
			strcpy(ver, "F11");
		else
			strcpy(ver, "F03");
		break; 
	case CMCS_850_RTP_PR2: //0x10
	case CMCS_850_RTP_PR3:
	case CMCS_850_RTP_PR4:
	case CMCS_850_RTP_PR5:
	case CMCS_850_RTP_MP1:
	case CMCS_850_RTP_MP2:
	case CMCS_850_RTP_MP3:
		if (JogballExist_pr2)
			strcpy(ver, "F10");
		else
			strcpy(ver, "F02");
		break; 
	case CMCS_850_CTP_PR2: //0x17
	case CMCS_850_CTP_PR3:
	case CMCS_850_CTP_PR4:
	case CMCS_850_CTP_PR5:
	case CMCS_850_CTP_MP1:
	case CMCS_850_CTP_MP2:
	case CMCS_850_CTP_MP3:
		if (JogballExist_pr2)
			strcpy(ver, "F11");
		else
			strcpy(ver, "F03");
		break;
	case CMCS_900_RTP_PR2: //0x20
	case CMCS_900_RTP_PR3:
	case CMCS_900_RTP_PR4:
	case CMCS_900_RTP_PR5:
	case CMCS_900_RTP_MP1:
	case CMCS_900_RTP_MP2:
	case CMCS_900_RTP_MP3:
		if (JogballExist_pr2)
			strcpy(ver, "F10");
		else
			strcpy(ver, "F02");
		break;
	case CMCS_900_CTP_PR2: //0x27
	case CMCS_900_CTP_PR3:
	case CMCS_900_CTP_PR4:
	case CMCS_900_CTP_PR5:
	case CMCS_900_CTP_MP1:
	case CMCS_900_CTP_MP2:
	case CMCS_900_CTP_MP3:
		if (JogballExist_pr2)
			strcpy(ver, "F11");
		else
			strcpy(ver, "F03");
		break;
	case CMCS_145_CTP_PR1: //0x2E
		strcpy(ver, "AWS");
		break; 
	case CMCS_125_FST_PR1: //0x30
	case CMCS_125_FST_PR2:
	case CMCS_125_FST_MP1:
	case CMCS_128_FST_PR1: //0x40
	case CMCS_128_FST_PR2:
	case CMCS_128_FST_MP1:
		strcpy(ver, "FST");
		break; 
	case CMCS_CTP_F917_PR1:	//0x50
	case CMCS_CTP_F917_PR2:
	case CMCS_CTP_F917_PR3:
	case CMCS_CTP_F917_PR4:
	case CMCS_CTP_F917_PR5:
	case CMCS_CTP_F917_MP1:
	case CMCS_CTP_F917_MP2:
	case CMCS_CTP_F917_MP3:
		strcpy(ver, "F17");
		break;
	case CMCS_125_CTP_GRE_PR1: //0x60
	case CMCS_125_CTP_GRE_PR2:
	case CMCS_125_CTP_GRE_MP1:
	case CMCS_125_CTP_GRE_MP2:
		strcpy(ver, "GRE");
		break;
	case CMCS_125_FA9_PR1: //0x70
	case CMCS_125_FA9_PR2:
	case CMCS_125_FA9_PR3:
	case CMCS_125_FA9_MP1:
		strcpy(ver, "F19");
		break;
	case CMCS_125_4G4G_FAA_PR1: //0x80
	case CMCS_125_4G4G_FAA_PR2:
	case CMCS_125_4G4G_FAA_PR3:
	case CMCS_125_4G4G_FAA_MP1:
	case CMCS_15_4G4G_FAA_PR2:  //0x84
	case CMCS_15_4G4G_FAA_PR3:
	case CMCS_15_4G4G_FAA_MP1:
	case CMCS_128_4G4G_FAA_PR1: //0x87
	case CMCS_128_4G4G_FAA_PR2:
	case CMCS_128_4G4G_FAA_PR3:
	case CMCS_128_4G4G_FAA_MP1:
		strcpy(ver, "FAA");
		break; 	
	case CMCS_MQ4_PR0: //0x90
	case CMCS_MQ4_18_PR1:
	case CMCS_MQ4_18_PR2:
	case CMCS_MQ4_18_PR3:
	case CMCS_MQ4_25_PR1:
	case CMCS_MQ4_25_PR2:
	case CMCS_MQ4_25_PR3:
		strcpy(ver, "MQ4");
		break; 
	case CMCS_128_FM6_PR1: //0x100
	case CMCS_128_FM6_PR2:
	case CMCS_125_FM6_PR2:
	case CMCS_128_FM6_PR3:
	case CMCS_125_FM6_PR3:
        case CMCS_145_FM6_PR3:
        case CMCS_128_FM6_8L:
        case CMCS_125_FM6_8L:
        case CMCS_145_FM6_8L:
	case CMCS_128_FM6_MP:
	case CMCS_125_FM6_MP:
		strcpy(ver, "FM6");
		break; 
	/* Domino Q */
	case CMCS_DOMINO_Q_W_PR1: // 0x110
		strcpy(ver, "DMQ");
		break;
	/* Domino Plus */
	case CMCS_DOMINO_PLUS_EG_PR1: // 0x120
	case CMCS_DOMINO_PLUS_WG_PR1: // 0x130
		strcpy(ver, "DMP");
		break;
	/*** 7627 definition start from 0x500 ***/
	case CMCS_7627_ORIG_EVB1: //0x500
		strcpy(ver, "F13");
		break;    
	case CMCS_7627_F905_PR1:
	case CMCS_7627_F905_PR2:
	case CMCS_7627_F905_PR3:
	case CMCS_7627_F905_PR4:
	case CMCS_7627_F905_PR5:
		strcpy(ver, "F05");
		break; 
	case CMCS_7627_F913_PR1:
	case CMCS_7627_F913_PR2:
	case CMCS_7627_F913_PR3:
	case CMCS_7627_F913_PR4:
	case CMCS_7627_F913_PR5:
	case CMCS_7627_F913_MP1_W: 
	case CMCS_7627_F913_MP1_C_G:
	case CMCS_7627_F913_MP1_W_4G4G:
	case CMCS_7627_F913_MP1_C_G_4G4G:
	case CMCS_7627_F913_PCB_W: // 0x510
	case CMCS_7627_F913_PCB_C_G:
	case CMCS_7627_F913_HAIER_C:
		strcpy(ver, "F13");
		break;
	case CMCS_7627_F20_PR1: //0x520
	case CMCS_7627_F20_PR2:
	case CMCS_7627_F20_PR3:
	case CMCS_7627_F20_MP1:
		strcpy(ver, "F20");
		break;
	case CMCS_FN6_ORIG_PR1: //0x600
	case CMCS_FN6_ORIG_PR2:
	case CMCS_FN6_ORIG_PR3:
	case CMCS_FN6_ORIG_MP1:
		strcpy(ver, "FN6");
		break;
	default:
		strcpy(ver, "Unkonwn Device Model");
		break;
	}
/* FIH, JiaHao, 2010/08/20 } */

	len = snprintf(page, PAGE_SIZE, "%s\n", ver);

	return proc_calc_metrics(page, start, off, count, eof, len);
}

static int baseband_read_proc(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{
	int len;
	int HWID = FIH_READ_ORIG_HWID_FROM_SMEM();
	char ver[24];
	
/* FIH, JiaHao, 2010/08/20 { */
	switch (HWID){
	case CMCS_HW_EVB1: strcpy(ver, "EVB1"); break; //0x0
	case CMCS_HW_EVB2: strcpy(ver, "EVB2"); break;
	case CMCS_HW_EVB3: strcpy(ver, "EVB3");	break;
	case CMCS_HW_EVB4: strcpy(ver, "EVB4"); break;
	case CMCS_HW_EVB5: strcpy(ver, "EVB5");	break;
	case CMCS_ORIG_RTP_PR1: strcpy(ver, "PR1"); break; //0x5
	case CMCS_ORIG_CTP_PR1: strcpy(ver, "PR1"); break; //0xd
	/* 850 family */
	case CMCS_850_RTP_PR2: strcpy(ver, "PR2_850"); break; //0x10
	case CMCS_850_RTP_PR3: strcpy(ver, "PR3_850"); break;
	case CMCS_850_RTP_PR4: strcpy(ver, "PR4_850"); break;
	case CMCS_850_RTP_PR5: strcpy(ver, "PR5_850"); break;
	case CMCS_850_RTP_MP1: strcpy(ver, "MP1_850"); break;
	case CMCS_850_RTP_MP2: strcpy(ver, "MP2_850"); break;
	case CMCS_850_RTP_MP3: strcpy(ver, "MP3_850"); break;
	case CMCS_850_CTP_PR2: strcpy(ver, "PR2_850"); break; //0x17
	case CMCS_850_CTP_PR3: strcpy(ver, "PR3_850"); break;
	case CMCS_850_CTP_PR4: strcpy(ver, "PR4_850"); break;
	case CMCS_850_CTP_PR5: strcpy(ver, "PR5_850"); break;
	case CMCS_850_CTP_MP1: strcpy(ver, "MP1_850"); break;
	case CMCS_850_CTP_MP2: strcpy(ver, "MP2_850"); break;
	case CMCS_850_CTP_MP3: strcpy(ver, "MP3_850"); break;
	/* 900 family */
	case CMCS_900_RTP_PR2: strcpy(ver, "PR2_900"); break; //0x20
	case CMCS_900_RTP_PR3: strcpy(ver, "PR3_900"); break;
	case CMCS_900_RTP_PR4: strcpy(ver, "PR4_900"); break;
	case CMCS_900_RTP_PR5: strcpy(ver, "PR5_900"); break;
	case CMCS_900_RTP_MP1: strcpy(ver, "MP1_900"); break;
	case CMCS_900_RTP_MP2: strcpy(ver, "MP2_900"); break;
	case CMCS_900_RTP_MP3: strcpy(ver, "MP3_900"); break;
	case CMCS_900_CTP_PR2: strcpy(ver, "PR2_900"); break; //0x27
	case CMCS_900_CTP_PR3: strcpy(ver, "PR3_900"); break;
	case CMCS_900_CTP_PR4: strcpy(ver, "PR4_900"); break;
	case CMCS_900_CTP_PR5: strcpy(ver, "PR5_900"); break;
	case CMCS_900_CTP_MP1: strcpy(ver, "MP1_900"); break;
	case CMCS_900_CTP_MP2: strcpy(ver, "MP2_900"); break;
	case CMCS_900_CTP_MP3: strcpy(ver, "MP3_900"); break;
	/* AWS family */
	case CMCS_145_CTP_PR1: strcpy(ver, "PR1_850"); break; //0x2E
	/* FST family */
	case CMCS_125_FST_PR1: strcpy(ver, "PR1_850"); break; //0x30
	case CMCS_125_FST_PR2: strcpy(ver, "PR2_850"); break;
	case CMCS_125_FST_MP1: strcpy(ver, "MP1_850"); break;
	case CMCS_128_FST_PR1: strcpy(ver, "PR1_900"); break; //0x40
	case CMCS_128_FST_PR2: strcpy(ver, "PR2_900"); break;
	case CMCS_128_FST_MP1: strcpy(ver, "MP1_900"); break;
	/* F917 family */
	case CMCS_CTP_F917_PR1: strcpy(ver, "PR1"); break; //0x50
	case CMCS_CTP_F917_PR2: strcpy(ver, "PR2"); break;
	case CMCS_CTP_F917_PR3: strcpy(ver, "PR3"); break;
	case CMCS_CTP_F917_PR4: strcpy(ver, "PR4"); break;
	case CMCS_CTP_F917_PR5: strcpy(ver, "PR5"); break;
	case CMCS_CTP_F917_MP1: strcpy(ver, "MP1"); break;
	case CMCS_CTP_F917_MP2: strcpy(ver, "MP2"); break;
	case CMCS_CTP_F917_MP3: strcpy(ver, "MP3"); break;
	/* Greco family */
	case CMCS_125_CTP_GRE_PR1: strcpy(ver, "PR1"); break; //0x60
	case CMCS_125_CTP_GRE_PR2: strcpy(ver, "PR2"); break;
	case CMCS_125_CTP_GRE_MP1: strcpy(ver, "MP1"); break;
	case CMCS_125_CTP_GRE_MP2: strcpy(ver, "MP2"); break;
	/* FA9 family*/
	case CMCS_125_FA9_PR1: strcpy(ver, "PR1"); break; //0x70
	case CMCS_125_FA9_PR2: strcpy(ver, "PR2"); break;
	case CMCS_125_FA9_PR3: strcpy(ver, "PR3"); break;
	case CMCS_125_FA9_MP1: strcpy(ver, "MP1"); break;
	/* FAA family */
	case CMCS_125_4G4G_FAA_PR1: strcpy(ver, "PR1_125"); break; //0x80
	case CMCS_125_4G4G_FAA_PR2: strcpy(ver, "PR2_125"); break;
	case CMCS_125_4G4G_FAA_PR3: strcpy(ver, "PR3_125"); break;
	case CMCS_125_4G4G_FAA_MP1: strcpy(ver, "MP1_125"); break;
	case CMCS_15_4G4G_FAA_PR2:  strcpy(ver, "PR2_15");  break; //0x84
	case CMCS_15_4G4G_FAA_PR3:  strcpy(ver, "PR3_15");  break;
	case CMCS_15_4G4G_FAA_MP1:  strcpy(ver, "MP1_15");  break;
	case CMCS_128_4G4G_FAA_PR1: strcpy(ver, "PR1_128"); break; //0x87
	case CMCS_128_4G4G_FAA_PR2: strcpy(ver, "PR2_128"); break;
	case CMCS_128_4G4G_FAA_PR3: strcpy(ver, "PR3_128"); break;
	case CMCS_128_4G4G_FAA_MP1: strcpy(ver, "MP1_128"); break;
	/* MQ4 familiy */
	case CMCS_MQ4_PR0:    strcpy(ver, "PR0");    break; //0x90
	case CMCS_MQ4_18_PR1: strcpy(ver, "PR1_18"); break;
	case CMCS_MQ4_18_PR2: strcpy(ver, "PR2_18"); break;
	case CMCS_MQ4_18_PR3: strcpy(ver, "PR3_18"); break;
	case CMCS_MQ4_25_PR1: strcpy(ver, "PR1_25"); break;
	case CMCS_MQ4_25_PR2: strcpy(ver, "PR2_25"); break;
	case CMCS_MQ4_25_PR3: strcpy(ver, "PR3_25"); break;
	/* FM6 family */
	case CMCS_128_FM6_PR1: strcpy(ver, "PR1_128"); break; //0x100
	case CMCS_128_FM6_PR2: strcpy(ver, "PR2_128"); break;
	case CMCS_125_FM6_PR2: strcpy(ver, "PR2_125"); break;
	case CMCS_128_FM6_PR3: strcpy(ver, "PR3_128"); break;
	case CMCS_125_FM6_PR3: strcpy(ver, "PR3_125"); break;
	case CMCS_145_FM6_PR3: strcpy(ver, "PR3_145"); break;
	case CMCS_128_FM6_8L:  strcpy(ver, "PR3.7_128");  break;
	case CMCS_125_FM6_8L:  strcpy(ver, "PR3.7_125");  break;
	case CMCS_145_FM6_8L:  strcpy(ver, "PR3.7_145");  break;
	case CMCS_128_FM6_MP:  strcpy(ver, "MP_128");  break;
	case CMCS_125_FM6_MP:  strcpy(ver, "MP_125");  break;
	/* Domino Q */
	case CMCS_DOMINO_Q_W_PR1: strcpy(ver, "PR1_W"); break; // 0x110
	/* Domino Plus */
	case CMCS_DOMINO_PLUS_EG_PR1: strcpy(ver, "PR1_EG"); break; // 0x120
	case CMCS_DOMINO_PLUS_WG_PR1: strcpy(ver, "PR1_WG"); break; // 0x130
	/*** 7627 definition start from 0x500 ***/
	case CMCS_7627_ORIG_EVB1: strcpy(ver, "EVB1"); break; //0x500
	/* F905 family */
	case CMCS_7627_F905_PR1: strcpy(ver, "PR1"); break;
	case CMCS_7627_F905_PR2: strcpy(ver, "PR2"); break;
	case CMCS_7627_F905_PR3: strcpy(ver, "PR3"); break;
	case CMCS_7627_F905_PR4: strcpy(ver, "PR4"); break;
	case CMCS_7627_F905_PR5: strcpy(ver, "PR5"); break;
	/* F913 family */
	case CMCS_7627_F913_PR1: strcpy(ver, "PR1"); break;
	case CMCS_7627_F913_PR2: strcpy(ver, "PR2"); break;
	case CMCS_7627_F913_PR3: strcpy(ver, "PR3"); break;
	case CMCS_7627_F913_PR4: strcpy(ver, "PR4"); break;
	case CMCS_7627_F913_PR5: strcpy(ver, "PR5"); break;
	case CMCS_7627_F913_MP1_W:
	case CMCS_7627_F913_MP1_W_4G4G: strcpy(ver, "MP1_W"); break;
	case CMCS_7627_F913_MP1_C_G:
	case CMCS_7627_F913_MP1_C_G_4G4G: strcpy(ver, "MP1_C_G"); break;
	case CMCS_7627_F913_PCB_W:   strcpy(ver, "MP1_W"); break; //0x510
	case CMCS_7627_F913_PCB_C_G: strcpy(ver, "MP1_C_G"); break;
	case CMCS_7627_F913_HAIER_C: strcpy(ver, "MP1_C"); break;
	/* F20 family */
	case CMCS_7627_F20_PR1: strcpy(ver, "PR1"); break; //0x520
	case CMCS_7627_F20_PR2: strcpy(ver, "PR2"); break;
	case CMCS_7627_F20_PR3: strcpy(ver, "PR3"); break;
	case CMCS_7627_F20_MP1: strcpy(ver, "MP1"); break;
	/* FN6 family */
	case CMCS_FN6_ORIG_PR1: strcpy(ver, "PR1"); break; //0x600
	case CMCS_FN6_ORIG_PR2: strcpy(ver, "PR2"); break;
	case CMCS_FN6_ORIG_PR3: strcpy(ver, "PR3"); break;
	case CMCS_FN6_ORIG_MP1: strcpy(ver, "MP1"); break;
	default:
		strcpy(ver, "Unkonwn Baseband version");
		break;
	}
/* FIH, JiaHao, 2010/08/20 } */

	len = snprintf(page, PAGE_SIZE, "%s\n", ver);

	return proc_calc_metrics(page, start, off, count, eof, len);
}

static int device_HWSpec_read_proc(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{
	int len;
	int HWID = FIH_READ_ORIG_HWID_FROM_SMEM();
	char ver[24];

/* FIH, JiaHao, 2010/08/20 { */
	switch (HWID){
	case CMCS_HW_EVB1: //0x0
	case CMCS_HW_EVB2:
	case CMCS_HW_EVB3:
	case CMCS_HW_EVB4:
	case CMCS_HW_EVB5:
		strcpy(ver, "R-_-R");
		break; 
	case CMCS_ORIG_RTP_PR1: //0x5
		if (JogballExist_pr1)
			strcpy(ver, "R-J-R");
		else
			strcpy(ver, "R-_-R");
		break; 
	case CMCS_ORIG_CTP_PR1: //0xd
		if (JogballExist_pr1)
			strcpy(ver, "M-J-C");
		else
			strcpy(ver, "M-_-C");
		break; 
	case CMCS_850_RTP_PR2: //0x10
	case CMCS_850_RTP_PR3:
	case CMCS_850_RTP_PR4:
	case CMCS_850_RTP_PR5:
	case CMCS_850_RTP_MP1:
	case CMCS_850_RTP_MP2:
	case CMCS_850_RTP_MP3:
		if (JogballExist_pr2)
			strcpy(ver, "R-J-R");
		else
			strcpy(ver, "R-_-R");
		break; 
	case CMCS_850_CTP_PR2: //0x17
	case CMCS_850_CTP_PR3:
	case CMCS_850_CTP_PR4:
	case CMCS_850_CTP_PR5:
	case CMCS_850_CTP_MP1:
	case CMCS_850_CTP_MP2:
	case CMCS_850_CTP_MP3:
		if (JogballExist_pr2)
			strcpy(ver, "M-J-C");
		else
			strcpy(ver, "M-_-C");
		break; 
	case CMCS_900_RTP_PR2: //0x20
	case CMCS_900_RTP_PR3:
	case CMCS_900_RTP_PR4:
	case CMCS_900_RTP_PR5:
	case CMCS_900_RTP_MP1:
	case CMCS_900_RTP_MP2:
	case CMCS_900_RTP_MP3:
		if (JogballExist_pr2)
			strcpy(ver, "R-J-R");
		else
			strcpy(ver, "R-_-R");
		break; 
	case CMCS_900_CTP_PR2: //0x27
	case CMCS_900_CTP_PR3:
	case CMCS_900_CTP_PR4:
	case CMCS_900_CTP_PR5:
	case CMCS_900_CTP_MP1:
	case CMCS_900_CTP_MP2:
	case CMCS_900_CTP_MP3:
		if (JogballExist_pr2)
			strcpy(ver, "M-J-C");
		else
			strcpy(ver, "M-_-C");
		break; 
	case CMCS_145_CTP_PR1: //0x2E
		strcpy(ver,  "M-J-C");
		break; 
	case CMCS_125_FST_PR1: //0x30
	case CMCS_125_FST_PR2:
	case CMCS_125_FST_MP1:
	case CMCS_128_FST_PR1: //0x40
	case CMCS_128_FST_PR2:
	case CMCS_128_FST_MP1:
		strcpy(ver, "M-_-C");
		break; 
	case CMCS_CTP_F917_PR1:	//0x50
	case CMCS_CTP_F917_PR2:		
	case CMCS_CTP_F917_PR3:	
	case CMCS_CTP_F917_PR4:	
	case CMCS_CTP_F917_PR5:
	case CMCS_CTP_F917_MP1:	
	case CMCS_CTP_F917_MP2:	
	case CMCS_CTP_F917_MP3:
		strcpy(ver, "M-_-C");
		break; 	
	case CMCS_125_CTP_GRE_PR1: //0x60
	case CMCS_125_CTP_GRE_PR2:
	case CMCS_125_CTP_GRE_MP1:
	case CMCS_125_CTP_GRE_MP2:
		if (JogballExist_pr2)
			strcpy(ver, "M-J-C");
		else
			strcpy(ver, "M-_-C");
		break;
	case CMCS_125_FA9_PR1: //0x70
	case CMCS_125_FA9_PR2:
	case CMCS_125_FA9_PR3:
	case CMCS_125_FA9_MP1:
		strcpy(ver, "M-_-C");
		break;
	case CMCS_125_4G4G_FAA_PR1: //0x80
	case CMCS_125_4G4G_FAA_PR2:
	case CMCS_125_4G4G_FAA_PR3:
	case CMCS_125_4G4G_FAA_MP1:
	case CMCS_15_4G4G_FAA_PR2:  //0x84
	case CMCS_15_4G4G_FAA_PR3:
	case CMCS_15_4G4G_FAA_MP1:
	case CMCS_128_4G4G_FAA_PR1: //0x87
	case CMCS_128_4G4G_FAA_PR2:
	case CMCS_128_4G4G_FAA_PR3:
	case CMCS_128_4G4G_FAA_MP1:
		strcpy(ver, "M-_-C");
		break;
	case CMCS_MQ4_PR0: //0x90
	case CMCS_MQ4_18_PR1:
	case CMCS_MQ4_18_PR2:
	case CMCS_MQ4_18_PR3:
	case CMCS_MQ4_25_PR1:
	case CMCS_MQ4_25_PR2:
	case CMCS_MQ4_25_PR3:
		strcpy(ver, "M-_-C");
		break;
	case CMCS_128_FM6_PR1: //0x100
	case CMCS_128_FM6_PR2:
	case CMCS_125_FM6_PR2:
	case CMCS_128_FM6_PR3:
	case CMCS_125_FM6_PR3:
	case CMCS_145_FM6_PR3:
	case CMCS_128_FM6_8L:
	case CMCS_125_FM6_8L:
	case CMCS_145_FM6_8L:
	case CMCS_128_FM6_MP:
	case CMCS_125_FM6_MP:
		strcpy(ver, "M-_-C");
		break;
	/* Domino Q */
	case CMCS_DOMINO_Q_W_PR1: // 0x110
		strcpy(ver, "M-_-C");
		break;
	/* Domino Plus */
	case CMCS_DOMINO_PLUS_EG_PR1: // 0x120
	case CMCS_DOMINO_PLUS_WG_PR1: // 0x130
		strcpy(ver, "M-_-C");
		break;
	/*** 7627 definition start from 0x500 ***/
	case CMCS_7627_ORIG_EVB1: //0x500
		strcpy(ver, "M-_-C");
		break;    
	case CMCS_7627_F905_PR1:
	case CMCS_7627_F905_PR2:
	case CMCS_7627_F905_PR3:
	case CMCS_7627_F905_PR4:
	case CMCS_7627_F905_PR5:
		strcpy(ver, "M-_-C");
		break; 
	case CMCS_7627_F913_PR1:
	case CMCS_7627_F913_PR2:
	case CMCS_7627_F913_PR3:
	case CMCS_7627_F913_PR4:
	case CMCS_7627_F913_PR5:
	case CMCS_7627_F913_MP1_W:
	case CMCS_7627_F913_MP1_C_G:
	case CMCS_7627_F913_MP1_W_4G4G:
	case CMCS_7627_F913_MP1_C_G_4G4G:
	case CMCS_7627_F913_PCB_W: //0x510
	case CMCS_7627_F913_PCB_C_G:
	case CMCS_7627_F913_HAIER_C:
		strcpy(ver, "M-_-C");
		break;
	case CMCS_7627_F20_PR1: //0x520
	case CMCS_7627_F20_PR2:
	case CMCS_7627_F20_PR3:
	case CMCS_7627_F20_MP1:
		strcpy(ver, "M-_-C");
		break;
	case CMCS_FN6_ORIG_PR1: //0x600
	case CMCS_FN6_ORIG_PR2:
	case CMCS_FN6_ORIG_PR3:
	case CMCS_FN6_ORIG_MP1:
		strcpy(ver, "M-_-C");
		break; 
	default:
		strcpy(ver, "Unkonwn HW Spec");
		break;
	}
/* FIH, JiaHao, 2010/08/20 } */

	len = snprintf(page, PAGE_SIZE, "%s\n",	ver);

	return proc_calc_metrics(page, start, off, count, eof, len);
}

#ifdef POWER_ON_CAUSE_PROC_READ_ENTRY
static int proc_read_power_on_cause(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{
	int len;
	uint32_t poc = FIH_READ_POWER_ON_CAUSE();

	len = snprintf(page, PAGE_SIZE, "0x%x\n", poc);

	return proc_calc_metrics(page, start, off, count, eof, len);	
}
#endif
//FIH, JamesKCTung, 2009/11/03 +++
/* Add dbgmask partition*/
#ifdef CONFIG_PRINTK
static int proc_read_debug_mask(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{

	int i,j;
	
	for (i=0,j=0; i<MASKSIZE; i++,j=j+2)
		sprintf((page+j),"%02x",debug_mask[i]);
	
	return MASKSIZE;
}
static int proc_write_debug_mask(struct file *file, const char *buffer, 
				 unsigned long count, void *data)
{
	//char mask[16];
/*FIHTDC, WillChen add for debug mask { */
	printk("proc_write_debug_mask()\n");
	//if ( copy_from_user(debug_mask,buffer,count))
	//	return -EFAULT;
	if (buffer)
		memcpy(debug_mask,buffer,count);

	return count;
/*FIHTDC, WillChen add for debug mask } */
}
#endif
//FIH, JamesKCTung, 2009/11/03 ---
static struct {
		char *name;
		int (*read_proc)(char*,char**,off_t,int,int*,void*);
} *p, adq_info[] = {
	{"socinfo", build_version_read_proc},
	{"devmodel", device_model_read_proc},
	{"baseband", baseband_read_proc},
	{"hwspec", device_HWSpec_read_proc},
	#ifdef POWER_ON_CAUSE_PROC_READ_ENTRY
	{"poweroncause", proc_read_power_on_cause},
	#endif
	{NULL,},
};

void adq_info_init(void)
{	
	JogballExist_pr1 = 0;
	JogballExist_pr2 = 0;

	if(!gpio_get_value(PLUS_X_GPIO) || !gpio_get_value(NEG_X_GPIO) ||
	    !gpio_get_value(PLUS_Y_GPIO) || !gpio_get_value(NEG_Y_GPIO)) 
	{
		JogballExist_pr1 = 1;
		printk("PLUS_X_GPIO=%d\n",gpio_get_value(PLUS_X_GPIO));
		printk("NEG_X_GPIO=%d\n",gpio_get_value(NEG_X_GPIO));
		printk("PLUS_Y_GPIO=%d\n",gpio_get_value(PLUS_Y_GPIO));
		printk("NEG_Y_GPIOd=%d\n",gpio_get_value(NEG_Y_GPIO));
		printk("JogballExist1=%d\n",JogballExist_pr1);
	}

	if(!gpio_get_value(NEG_Y_GPIO)) 
	{
		JogballExist_pr2 = 1;
		printk("JogballExist2=%d\n",JogballExist_pr2);
	}
		
	for (p = adq_info; p->name; p++)
		create_proc_read_entry(p->name, 0, NULL, p->read_proc, NULL);
		
//FIH, JamesKCTung, 2009/11/03 +++
/* Add dbgmask partition*/
#ifdef CONFIG_PRINTK
	mask_file = create_proc_entry("debug_mask",0777,NULL);
	mask_file->read_proc = proc_read_debug_mask;
	mask_file->write_proc = proc_write_debug_mask;
	/* FIH, JiaHao, 2010/08/20 { */
	/* ./android/kernel/include/linux/proc_fs.h no define owner at 6030cs */
	//mask_file->owner = THIS_MODULE;
	/* FIH, JiaHao, 2010/08/20 } */
#endif
//FIH, JamesKCTung, 2009/11/03 ---
}
EXPORT_SYMBOL(adq_info_init);

void adq_info_remove(void)
{
	for (p = adq_info; p->name; p++)
		remove_proc_entry(p->name, NULL);
}
EXPORT_SYMBOL(adq_info_remove);
