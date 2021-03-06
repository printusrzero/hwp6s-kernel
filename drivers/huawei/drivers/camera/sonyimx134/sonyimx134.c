/*
 * sonyimx134 sensor driver
 *
 *  Author: 	Zhoujie (zhou.jie1981@163.com)
 *  Date:  	2013/01/05
 *  Version:	1.0
 *  History:	2013/01/05      Frist add driver for sonyimx134
 *
 * ----------------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/videodev2.h>
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/unistd.h>
#include <linux/uaccess.h>
#include <asm/div64.h>
#include <mach/hisi_mem.h>
#include "mach/hardware.h"
#include <mach/boardid.h>
#include <mach/gpio.h>
#include "sensor_common.h"
#include "sonyimx134.h"
#include <asm/bug.h>
#include <linux/device.h>

#define LOG_TAG "SONYIMX134"
//#define DEBUG_DEBUG 1
#include "cam_log.h"
#include <cam_util.h>
#include "drv_regulator_user.h"
#include "hsad/config_interface.h"
#include <mach/pmussi_drv.h>
#include "effect.h"

//#define _IS_DEBUG_AE 0	
#define SONYIMX134_ZSL                       (0x00)
/*add for set awb gain begin*/
#define DIG_GAIN_GR_H                        0x020E
#define DIG_GAIN_GR_L                        0x020F
#define DIG_GAIN_R_H                         0x0210
#define DIG_GAIN_R_L                         0x0211
#define DIG_GAIN_B_H                         0x0212
#define DIG_GAIN_B_L                         0x0213
#define DIG_GAIN_GB_H                        0x0214
#define DIG_GAIN_GB_L                        0x0215

#define WD_COARSE_INTEG_TIME_DS_H     		 (0x0230)
#define WD_COARSE_INTEG_TIME_DS_L     		 (0x0231)
#define WD_ANA_GAIN_DS                    	 (0x0233)

#define ATR_OFF_SETTING_1             		 (0x446D)
#define ATR_OFF_SETTING_2             		 (0x446E)
#define TRIGGER_SETTING                  	 (0x446C)

#define WB_LMT_REG_H                         (0x441E)
#define WB_LMT_REG_L                         (0x441F)

#define AE_SAT_REG_H                         (0x4446)
#define AE_SAT_REG_L                         (0x4447)

/*add for set awb gain begin*/
#define SONYIMX134_ABS_GAIN_B_H				 (0x0716)
#define SONYIMX134_ABS_GAIN_B_L				 (0x0717)
#define SONYIMX134_ABS_GAIN_GB_H			 (0x0718)
#define SONYIMX134_ABS_GAIN_GB_L			 (0x0719)
#define SONYIMX134_ABS_GAIN_GR_H			 (0x0712)
#define SONYIMX134_ABS_GAIN_GR_L			 (0x0713)
#define SONYIMX134_ABS_GAIN_R_H				 (0x0714)
#define SONYIMX134_ABS_GAIN_R_L				 (0x0715)

#define GROUP_HOLD_FUNCTION_REG   			 (0x0104)
#define SONYIMX134_MAX_ISO 			         1550
#define SONYIMX134_MIN_ISO                   100

/* support 3 level framerate now, if mid fps same as min, set to 2 level framerate */
#define SONYIMX134_AUTOFPS_GAIN_HIGH2MID		0x24
#define SONYIMX134_AUTOFPS_GAIN_MID2LOW		    0x24
#define SONYIMX134_AUTOFPS_GAIN_LOW2MID		    0x60
#define SONYIMX134_AUTOFPS_GAIN_MID2HIGH		0x60

#define SONYIMX134_MAX_FRAMERATE                30
#define SONYIMX134_MID_FRAMERATE		        15
#define SONYIMX134_MIN_FRAMERATE                10
#define SONYIMX134_MIN_CAP_FRAMERATE            8
#define SONYIMX134_FLASH_TRIGGER_GAIN           0xff
#define SONYIMX134_SHARPNESS_PREVIEW            0x30
#define SONYIMX134_SHARPNESS_CAPTURE            0x08

#define SONYIMX134_SLAVE_ADDRESS                0x20
#define SONYIMX134_CHIP_ID                      (0x0134)

#define SONYIMX134_FLIP		                    0x0101

#define SONYIMX134_EXPOSURE_REG_H	            0x0202
#define SONYIMX134_EXPOSURE_REG_L	            0x0203
#define SONYIMX134_GAIN_REG_H		            0x0204
#define SONYIMX134_GAIN_REG_L		            0x0205

#define SONYIMX134_VTS_REG_H		            0x0340
#define SONYIMX134_VTS_REG_L		            0x0341

#define SONYIMX134_APERTURE_FACTOR              200 //F2.0
#define SONYIMX134_EQUIVALENT_FOCUS	            0

#define SONYIMX134_AP_WRITEAE_MODE
#define SONYIMX134_MAX_ANALOG_GAIN	            8

#define SONYIMX134_DPC_THRESHOLD_ISO		400

#define SONYIMX134_I2C_RETRY_TIMES               5
enum sensor_module_type
{
	MODULE_LITEON,
	MODULE_SUNNY,
	MODULE_UNSUPPORT
};

static u8 sensor_module;
#ifdef IMX134_OTP
#define OTP_VCM  		      0xa6
#define OTP_VCM_REG 	      0x40
#define OTP_ID_AWB  	      0xa4
#define OTP_ID_REG 		      0x00
#define OTP_AWB_REG 	      0x05
#define OTP_LSC_1  		      0xa4
#define OTP_LSC_2  		      0xa6
#define OTP_LSC_1_REG 	      0x0b
#define OTP_LSC_2_REG	      0x00
#define OTP_CHECKSUM          0xa6
#define OTP_CHECKSUM_REG      0x44
#define OTP_CHECKSUM_FLAG_REG 0x45

#define SONYIMX134_OTP_ID_READ				(1 << 0)
#define SONYIMX134_OTP_VCM_READ				(1 << 1)
#define SONYIMX134_OTP_LSC_READ				(1 << 2)
#define SONYIMX134_OTP_LSC_WRITED			(1 << 3)
#define SONYIMX134_OTP_LSC_FILE_ERR         (1 << 4)
#define SONYIMX134_OTP_AWB_READ				(1 << 5)
#define SONYIMX134_OTP_CHECKSUM_ERR			(1 << 6)
#define SONYIMX134_OTP_CHECKSUM_READ		(1 << 7)

#define SONYIMX134_SENSOR_LSC_MODE			0x0700
#define SONYIMX134_SENSOR_LSC_EN			0x4500
#define SONYIMX134_SENSOR_RAM_SEL			0x3A63

#define SONYIMX134_SENSOR_LSC_RAM			0x4800

//7*5*4*2
#define SONYIMX134_OTP_LSC_SIZE             280
static u8 sonyimx134_otp_flag   = 0;
/* VCM start and end values */
static u16 sonyimx134_vcm_start = 0;
static u16 sonyimx134_vcm_end   = 0;

static u32 OTPSUMVAL            = 0;
static u8  OTPCHECKSUMVAL       = 0;
static u8  OTPCHECKSUMFLAG      = 0;
#define OTPCHECKSUM_FLAG                    0xA5

static u8 sonyimx134_otp_lsc_param[SONYIMX134_OTP_LSC_SIZE] ;
extern int ispv1_read_sensor_byte_addr8(i2c_index_t index, u8 i2c_addr, u16 reg, u16 *val, i2c_length length);//add by zhoujie
static void sonyimx134_get_otp_from_sensor(void);
static void sonyimx134_otp_get_vcm(u16 *vcm_start, u16 *vcm_end);
static bool sonyimx134_otp_enable_lsc(bool enable);
static bool sonyimx134_otp_set_lsc(void);
#endif

static camera_capability sonyimx134_cap[] = {
	{V4L2_CID_FLASH_MODE, THIS_FLASH},
	{V4L2_CID_FOCUS_MODE, THIS_FOCUS_MODE},
	{V4L2_CID_FOCAL_LENGTH, 296},//2.96mm
	//{V4L2_CID_ZSL,		  SONYIMX134_ZSL},
};

static awb_gain_t flash_platform_awb[] = {
	{0xbd, 0x80, 0x80, 0xdc}, /*EDGE*/
};

/*
 * should be calibrated, three lights, from 0x1c264
 * here is long exposure
 */
char sonyimx134_lensc_param[86*3] = {
};

/* should be calibrated, 6 groups 3x3, from 0x1c1d8 */
short sonyimx134_ccm_param[54] = {
};

char sonyimx134_awb_param[] = {
};

static vcm_info_s vcm_dw9714_Sunny = {
    #include "vcm_dw9714_Sunny.h"
};

static vcm_info_s vcm_dw9714_Liteon = {
    #include "vcm_dw9714_Liteon.h"
};

static effect_params effect_imx134_sunny = {
    #include "effect_imx134_sunny_edge.h"
};

static effect_params effect_imx134_liteon = {
    #include "effect_imx134_liteon_edge.h"
};

//static effect_params  *effect=NULL; //z00174260

static framesize_s sonyimx134_framesizes[] = {
    /*1280x720, hdr movie*/
    {0, 0, 1280, 720, 3600, 2518, 30, 10, 0x2F3, 0x275,0x100, VIEW_HDR_MOVIE, RESOLUTION_16_9,false,false,{ (sensor_reg_t *)sonyimx134_rgb_framesize_1280x720_HDR, ARRAY_SIZE(sonyimx134_rgb_framesize_1280x720_HDR)} },

    /*1600x1200, hdr movie*/
	{0, 0, 1600, 1200, 3600, 2518, 30, 10, 0x2F3, 0x275,0x100, VIEW_HDR_MOVIE, RESOLUTION_4_3,false,false,{ (sensor_reg_t *)sonyimx134_rgb_framesize_1600x1200_HDR, ARRAY_SIZE(sonyimx134_rgb_framesize_1600x1200_HDR)} },
        
	/* 1600x1200, just close with quarter size */
	{0, 0, 1600, 1200, 3600, 1480, 30, 30, 0x1BC, 0x172, 0x100, VIEW_FULL, RESOLUTION_4_3, false, false,{ (sensor_reg_t *)sonyimx134_framesize_1600x1200, ARRAY_SIZE(sonyimx134_framesize_1600x1200)} }, 

	/* 1920x1088, 30fps for 1080P video, edge & sony mipi 720 */
    /* hts will scale with image (reg 0x0340=2592,hts=2592/1.6875=1536),vts will not. */
	{0, 0, 1920, 1088, 3600, 2592, 30, 30, 0x309, 0x288, 0x100, VIEW_FULL, RESOLUTION_16_9, false, false, { (sensor_reg_t *)sonyimx134_framesize_1920x1088, ARRAY_SIZE(sonyimx134_framesize_1920x1088)} },

	/* full size 30fps for zsl mode */
	{0, 0, 3264, 2448, 3600, 2774, 30, 30, 0x341, 0x2B6, 0x1E0, VIEW_FULL, RESOLUTION_4_3, false, true,{ (sensor_reg_t *)sonyimx134_framesize_zsl_full, ARRAY_SIZE(sonyimx134_framesize_zsl_full)} },

	/* full size 15fps cs, es will revise a little(not correct 13fps) */
	{0, 0, 3264, 2448, 4150, 2962, 13, 15, 0x181, 0x141, 0xDE, VIEW_FULL, RESOLUTION_4_3, false, false,{ (sensor_reg_t *)sonyimx134_framesize_full, ARRAY_SIZE(sonyimx134_framesize_full)} },

    /* 3280x1960@30fps base 3600x2148@580M for zsl mode. FHD 1080P for video*/
    {0, 0, 3280, 1960, 3600, 2148, 30, 30, 0x284, 0x219, 0x173, VIEW_FULL, RESOLUTION_16_9, false, true, { (sensor_reg_t *)sonyimx134_framesize_3280x1960_580M, ARRAY_SIZE(sonyimx134_framesize_3280x1960_580M)} },

    /* 3280x2464@30fps base 3542x2598@690M. not use only for enum 8M fmt*/
    {0, 0, 3280, 2464, 3600, 2718, 28, 28, 0x2FE, 0X27E, 0x1B9, VIEW_FULL, RESOLUTION_4_3, false, true, { (sensor_reg_t *)sonyimx134_framesize_3280x2464_690M, ARRAY_SIZE(sonyimx134_framesize_3280x2464_690M)} },
};

static camera_sensor sonyimx134_sensor;

/*
 **************************************************************************
 * FunctionName: sonyimx134_read_reg;
 * Description : read sonyimx134 reg by i2c;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static int sonyimx134_read_reg(u16 reg, u8 *val)
{
	return k3_ispio_read_reg(sonyimx134_sensor.i2c_config.index,
			 sonyimx134_sensor.i2c_config.addr, reg, (u16*)val, sonyimx134_sensor.i2c_config.val_bits,sonyimx134_sensor.i2c_config.addr_bits);
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_write_reg;
 * Description : write sonyimx134 reg by i2c;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static int sonyimx134_write_reg(u16 reg, u8 val, u8 mask)
{
	//msleep(50); //deleted by c00144034,not sleep in IRQ process.
	return k3_ispio_write_reg(sonyimx134_sensor.i2c_config.index,
			sonyimx134_sensor.i2c_config.addr, reg, val, sonyimx134_sensor.i2c_config.val_bits, mask,sonyimx134_sensor.i2c_config.addr_bits);
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_write_seq;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static int sonyimx134_write_seq(const struct _sensor_reg_t *seq, u32 size, u8 mask)
{
	print_debug("Enter %s, seq[%#x], size=%d", __func__, (int)seq, size);
	return k3_ispio_write_seq(sonyimx134_sensor.i2c_config.index,
			sonyimx134_sensor.i2c_config.addr, seq, size, sonyimx134_sensor.i2c_config.val_bits, mask,sonyimx134_sensor.i2c_config.addr_bits);
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_write_isp_seq;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void sonyimx134_write_isp_seq(const struct isp_reg_t *seq, u32 size)
{
	print_debug("Enter %s, seq[%#x], size=%d", __func__, (int)seq, size);
	k3_ispio_write_isp_seq(seq, size);
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_read_isp_seq;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
 #if 0
static void sonyimx134_read_isp_seq(struct isp_reg_t *seq, u32 size)
{
	print_debug("Enter %s, seq[%#x], size=%d", __func__, (int)seq, size);
	k3_ispio_read_isp_seq(seq, size);
}
#endif

/*
 **************************************************************************
 * FunctionName: sonyimx134_read_isp_reg;
 * Description : Read ISP register;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
 #if 0
static u32 sonyimx134_read_isp_reg(u32 reg_addr, u32 size)
{
	struct isp_reg_t reg_seq[4];
	int i = 0;
	u32 reg_value = 0x00;
	print_debug("Enter %s, size=%d", __func__, size);

	/*initialize buffer */
	for (i = 0; i < size; i++) {
		reg_seq[i].subaddr = reg_addr;
		reg_seq[i].value = 0x00;
		reg_seq[i].mask = 0x00;
		reg_addr++;
	}

	/*read register of isp for imx134 */
	sonyimx134_read_isp_seq(reg_seq, size);
	/*construct return value */
	do {
		i--;
		reg_value = reg_value << 8;
		reg_value |= reg_seq[i].value;
		print_debug("reg_seq[%d].value:%x", i, reg_seq[i].value);
	} while (i > 0);
	return reg_value;
}
#endif
/*
 **************************************************************************
 * FunctionName: sonyimx134_enum_frame_intervals;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx134_enum_frame_intervals(struct v4l2_frmivalenum *fi)
{
    if(!fi) {
        return -EINVAL;
    }

	print_debug("enter %s", __func__);
	if (fi->index >= CAMERA_MAX_FRAMERATE) {
		return -EINVAL;
	}

	fi->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fi->discrete.numerator = 1;
	fi->discrete.denominator = (fi->index + 1);
	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_get_format;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx134_get_format(struct v4l2_fmtdesc *fmt)
{
	if (fmt->type == V4L2_BUF_TYPE_VIDEO_OVERLAY) {
		fmt->pixelformat = sonyimx134_sensor.fmt[STATE_PREVIEW];
	} else {
		fmt->pixelformat = sonyimx134_sensor.fmt[STATE_CAPTURE];
	}
	return 0;
}

static int sonyimx134_get_capability(u32 id, u32 *value)
{
	int i;
	for (i = 0; i < sizeof(sonyimx134_cap) / sizeof(sonyimx134_cap[0]); ++i) {
		if (id == sonyimx134_cap[i].id) {
			*value = sonyimx134_cap[i].value;
			break;
		}
	}
	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_enum_framesizes;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx134_enum_framesizes(struct v4l2_frmsizeenum *framesizes)
{
	u32 max_index = ARRAY_SIZE(camera_framesizes) - 1;
	u32 this_max_index = ARRAY_SIZE(sonyimx134_framesizes) - 1;

    if(!framesizes) {
        return -EINVAL;
    }

	print_debug("enter %s; ", __func__);

	if (framesizes->index > max_index) {
		print_error("framesizes->index = %d error", framesizes->index);
		return -EINVAL;
	}

	if ((camera_framesizes[framesizes->index].width > sonyimx134_framesizes[this_max_index].width)
		|| (camera_framesizes[framesizes->index].height > sonyimx134_framesizes[this_max_index].height)) {
		print_error("framesizes->index = %d error", framesizes->index);
		return -EINVAL;
	}

	framesizes->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	framesizes->discrete.width = sonyimx134_framesizes[this_max_index].width;
	framesizes->discrete.height = sonyimx134_framesizes[this_max_index].height;

	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_try_framesizes;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx134_try_framesizes(struct v4l2_frmsizeenum *framesizes)
{
	int max_index = ARRAY_SIZE(sonyimx134_framesizes) - 1;

    if(!framesizes) {
        return -EINVAL;
    }

	print_debug("Enter Function:%s  ", __func__);

	if ((framesizes->discrete.width <= sonyimx134_framesizes[max_index].width)
	    && (framesizes->discrete.height <= sonyimx134_framesizes[max_index].height)) {
		print_debug("===========width = %d", framesizes->discrete.width);
		print_debug("===========height = %d", framesizes->discrete.height);
		return 0;
	}

	print_error("frame size too large, [%d,%d]",
    framesizes->discrete.width, framesizes->discrete.height);
	return -EINVAL;
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_set_framesizes;
 * Description : NA;
 * Input       : flag: if 1, set framesize to sensor,
 *					   if 0, only store framesize to camera_interface;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx134_set_framesizes(camera_state state,
				 struct v4l2_frmsize_discrete *fs, int flag, camera_setting_view_type view_type,bool zsl_preview)
{
	int i = 0;
	bool match = false;
	int size = 0;
    if(!fs) {
        return -EINVAL;
    }

	print_info("Enter Function:%s State(%d), flag=%d, width=%d, height=%d",
		   __func__, state, flag, fs->width, fs->height);
	size = ARRAY_SIZE(sonyimx134_framesizes);
    if(HDR_MOVIE_ON == sonyimx134_sensor.sensor_hdr_movie.hdrInfo.hdr_on) {
		for (i = 0; i < size; i++) {
			if ((sonyimx134_framesizes[i].width >= fs->width)
			    && (sonyimx134_framesizes[i].height >= fs->height)
			    && (camera_get_resolution_type(fs->width, fs->height)
				   <= sonyimx134_framesizes[i].resolution_type)
			    && (VIEW_HDR_MOVIE == sonyimx134_framesizes[i].view_type)) {
    				fs->width = sonyimx134_framesizes[i].width;
    				fs->height = sonyimx134_framesizes[i].height;
    				match = true;
                    break;
			}
		}
	} else {
    	if (VIEW_FULL == view_type) {
    		for (i = 0; i < size; i++) {
    		    /*zsl only=true means is only for zsl, otherwise can used by zsl and not zsl*/
    		    if (zsl_preview == false && sonyimx134_framesizes[i].zsl_only) {
    		        continue;
    		    }
    			if ((sonyimx134_framesizes[i].width >= fs->width)
    			    && (sonyimx134_framesizes[i].height >= fs->height)
    			    && (VIEW_FULL == sonyimx134_framesizes[i].view_type)
    			    && (camera_get_resolution_type(fs->width, fs->height)
    			    <= sonyimx134_framesizes[i].resolution_type)) {
        				fs->width = sonyimx134_framesizes[i].width;
        				fs->height = sonyimx134_framesizes[i].height;
        				match = true;
        				break;
    			}
    		}
    	}
	}
	if (false == match) {
		for (i = 0; i < size; i++) {
		    /*zsl only=true means is only for zsl, otherwise can used by zsl and not zsl*/
		    if (zsl_preview == false && sonyimx134_framesizes[i].zsl_only) {
		        continue;
		    }

			if ((sonyimx134_framesizes[i].width >= fs->width)
				    && (sonyimx134_framesizes[i].height >= fs->height)
				    && (camera_get_resolution_type(fs->width, fs->height)
    				    <= sonyimx134_framesizes[i].resolution_type)
				    && (VIEW_HDR_MOVIE != sonyimx134_framesizes[i].view_type)){
					fs->width = sonyimx134_framesizes[i].width;
					fs->height = sonyimx134_framesizes[i].height;
					break;
			}
		}
	}

	if (i >= size) {
		print_error("request resolution larger than sensor's max resolution");
		return -EINVAL;
	}
	if (state == STATE_PREVIEW) {
		sonyimx134_sensor.preview_frmsize_index = i;
	} else {
		sonyimx134_sensor.capture_frmsize_index = i;
	}
	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_get_framesizes;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx134_get_framesizes(camera_state state,
				     struct v4l2_frmsize_discrete *fs)
{
	int frmsize_index;

    if(!fs) {
        return -EINVAL;
    }

	if (state == STATE_PREVIEW) {
		frmsize_index = sonyimx134_sensor.preview_frmsize_index;
	} else if (state == STATE_CAPTURE) {
		frmsize_index = sonyimx134_sensor.capture_frmsize_index;
	} else {
		return -EINVAL;
	}
	fs->width = sonyimx134_framesizes[frmsize_index].width;
	fs->height = sonyimx134_framesizes[frmsize_index].height;

	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_init_isp_reg;
 * Description : load initial seq for sensor init;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx134_init_isp_reg(void)
{
	int size = 0;
	camera_sensor *sensor = &sonyimx134_sensor;
    
    size = CAMERA_MAX_SETTING_SIZE; 
    if( HDR_MOVIE_ON == sensor->sensor_hdr_movie.hdrInfo.hdr_on) {
		sonyimx134_write_isp_seq(sensor->effect->hdr_movie_isp_settings, size);
	} else {
    	sonyimx134_write_isp_seq(sensor->effect->isp_settings, size);
	}
	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_support_hdr_movie;
 * Description : check sensor support hdr movie or not;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx134_support_hdr_movie(void)
{
    return HDR_MOVIE_SUPPORT;
}


/*
 **************************************************************************
 * FunctionName: sonyimx134_support_awb_otp;
 * Description : check sensor support awb otp or not;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx134_support_awb_otp(void)
{
	return AWB_OTP_SUPPORT;
}


/*
 **************************************************************************
 * FunctionName: sonyimx134_get_hdr_movie_switch;
 * Description : the function that get hdr movie status on or off;
 * Input       : NA;
 * Output      : the status of hdr movie status;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx134_get_hdr_movie_switch(void)
{
    return sonyimx134_sensor.sensor_hdr_movie.hdrInfo.hdr_on;
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_set_hdr_movie_switch;
 * Description : the function that set hdr movie status;
 * Input       : the status of hdr movie on or off;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void sonyimx134_set_hdr_movie_switch(int on)
{
	print_debug("Enter Function:%s on = %d",__func__,on);
	sonyimx134_sensor.sensor_hdr_movie.hdrInfo.hdr_on = (u32)on;
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_init_reg;
 * Description : download initial seq for sensor init;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx134_init_reg(void)
{
	int size = 0;

	print_debug("Enter Function:%s  , initsize=%d",
		    __func__, sizeof(sonyimx134_init_regs));

	if (0 != k3_ispio_init_csi(sonyimx134_sensor.mipi_index,
			sonyimx134_sensor.mipi_lane_count, sonyimx134_sensor.lane_clk)) {
		return -EFAULT;
	}

	size = ARRAY_SIZE(sonyimx134_init_regs);
	if (0 != sonyimx134_write_seq(sonyimx134_init_regs, size, 0x00)) {
		print_error("line %d, fail to init sonyimx134 sensor",__LINE__);
		return -EFAULT;
	}

	if(E_CAMERA_SENSOR_FLIP_TYPE_H_V == get_primary_sensor_flip_type()) {
		sonyimx134_write_reg(SONYIMX134_FLIP, 0x03, 0x00); //turn camera layout
		print_info("%s:change flip and mirror",__func__);
	}

#ifdef IMX134_OTP
	if((sonyimx134_otp_flag & SONYIMX134_OTP_LSC_READ) == SONYIMX134_OTP_LSC_READ) {
		sonyimx134_otp_enable_lsc(false);
		sonyimx134_otp_set_lsc();
		sonyimx134_otp_enable_lsc(true);
	}
#endif
	return 0;
}

static int sonyimx134_set_hflip(int flip)
{
	print_debug("enter %s flip=%d", __func__, flip);
	sonyimx134_sensor.hflip = flip;
	return 0;
}
static int sonyimx134_get_hflip(void)
{
	print_debug("enter %s", __func__);

	return sonyimx134_sensor.hflip;
}
static int sonyimx134_set_vflip(int flip)
{
	print_debug("enter %s flip=%d", __func__, flip);

	sonyimx134_sensor.vflip = flip;

	return 0;
}
static int sonyimx134_get_vflip(void)
{
	print_debug("enter %s", __func__);
	return sonyimx134_sensor.vflip;
}

static int sonyimx134_update_flip(u16 width, u16 height)
{
	u8 new_flip = ((sonyimx134_sensor.vflip << 1) | sonyimx134_sensor.hflip);
	
	print_debug("Enter %s  ", __func__);
	
	k3_ispio_update_flip(( sonyimx134_sensor.old_flip^new_flip) & 0x03, width, height, PIXEL_ORDER_CHANGED);

	sonyimx134_sensor.old_flip = new_flip;
	
	if(E_CAMERA_SENSOR_FLIP_TYPE_H_V == get_primary_sensor_flip_type()){
		sonyimx134_write_reg(SONYIMX134_FLIP, sonyimx134_sensor.vflip ? 0x00 : 0x02, ~0x02);
		sonyimx134_write_reg(SONYIMX134_FLIP, sonyimx134_sensor.hflip ? 0x00 : 0x01, ~0x01);
	}else{
		sonyimx134_write_reg(SONYIMX134_FLIP, sonyimx134_sensor.vflip ? 0x02 : 0x00, ~0x02);
		sonyimx134_write_reg(SONYIMX134_FLIP, sonyimx134_sensor.hflip ? 0x01 : 0x00, ~0x01);
	}
	
	return 0;
}
/*
 **************************************************************************
 * FunctionName: sonyimx134_framesize_switch;
 * Description : switch frame size, used by preview and capture
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx134_framesize_switch(camera_state state)
{
	u8 next_frmsize_index;

	if (state == STATE_PREVIEW)
		next_frmsize_index = sonyimx134_sensor.preview_frmsize_index;
	else
		next_frmsize_index = sonyimx134_sensor.capture_frmsize_index;

	print_debug("Enter Function:%s frm index=%d", __func__, next_frmsize_index);

	if (next_frmsize_index >= ARRAY_SIZE(sonyimx134_framesizes)){
		print_error("Unsupport sensor setting index: %d",next_frmsize_index);
		return -ETIME;
	}

	if (0 != sonyimx134_write_seq(sonyimx134_sensor.frmsize_list[next_frmsize_index].sensor_setting.setting
		,sonyimx134_sensor.frmsize_list[next_frmsize_index].sensor_setting.seq_size, 0x00)) {
		print_error("fail to init sonyimx134 sensor");
		return -ETIME;
	}

	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_stream_on;
 * Description : download preview seq for sensor preview;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx134_stream_on(camera_state state)
{
	print_debug("Enter Function:%s ", __func__);
	return sonyimx134_framesize_switch(state);
}

/*  **************************************************************************
* FunctionName: sonyimx134_check_sensor;
* Description : NA;
* Input       : NA;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
static int sonyimx134_check_sensor(void)
{
	u8 idl = 0;
	u8 idh = 0;
	u16 id = 0;
	int pin_id = 0;
    int retry = 0;
    camera_sensor *sensor = &sonyimx134_sensor;

    for(retry = 0; retry < SONYIMX134_I2C_RETRY_TIMES; retry++) {
    	sonyimx134_read_reg(0x0016, &idh);
    	sonyimx134_read_reg(0x0017, &idl);
    	id = ((idh << 8) | idl);
        print_info("sonyimx134 product id:0x%x, retrytimes:%d", id, retry);
        if(SONYIMX134_CHIP_ID == id) {
            break;
        }
        udelay(100);
    }
	if (SONYIMX134_CHIP_ID != id) {
		print_error("Invalid product id ,Could not load sensor sonyimx134\n");
		return -ENODEV;
	}
	pin_id = gpio_get_value(GPIO_18_3);
	if(pin_id < 0) {
		pin_id = 0;
		print_error("sonyimx134_check_sensor fail to get gpio value!!! set pin_id to 0 by default MODULE_LITEON !\n");
	}
	sensor_module = pin_id>0 ? MODULE_SUNNY:MODULE_LITEON;

	if(sensor_module ==  MODULE_SUNNY) {
		sonyimx134_sensor.vcm = &vcm_dw9714_Sunny;
        sensor->effect = &effect_imx134_sunny;
		snprintf(sonyimx134_sensor.info.name, sizeof(sonyimx134_sensor.info.name),"sonyimx134_sunny");
	}else {
		sonyimx134_sensor.vcm = &vcm_dw9714_Liteon;
        sensor->effect = &effect_imx134_liteon;  
		snprintf(sonyimx134_sensor.info.name, sizeof(sonyimx134_sensor.info.name),"sonyimx134_liteon");
	}

#ifdef IMX134_OTP
	sonyimx134_sensor.vcm->get_vcm_otp = sonyimx134_otp_get_vcm;
	sonyimx134_get_otp_from_sensor();
#endif
	return 0;
}

/****************************************************************************
* FunctionName: sonyimx134_check_sensor;
* Description : NA;
* Input       : NA;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
int sonyimx134_power(camera_power_state power)
{
	int ret = 0;

	print_debug("Enter Function:%s\n ", __func__);

	if(0 == get_sensor_iopower()) {
		if (power == POWER_ON) {
			k3_ispldo_power_sensor(power, M_CAMERA_ANALOG_VDD);
			//ret = camera_power_core_ldo(power);
			k3_ispldo_power_sensor(power, M_CAMERA_CORE_VDD);
			udelay(200);
			k3_ispldo_power_sensor(power, M_CAMERA_IO_VDD);
			k3_ispldo_power_sensor(power, M_CAMERA_VCM_VDD);
			udelay(1);
	        //k3_ispldo_power_sensor(power, "sec-cameralog-vcc");
			k3_ispldo_power_sensor(power, S_CAMERA_ANALOG_VDD);
			k3_ispldo_power_sensor(power, S_CAMERA_CORE_VDD);
	        k3_ispldo_power_sensor(power, S_BACKUP_CAMERA_CORE_VDD);
			//k3_ispldo_power_sensor(power, S_CAMERA_IO_VDD);

			k3_ispio_ioconfig(&sonyimx134_sensor, power);
			k3_ispgpio_power_sensor(&sonyimx134_sensor, power);
			msleep(3);
		} else {
			k3_ispio_deinit_csi(sonyimx134_sensor.mipi_index);
			k3_ispgpio_power_sensor(&sonyimx134_sensor, power);
			k3_ispio_ioconfig(&sonyimx134_sensor, power);

			k3_ispldo_power_sensor(power, S_BACKUP_CAMERA_CORE_VDD);
			k3_ispldo_power_sensor(power, S_CAMERA_CORE_VDD);
	        k3_ispldo_power_sensor(power, S_CAMERA_ANALOG_VDD);
			//k3_ispldo_power_sensor(power, S_CAMERA_IO_VDD);

	        //k3_ispldo_power_sensor(power, "sec-cameralog-vcc");
			k3_ispldo_power_sensor(power, M_CAMERA_VCM_VDD);
			k3_ispldo_power_sensor(power, M_CAMERA_CORE_VDD);
			//camera_power_core_ldo(power);
			udelay(200);
			k3_ispldo_power_sensor(power, M_CAMERA_ANALOG_VDD);
			k3_ispldo_power_sensor(power, M_CAMERA_IO_VDD);

#ifdef DUMP_SCCTRL_ISP_REG
            dump_scctrl_isp_reg();
#endif
		}
	} else {
		if (power == POWER_ON) {
			k3_ispldo_power_sensor(power, M_CAMERA_ANALOG_VDD);
			//ret = camera_power_core_ldo(power);
			k3_ispldo_power_sensor(power, M_CAMERA_CORE_VDD);
			udelay(200);
			k3_ispldo_power_sensor(power, M_CAMERA_IO_VDD);
			k3_ispldo_power_sensor(power, M_CAMERA_VCM_VDD);
			udelay(1);

			k3_ispio_ioconfig(&sonyimx134_sensor, power);
			k3_ispgpio_power_sensor(&sonyimx134_sensor, power);
			msleep(3);
		} else {
			k3_ispio_deinit_csi(sonyimx134_sensor.mipi_index);
			k3_ispgpio_power_sensor(&sonyimx134_sensor, power);
			k3_ispio_ioconfig(&sonyimx134_sensor, power);

	        //k3_ispldo_power_sensor(power, "sec-cameralog-vcc");
			k3_ispldo_power_sensor(power, M_CAMERA_VCM_VDD);
			k3_ispldo_power_sensor(power, M_CAMERA_CORE_VDD);
			//camera_power_core_ldo(power);
			udelay(200);
			k3_ispldo_power_sensor(power, M_CAMERA_ANALOG_VDD);
			k3_ispldo_power_sensor(power, M_CAMERA_IO_VDD);

#ifdef DUMP_SCCTRL_ISP_REG
            dump_scctrl_isp_reg();
#endif
		}
	}
	return ret;
}

static int sonyimx134_get_sensor_aperture(void)
{
	return SONYIMX134_APERTURE_FACTOR;
}

/*
 * Here gain is in unit 1/16 of sensor gain,
 * y36721 todo, temporarily if sensor gain=0x10, ISO is 100
 * in fact we need calibrate an ISO-ET-gain table.
 */
u32 sonyimx134_gain_to_iso(int gain)
{
	return (gain * 100) / 0x10;
}

u32 sonyimx134_iso_to_gain(int iso)
{
    return (iso * 0x10) / 100;
}

void sonyimx134_set_gain(u32 gain)
{
	if (gain == 0)
		return;
	gain = 256 - (256 * 16) / gain;
	//sonyimx134_write_reg(SONYIMX134_GAIN_REG_H, (gain >> 8) & 0xff, 0x00);
	sonyimx134_write_reg(SONYIMX134_GAIN_REG_L, gain & 0xff, 0x00);
}

void sonyimx134_set_exposure(u32 exposure)
{
	exposure >>= 4;
	sonyimx134_write_reg(SONYIMX134_EXPOSURE_REG_H, (exposure >> 8) & 0xff, 0x00);
	sonyimx134_write_reg(SONYIMX134_EXPOSURE_REG_L, exposure & 0xff, 0x00);
}

 /* *************************************************************************
  * FunctionName: 	: sonyimx134_set_dpc_funciton;
  * Description 	: set dpc register to isp and sensor according to iso
  * Input			: gain
  * Output			: NA;
  * ReturnValue 	: NA;
  * Other       	: NA;
  ***************************************************************************/
static void  sonyimx134_set_dpc_function(u32 gain)
{
	u32	iso = 0 ;
	u32	isp_dpc_size = 0;

	iso = sonyimx134_gain_to_iso(gain);
	
	if(iso > SONYIMX134_DPC_THRESHOLD_ISO)
	{
		const struct isp_reg_t  isp_dpc_reg[]={
			{0x65409,0x08},
			{0x6540a,0x08},
			{0x6540b,0x01},
			{0x6540c,0x01},
			{0x6540d,0x08},
			{0x6540e,0x08},
			{0x6540f,0x01},
			{0x65410,0x01},
			{0x65408,0x0b},
		};
		isp_dpc_size = ARRAY_SIZE(isp_dpc_reg);
		sonyimx134_write_isp_seq(isp_dpc_reg, isp_dpc_size);
		sonyimx134_write_reg(0x4100, 0xF8, 0x00);
	}else{
		const struct isp_reg_t  isp_dpc_reg[]={
			{0x65409,0x08},
			{0x6540a,0x08},
			{0x6540b,0x08},
			{0x6540c,0x08},
			{0x6540d,0x0c},
			{0x6540e,0x08},
			{0x6540f,0x08},
			{0x65410,0x08},
			{0x65408,0x0b},
		};
		isp_dpc_size = ARRAY_SIZE(isp_dpc_reg);
		sonyimx134_write_isp_seq(isp_dpc_reg, isp_dpc_size);
		sonyimx134_write_reg(0x4100, 0xE8, 0x00);
	}

}

void sonyimx134_set_exposure_gain(u32 exposure, u32 gain)
{
	u32	 tmp_digital_gain = 0;
	u8    digital_ratio = 0;
	u8    digital_h = 0;
	u8    digital_l = 0;
	u32  analog_gain = 0;
	sonyimx134_set_dpc_function(gain);//modify DPC effect of sonyimx134 sensor

	sonyimx134_write_reg(GROUP_HOLD_FUNCTION_REG, 0x01 , 0x00);//group hold

    /*adapter isp2.2 register value*/
	exposure >>= 4; //isp2.2 exposure = register_value/0x10
	
	sonyimx134_write_reg(SONYIMX134_EXPOSURE_REG_H, (exposure >> 8) & 0xff, 0x00);
	sonyimx134_write_reg(SONYIMX134_EXPOSURE_REG_L, exposure & 0xff, 0x00);

	if (gain == 0)
		goto out;

    //isp2.2 gain = register_value/0x10
    /*
    digital_h = (gain/SONYIMX134_MAX_ANALOG_GAIN)/16;
    digital_l = (digital_h'decimal)*256
    analog_gain = 256 - (256/(gain/16))
    */
	if(gain > SONYIMX134_MAX_ANALOG_GAIN*16)
	{
        /*tmp_digital_gain*256 is keep the decimal part*/
		tmp_digital_gain = (gain*256) >> 4;
		tmp_digital_gain = tmp_digital_gain >> 3;

		digital_ratio = (gain/SONYIMX134_MAX_ANALOG_GAIN) >> 4;
		digital_h = digital_ratio;
		digital_l = tmp_digital_gain - digital_ratio*256;
		analog_gain = SONYIMX134_MAX_ANALOG_GAIN*16;
	}
	else
	{
		analog_gain = gain;
		digital_h = 1;
		digital_l = 0;
	}

	analog_gain = 256 - (256 * 16) / analog_gain;
	sonyimx134_write_reg(SONYIMX134_GAIN_REG_L, analog_gain & 0xff, 0x00);
	sonyimx134_write_reg(DIG_GAIN_GR_H, digital_h & 0xff, 0x00);
	sonyimx134_write_reg(DIG_GAIN_GR_L, digital_l & 0xff, 0x00);
	sonyimx134_write_reg(DIG_GAIN_R_H, digital_h & 0xff, 0x00);
	sonyimx134_write_reg(DIG_GAIN_R_L, digital_l & 0xff, 0x00);
	sonyimx134_write_reg(DIG_GAIN_B_H, digital_h & 0xff, 0x00);
	sonyimx134_write_reg(DIG_GAIN_B_L, digital_l & 0xff, 0x00);
	sonyimx134_write_reg(DIG_GAIN_GB_H, digital_h & 0xff, 0x00);
	sonyimx134_write_reg(DIG_GAIN_GB_L, digital_l & 0xff, 0x00);
out:
	sonyimx134_write_reg(GROUP_HOLD_FUNCTION_REG, 0x00 , 0x00);//group hold
	return;
}

/****************************************************************************
* FunctionName: sonyimx134_set_awb_gain;
* Description : NA;
* Input       : R,GR,GB,B gain from isp;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/

void sonyimx134_set_awb_gain(u16 b_gain, u16 gb_gain, u16 gr_gain, u16 r_gain)
{

	sonyimx134_write_reg(GROUP_HOLD_FUNCTION_REG, 0x01 , 0x00);//group hold
	sonyimx134_write_reg(SONYIMX134_ABS_GAIN_B_H, (b_gain >> 8) & 0xff, 0x00);
	sonyimx134_write_reg(SONYIMX134_ABS_GAIN_B_L, b_gain & 0xff, 0x00);
	sonyimx134_write_reg(SONYIMX134_ABS_GAIN_GB_H, (gb_gain >> 8) & 0xff, 0x00);
	sonyimx134_write_reg(SONYIMX134_ABS_GAIN_GB_L, gb_gain & 0xff, 0x00);
	sonyimx134_write_reg(SONYIMX134_ABS_GAIN_GR_H, (gr_gain >> 8) & 0xff, 0x00);
	sonyimx134_write_reg(SONYIMX134_ABS_GAIN_GR_L, gr_gain & 0xff, 0x00);
	sonyimx134_write_reg(SONYIMX134_ABS_GAIN_R_H, (r_gain >> 8) & 0xff, 0x00);
	sonyimx134_write_reg(SONYIMX134_ABS_GAIN_R_L, r_gain & 0xff, 0x00);

	sonyimx134_write_reg(GROUP_HOLD_FUNCTION_REG, 0x00 , 0x00);//group open
}

void sonyimx134_set_vts(u16 vts)
{
	print_debug("Enter %s  ", __func__);
	sonyimx134_write_reg(SONYIMX134_VTS_REG_H, (vts >> 8) & 0xff, 0x00);
	sonyimx134_write_reg(SONYIMX134_VTS_REG_L, vts & 0xff, 0x00);
}

u32 sonyimx134_get_vts_reg_addr(void)
{
	return SONYIMX134_VTS_REG_H;
}

extern u32 sensor_override_params[OVERRIDE_TYPE_MAX];
static u32 sonyimx134_get_override_param(camera_override_type_t type)
{
	u32 ret_val = sensor_override_params[type];

	switch (type) {
	case OVERRIDE_ISO_HIGH:
		ret_val = SONYIMX134_MAX_ISO;
		break;

	case OVERRIDE_ISO_LOW:
		ret_val = SONYIMX134_MIN_ISO;
		break;

	/* increase frame rate gain threshold */
	case OVERRIDE_AUTOFPS_GAIN_LOW2MID:
		ret_val = SONYIMX134_AUTOFPS_GAIN_LOW2MID;
		break;
	case OVERRIDE_AUTOFPS_GAIN_MID2HIGH:
		ret_val = SONYIMX134_AUTOFPS_GAIN_MID2HIGH;
		break;

	/* reduce frame rate gain threshold */
	case OVERRIDE_AUTOFPS_GAIN_MID2LOW:
		ret_val = SONYIMX134_AUTOFPS_GAIN_MID2LOW;
		break;
	case OVERRIDE_AUTOFPS_GAIN_HIGH2MID:
		ret_val = SONYIMX134_AUTOFPS_GAIN_HIGH2MID;
		break;

	case OVERRIDE_FPS_MAX:
		ret_val = SONYIMX134_MAX_FRAMERATE;
		break;

    case OVERRIDE_FPS_MID:
        ret_val = SONYIMX134_MID_FRAMERATE;
        break;

	case OVERRIDE_FPS_MIN:
		ret_val = SONYIMX134_MIN_FRAMERATE;
		break;

	case OVERRIDE_CAP_FPS_MIN:
		ret_val = SONYIMX134_MIN_CAP_FRAMERATE;
		break;

	case OVERRIDE_FLASH_TRIGGER_GAIN:
		ret_val = SONYIMX134_FLASH_TRIGGER_GAIN;
		break;

	case OVERRIDE_SHARPNESS_PREVIEW:
		ret_val = SONYIMX134_SHARPNESS_PREVIEW;
		break;

	case OVERRIDE_SHARPNESS_CAPTURE:
		ret_val = SONYIMX134_SHARPNESS_CAPTURE;
		break;

	default:
		print_error("%s:not override or invalid type %d, use default",__func__, type);
		break;
	}

	return ret_val;
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_get_gain;
 * Description :get gain from sensor
 * Input       	 NA;
 * Output      	 gain value from sensor
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
u32 sonyimx134_get_gain(void)
{
	u8 gain_l = 0;
	u32 gain = 0;
	sonyimx134_read_reg(SONYIMX134_GAIN_REG_L, &gain_l);
	gain = gain_l;
	if(_IS_DEBUG_AE) {
		print_info("%s-0x0205=%x",__func__,gain_l);
	}
	return gain;
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_get_exposure;
 * Description :get exposure from sensor
 * Input       	 NA;
 * Output      	 exposure value from sensor
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
u32  sonyimx134_get_exposure(void)
{
	u32 expo = 0;
	u8 expo_h = 0;
	u8 expo_l = 0;
	
	sonyimx134_read_reg(SONYIMX134_EXPOSURE_REG_H, &expo_h);
	sonyimx134_read_reg(SONYIMX134_EXPOSURE_REG_L, &expo_l);

	if(_IS_DEBUG_AE) {
		print_info("sonyimx134_get_exposure-0x0202=%x,0x0203=%x",expo_h,expo_l);
	}
	expo = expo_h &0xff;
	expo = expo << 8 | expo_l;
	return expo;
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_set_hdr_exposure_gain;
 * Description 	   : set exposure and gain to sensor.
 * Input		   : the value of exposure and gain ,that include long exposure short exposure and long gain db ,short gain db
 		          :  gain is db but not times
 * Output      	   : NA
 * ReturnValue   : NA;
 * Other       	   : NA;
 **************************************************************************
*/

void sonyimx134_set_hdr_exposure_gain(u32 expo_long,u16 gain_global,u32 expo_short,u16 short_gain)
{
    u16 gain_switch_long = 0;
	u16 gain_switch_short = 0;

	gain_switch_long = gain_global;
	gain_switch_short = short_gain;
	if(_IS_DEBUG_AE) {
       print_info("__debug_esen: Enter %s  gain_switch_long = %d ", __func__,gain_switch_long);
       print_info("__debug_esen: Enter %s  expo_long = %d ", __func__,expo_long);
	   print_info("__debug_esen: Enter %s  gain_switch_short = %d ", __func__,gain_switch_short);
       print_info("__debug_esen: Enter %s  expo_short = %d", __func__,expo_short);
	}

	sonyimx134_write_reg(GROUP_HOLD_FUNCTION_REG, 0x01 , 0x00);//group hold
	sonyimx134_write_reg(SONYIMX134_EXPOSURE_REG_H, (expo_long >> 8) & 0xff, 0x00);
	sonyimx134_write_reg(SONYIMX134_EXPOSURE_REG_L, expo_long & 0xff, 0x00);
	sonyimx134_write_reg(WD_COARSE_INTEG_TIME_DS_H, (expo_short >> 8) & 0xff, 0x00);
	sonyimx134_write_reg(WD_COARSE_INTEG_TIME_DS_L, expo_short & 0xff, 0x00);
	sonyimx134_write_reg(SONYIMX134_GAIN_REG_L, gain_switch_long, 0x00);
	sonyimx134_write_reg(WD_ANA_GAIN_DS, gain_switch_short, 0x00);
	sonyimx134_write_reg(GROUP_HOLD_FUNCTION_REG, 0x00 , 0x00);//group release

}

/*
 **************************************************************************
 * FunctionName: sonyimx134_set_ATR;
 * Description : turn on or off atr curve;
 * Input       : 1 turn on atr ,0 turn off atr;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/

void sonyimx134_set_ATR(int on)
{
   if(on == 0) {//ATR off
    	sonyimx134_write_reg(GROUP_HOLD_FUNCTION_REG, 0x01 , 0x00);//group hold
    	sonyimx134_write_reg(ATR_OFF_SETTING_1,0x00, 0x00);
    	sonyimx134_write_reg(ATR_OFF_SETTING_2,0x00, 0x00);
    	sonyimx134_write_reg(TRIGGER_SETTING,0x01, 0x00);
    	sonyimx134_write_reg(0x4344,0x01, 0x00);//ARNR
    	sonyimx134_write_reg(0x4364,0x0B, 0x00);
    	sonyimx134_write_reg(0x436C,0x00, 0x00);
    	sonyimx134_write_reg(0x436D,0x00, 0x00);
    	sonyimx134_write_reg(0x436E,0x00, 0x00);
    	sonyimx134_write_reg(0x436F,0x00, 0x00);
    	sonyimx134_write_reg(0x4369,0x00, 0x00);
    	sonyimx134_write_reg(0x437B,0x00, 0x00);
    	sonyimx134_write_reg(GROUP_HOLD_FUNCTION_REG, 0x00 , 0x00);//group release
    } else {//ATR on
		sonyimx134_write_reg(GROUP_HOLD_FUNCTION_REG, 0x01 , 0x00);//group hold
		sonyimx134_write_reg(0x437B,0x01, 0x00);
		sonyimx134_write_reg(0x4369,0x0f, 0x00);
		sonyimx134_write_reg(0x436F,0x06, 0x00);
		sonyimx134_write_reg(0x436E,0x00, 0x00);
		sonyimx134_write_reg(0x436D,0x00, 0x00);
		sonyimx134_write_reg(0x436C,0x00, 0x00);
		sonyimx134_write_reg(0x4364,0x0B, 0x00);

		sonyimx134_write_reg(0x4344,0x01, 0x00);

		sonyimx134_write_reg(TRIGGER_SETTING,0x00, 0x00);
		sonyimx134_write_reg(GROUP_HOLD_FUNCTION_REG, 0x00 , 0x00);//group release
	}
	sonyimx134_sensor.sensor_hdr_movie.hdrInfo.atr_on = on;
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_over_exposure_adjust;
 * Description :
 * Input       	:
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/

void sonyimx134_over_exposure_adjust(int on,stats_hdr_frm * frm_stats)
{
	print_debug("enter %s  on = %d",__func__,on);
	if(on == 1)	{
		sonyimx134_write_reg(GROUP_HOLD_FUNCTION_REG, 0x01 , 0x00);//group hold
		sonyimx134_write_reg(0x4470,frm_stats->frm_ave_h&0xff,0x00);//frm_ave
		sonyimx134_write_reg(0x4471,frm_stats->frm_ave_l&0xff,0x00);//frm_ave2
		sonyimx134_write_reg(0x4472,frm_stats->frm_min_h&0xff,0x00);//frm_min1
		sonyimx134_write_reg(0x4473,frm_stats->frm_min_l&0xff,0x00);//frm_min2
		sonyimx134_write_reg(0x4474,frm_stats->frm_max_h&0xff,0x00);//frm_max1
		sonyimx134_write_reg(0x4475,frm_stats->frm_max_l&0xff,0x00);//frm_max2
		sonyimx134_write_reg(0x4476,0x01,0x00);
		sonyimx134_write_reg(GROUP_HOLD_FUNCTION_REG, 0x00 , 0x00);//group release
		if(_IS_DEBUG_AE) {
			print_info(" frm_ave_h = %d,frm_ave_l =%d,frm_min_h=%d,frm_min_l=%d,frm_max_h=%d,frm_max_l=%d,",
				frm_stats->frm_ave_h,frm_stats->frm_ave_l,frm_stats->frm_min_h,frm_stats->frm_min_l,
				frm_stats->frm_max_h,frm_stats->frm_max_l);
		}
	} else {
		sonyimx134_write_reg(0x4476,0x00,0x00);
	}
	sonyimx134_sensor.sensor_hdr_movie.hdrInfo.atr_over_expo_on = on;
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_set_ATR;
 * Description : turn on or off atr curve;
 * Input       : 1 turn on atr ,0 turn off atr;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/

void sonyimx134_set_lmt_sat(u32 lmt_value, u32 sat_value)
{
	if(_IS_DEBUG_AE) {
        print_info(" Enter %s  lmt_value = %x ", __func__,lmt_value);
        print_info(" Enter %s  sat_value = %x ", __func__,sat_value);
    }

	sonyimx134_write_reg(GROUP_HOLD_FUNCTION_REG, 0x01 , 0x00);//group hold
	sonyimx134_write_reg(WB_LMT_REG_H, (lmt_value >> 8) & 0xff, 0x00);
	sonyimx134_write_reg(WB_LMT_REG_L, lmt_value & 0xff, 0x00);
	sonyimx134_write_reg(AE_SAT_REG_H, (sat_value >> 8) & 0xff, 0x00);
	sonyimx134_write_reg(AE_SAT_REG_L, sat_value & 0xff, 0x00);
	sonyimx134_write_reg(GROUP_HOLD_FUNCTION_REG, 0x00 , 0x00);//group release
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_get_hdr_movie_ae_param;
 * Description : The ae arith in HAL get init parm from sensor by this interface.
 * Input       : the space of hdr_ae_constant_param for keeping ratio,max gain,min gain and vts;
 * Output      : the value of ratio,max gain,min gain and vts;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/

int sonyimx134_get_hdr_movie_ae_param(hdr_ae_constant_param * hdr_ae_param)
{
	if(NULL != hdr_ae_param) {
		hdr_ae_param->hdr_ae_ratio 		= sonyimx134_sensor.sensor_hdr_movie.ae_hdr_ratio;
		hdr_ae_param->sensor_max_gain	= sonyimx134_sensor.sensor_hdr_movie.sensor_max_gain;
		hdr_ae_param->sensor_min_gain	= sonyimx134_sensor.sensor_hdr_movie.sensor_min_gain;
		hdr_ae_param->max_analog_gain	= sonyimx134_sensor.sensor_hdr_movie.sensor_max_analog_gain;
	}
	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_set_digital_gain;
 * Description : set digital gain to sensor's digital registers
 * Input       : the struct of digital_gain_st saving digital value
 * Output      : NA
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/

void sonyimx134_set_digital_gain(digital_gain_st * digital_gain)
{
	u8 digital_gain_h = digital_gain->digital_gain_h;
	u8 digital_gain_l = digital_gain->digital_gain_l;
	if(_IS_DEBUG_AE) {
       print_info("Enter %s  digital_gain_h = %d ,digital_gain_l=%d", __func__,digital_gain_h,digital_gain_l);
	}
	sonyimx134_write_reg(GROUP_HOLD_FUNCTION_REG, 0x01 , 0x00);//group hold
	sonyimx134_write_reg(DIG_GAIN_GR_H, digital_gain_h, 0x00);
	sonyimx134_write_reg(DIG_GAIN_GR_L, digital_gain_l, 0x00);
	sonyimx134_write_reg(DIG_GAIN_R_H, digital_gain_h, 0x00);
	sonyimx134_write_reg(DIG_GAIN_R_L, digital_gain_l, 0x00);
	sonyimx134_write_reg(DIG_GAIN_B_H,digital_gain_h, 0x00);
	sonyimx134_write_reg(DIG_GAIN_B_L, digital_gain_l, 0x00);
	sonyimx134_write_reg(DIG_GAIN_GB_H, digital_gain_h, 0x00);
	sonyimx134_write_reg(DIG_GAIN_GB_L,digital_gain_l, 0x00);
	sonyimx134_write_reg(GROUP_HOLD_FUNCTION_REG, 0x00 , 0x00);//group release
}


 /*
  *************************************************************************
  * FunctionName: 	:sonyimx134_get_digital_gain;
  * Description 	: get digital gain from sensor's digital registers
  * Input			: the struct of digital_gain_st for storging digital value
  * Output			: NA;
  * ReturnValue 	: NA;
  * Other       		:
  **************************************************************************
  */

void sonyimx134_get_digital_gain(digital_gain_st * digital_gain)
{
	u8 digital_gain_h = 0;
	u8 digital_gain_l  = 0;
	sonyimx134_read_reg(DIG_GAIN_GR_H, &digital_gain_h);
	sonyimx134_read_reg(DIG_GAIN_GR_L, &digital_gain_l);
	sonyimx134_read_reg(DIG_GAIN_R_H, &digital_gain_h);
	sonyimx134_read_reg(DIG_GAIN_R_L, &digital_gain_l);
	sonyimx134_read_reg(DIG_GAIN_B_H,&digital_gain_h);
	sonyimx134_read_reg(DIG_GAIN_B_L, &digital_gain_l);
	sonyimx134_read_reg(DIG_GAIN_GB_H, &digital_gain_h);
	sonyimx134_read_reg(DIG_GAIN_GB_L,&digital_gain_l);

	digital_gain->digital_gain_h = digital_gain_h;
	digital_gain->digital_gain_l = digital_gain_l;
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_reset;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx134_reset(camera_power_state power_state)
{
	print_debug("%s  \n", __func__);
	sonyimx134_sensor.old_flip = 0;
	sonyimx134_sensor.sensor_hdr_movie.hdrInfo.hdr_on = HDR_MOVIE_OFF;

	if (POWER_ON == power_state) {
		k3_isp_io_enable_mclk(MCLK_ENABLE, sonyimx134_sensor.sensor_index);
		udelay(100);
		k3_ispgpio_reset_sensor(sonyimx134_sensor.sensor_index, power_state,
			      sonyimx134_sensor.power_conf.reset_valid);
		udelay(500);
	} else {
		k3_ispgpio_reset_sensor(sonyimx134_sensor.sensor_index, power_state,
			      sonyimx134_sensor.power_conf.reset_valid);
		udelay(10);
		k3_isp_io_enable_mclk(MCLK_DISABLE, sonyimx134_sensor.sensor_index);
	}

	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_init;
 * Description : sonyimx134 init function;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : Error code indicating success or failure;
 * Other       : NA;
 **************************************************************************
*/
static int sonyimx134_init(void)
{
	static bool sonyimx134_check = false;
    print_debug("%s  ", __func__);

	if (false == sonyimx134_check) {
        if (check_suspensory_camera("SONYIMX134") != 1) {
            return -ENODEV;     
        }       
        sonyimx134_check = true;
    }

	if (!camera_timing_is_match(0)) {
		print_error("%s: sensor timing don't match.\n", __func__);
		return -ENODEV;
	}

	if (sonyimx134_sensor.owner && !try_module_get(sonyimx134_sensor.owner)) {
		print_error("%s: try_module_get fail", __func__);
		return -ENOENT;
	}

    /* 1. M_CAMERA_IO_VDD and S_CAMERA_IO_VDD have the same power.
       2. Only power on one sensor cause leakage, so pri-sensor and sec-sensor
          power on at the same time. Then pull down sec-sensor's PWDN/RESET GPIO for low power.
    */
	if(E_CAMERA_SENSOR_FLIP_TYPE_H_V == get_primary_sensor_flip_type())
		sonyimx134_sensor.sensor_rgb_type = SENSOR_BGGR;
	else
		sonyimx134_sensor.sensor_rgb_type = SENSOR_RGGB;
	sonyimx134_sensor.old_flip = 0;
	if(0 == get_sensor_iopower()) {
		k3_ispio_power_init(M_CAMERA_ANALOG_VDD, LDO_VOLTAGE_28V, LDO_VOLTAGE_28V);	/*analog 2.85V - pri camera*/
		k3_ispio_power_init(M_CAMERA_IO_VDD, LDO_VOLTAGE_18V, LDO_VOLTAGE_18V);		/*IO 1.8V - pri camera*/
		k3_ispio_power_init(M_CAMERA_VCM_VDD, LDO_VOLTAGE_28V, LDO_VOLTAGE_28V);	/*AF 2.85V - pri camera*/
		k3_ispio_power_init(M_CAMERA_CORE_VDD, LDO_VOLTAGE_10V, LDO_VOLTAGE_10V);	/*core 1.05V - pri camera*/
		k3_ispio_power_init(S_CAMERA_ANALOG_VDD, LDO_VOLTAGE_28V, LDO_VOLTAGE_28V);	/*analog 2.85V - sec camera*/
		k3_ispio_power_init(S_CAMERA_CORE_VDD, LDO_VOLTAGE_18V, LDO_VOLTAGE_18V);	/*core 1.8V - sec camera*/
		k3_ispio_power_init(S_BACKUP_CAMERA_CORE_VDD, LDO_VOLTAGE_18V, LDO_VOLTAGE_18V);
		//k3_ispio_power_init(S_CAMERA_IO_VDD, LDO_VOLTAGE_18V, LDO_VOLTAGE_18V);

	} else {
		k3_ispio_power_init(M_CAMERA_ANALOG_VDD, LDO_VOLTAGE_28V, LDO_VOLTAGE_28V);	/*analog 2.85V - pri camera*/
		k3_ispio_power_init(M_CAMERA_IO_VDD, LDO_VOLTAGE_18V, LDO_VOLTAGE_18V);		/*IO 1.8V - pri camera*/
		k3_ispio_power_init(M_CAMERA_VCM_VDD, LDO_VOLTAGE_28V, LDO_VOLTAGE_28V);	/*AF 2.85V - pri camera*/
		k3_ispio_power_init(M_CAMERA_CORE_VDD, LDO_VOLTAGE_10V, LDO_VOLTAGE_10V);	/*core 1.05V - pri camera*/
	}

	return 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_exit;
 * Description : sonyimx134 exit function;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void sonyimx134_exit(void)
{
	print_debug("enter %s", __func__);

	k3_ispio_power_deinit();

	if (sonyimx134_sensor.owner) {
		module_put(sonyimx134_sensor.owner);
	}
	print_debug("exit %s", __func__);
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_shut_down;
 * Description : sonyimx134 shut down function;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void sonyimx134_shut_down(void)
{
	print_debug("enter %s", __func__);
	k3_ispgpio_power_sensor(&sonyimx134_sensor, POWER_OFF);
}

void sonyimx134_get_flash_awb(flash_platform_t type, awb_gain_t *flash_awb)
{
	*flash_awb = flash_platform_awb[0];
	print_info("sonyimx134_get_flash_awb: type 0x%x,", type);
}

#ifdef IMX134_OTP
int sonyimx134_read_otp(u8 i2c_addr,u16 reg,u8 *buf,u16 count)
{
	u16 i;
	int ret;
	u16 val = 0;
	for(i=0;i<count;i++) {
		ret =ispv1_read_sensor_byte_addr8(sonyimx134_sensor.i2c_config.index,i2c_addr,reg+i,&val,sonyimx134_sensor.i2c_config.val_bits);
		if(ret !=0)
			print_error("sonyimx134_read_otp  %d\n",ret);
		buf[i] = (val&0xff);
		OTPSUMVAL +=buf[i];
	}
	return 0;
}

static bool sonyimx134_otp_read_id(void)
{
	u8 buf[5] = {0};
	u8 vendor_id =0;
	u8 module_type = MODULE_UNSUPPORT;

	print_debug("enter %s", __func__);

	if((sonyimx134_otp_flag & SONYIMX134_OTP_ID_READ) ==SONYIMX134_OTP_ID_READ) {//we think OTP data is not correct at all
		print_debug("%s OTP ID data is read allread!!!\n",__func__);
		return true;
	}

	sonyimx134_read_otp(OTP_ID_AWB,OTP_ID_REG,buf,5);

	print_debug("module info year 20%02d month %d day %d, SNO. 0x%x  vendor id&version 0x%x\n",
		buf[0],buf[1],buf[2],buf[3],buf[4]);

	vendor_id = buf[4]>>4;
	if(vendor_id ==0x01) { //sunny
		module_type = MODULE_SUNNY;
	}

	if(vendor_id == 0x03) { //Liteon
		module_type = MODULE_LITEON;
	}

	if(sensor_module == module_type) {
		sonyimx134_otp_flag |= SONYIMX134_OTP_ID_READ;
	} else {
		print_error("%s OTP data is worng!!!\n",__func__);
		return false;
	}
	return true;
}


static bool sonyimx134_otp_read_awb(void)
{
	u8 buf[6] = {0};

	print_debug("enter %s", __func__);
	if((sonyimx134_otp_flag & SONYIMX134_OTP_ID_READ) ==0) { //we think OTP data is not correct at all
		print_error("%s OTP data is worng!!!\n",__func__);
		return false;
	}

	if((sonyimx134_otp_flag & SONYIMX134_OTP_AWB_READ) ==SONYIMX134_OTP_AWB_READ) {//we think OTP data is not correct at all
		print_debug("%s OTP AWB data is read allread!!!\n",__func__);
		return true;
	}

	sonyimx134_read_otp(OTP_ID_AWB,OTP_AWB_REG,buf,6);

	sonyimx134_sensor.sensor_hw_3a.awb_otp_value.r_g_high = buf[0];
	sonyimx134_sensor.sensor_hw_3a.awb_otp_value.r_g_low = buf[1];
	sonyimx134_sensor.sensor_hw_3a.awb_otp_value.b_g_high = buf[2];
	sonyimx134_sensor.sensor_hw_3a.awb_otp_value.b_g_low = buf[3];
	sonyimx134_sensor.sensor_hw_3a.awb_otp_value.gb_gr_high = buf[4];
	sonyimx134_sensor.sensor_hw_3a.awb_otp_value.gb_gr_low = buf[5];

	sonyimx134_otp_flag |= SONYIMX134_OTP_AWB_READ;
	return true;
}

static bool sonyimx134_otp_read_lsc(void)
{
	print_debug("enter %s", __func__);
	if((sonyimx134_otp_flag & SONYIMX134_OTP_ID_READ) ==0) {//we think OTP data is not correct at all
		print_error("%s OTP data is worng!!!\n",__func__);
		return false;
	}

	if((sonyimx134_otp_flag & SONYIMX134_OTP_LSC_READ) ==SONYIMX134_OTP_LSC_READ) {//we think OTP data is not correct at all
		print_debug("%s OTP LSC data is read allread!!!\n",__func__);
		return true;
	}

	memset(sonyimx134_otp_lsc_param, 0, SONYIMX134_OTP_LSC_SIZE);
	//LSC 0xa4:0b--0xff  / 0xa6:00--0x22  total = 280
	sonyimx134_read_otp(OTP_LSC_1,OTP_LSC_1_REG,sonyimx134_otp_lsc_param,0xff-0x0b+1);
	sonyimx134_read_otp(OTP_LSC_2,OTP_LSC_2_REG,&sonyimx134_otp_lsc_param[0xff-0x0b+1],0x22+1);

	print_debug("%s LCS[0]= %x,LSC[247] = %x  LSC[248]=%x,LSC[279]=%d\n",__func__,
		sonyimx134_otp_lsc_param[0],sonyimx134_otp_lsc_param[247],sonyimx134_otp_lsc_param[248],sonyimx134_otp_lsc_param[279]);
	sonyimx134_otp_flag |= SONYIMX134_OTP_LSC_READ;
	return true;
}


/****************************************************************************
* FunctionName: sonyimx134_otp_set_lsc;
* Description : Set lens shading parameters to sensor registers.; cost time is 0.0341s on sunny module
* Input       : NA;
* Output      : NA;
* ReturnValue : bool;
* Other       : NA;
***************************************************************************/
static bool sonyimx134_otp_set_lsc(void)
{
	u8 *pval = NULL;
	int i = 0;

	print_debug("enter %s\n", __func__);

	/* Lens shading parameters are burned OK. */
	if((sonyimx134_otp_flag & SONYIMX134_OTP_LSC_READ) ==0) {
		print_error("%s OTP data is worng!!!\n",__func__);
		return false;
	}
	pval = sonyimx134_otp_lsc_param;
	/* Write lens shading parameters to sensor registers. */
	for (i=0; i<SONYIMX134_OTP_LSC_SIZE; i++) {
		sonyimx134_write_reg(SONYIMX134_SENSOR_LSC_RAM+i, *(pval+i), 0x00);
		print_debug("LSC[%d] = %d  \n",i,*(pval+i));
	}
	print_debug("%s, set OTP LSC to sensor OK.\n", __func__);

	return true;
}


/****************************************************************************
* FunctionName: sonyimx134_otp_enable_lsc;
* Description : Enable LSC correct.;
* Input       : bool;
* Output      : NA;
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
static bool sonyimx134_otp_enable_lsc(bool enable)
{
	u8 lscMode = 0x00;
	u8 selToggle = 0x00;
	u8 lscEnable = 0x00;

	print_debug("enter %s", __func__);
	if((sonyimx134_otp_flag & SONYIMX134_OTP_LSC_READ) ==0)	{
		print_error("%s OTP data is worng!!!\n",__func__);
		return false;
	}
	/* Open OTP lsc mode */
	if (enable) {
		lscMode = 0x01;
		selToggle = 0x01;
		lscEnable = 0x1F;
		print_debug("%s, OTP LSC enabled!", __func__);
	}
	sonyimx134_write_reg(SONYIMX134_SENSOR_LSC_EN, lscEnable, 0x00);
	sonyimx134_write_reg(SONYIMX134_SENSOR_LSC_MODE, lscMode, 0x00);
	sonyimx134_write_reg(SONYIMX134_SENSOR_RAM_SEL, selToggle, 0x00);

	return true;
}

/****************************************************************************
* FunctionName: sonyimx134_otp_read_vcm;
* Description : Get AF motor parameters from OTP.;
* Input       : NA;
* Output      : NA;
* ReturnValue : bool;
* Other       : NA;
***************************************************************************/
static bool sonyimx134_otp_read_vcm(void)
{
	u8 buf[4] = {0};
	u16 start_code,end_code;

	print_debug("enter %s", __func__);
	if((sonyimx134_otp_flag & SONYIMX134_OTP_ID_READ) ==0) { //we think OTP data is not correct at all
		print_error("%s OTP data is worng!!!\n",__func__);
		return false;
	}

	if((sonyimx134_otp_flag & SONYIMX134_OTP_VCM_READ) ==SONYIMX134_OTP_VCM_READ) { //we think OTP data is not correct at all
		print_debug("%s OTP VCM data is read allread!!!\n",__func__);
		return true;
	}

	sonyimx134_read_otp(OTP_VCM,OTP_VCM_REG,buf,4);
	print_debug("raw buf_vcm[0]= %x, buf_vcm[1] = %x buf_vcm[2] = %x buf_vcm[3] = %x\n",buf[0],buf[1],buf[2],buf[3]);

	sonyimx134_otp_flag |= SONYIMX134_OTP_VCM_READ;
	start_code = buf[0];
	start_code <<= 8;
	start_code +=buf[1];
	end_code = buf[2];
	end_code <<= 8;
	end_code += buf[3];

	if((start_code != end_code) &&(end_code>start_code)) {
		/* VCM param is read  */
		sonyimx134_otp_flag |= SONYIMX134_OTP_VCM_READ;
		sonyimx134_vcm_start = start_code;
		sonyimx134_vcm_end = end_code;
		print_debug("sonyimx134_vcm_start= %x, sonyimx134_vcm_end = %x \n",sonyimx134_vcm_start,sonyimx134_vcm_end);
		return true;
	} else {
		print_error("%s VCM OTP data is worng!!!\n",__func__);
		return false;
	}
}
/****************************************************************************
* FunctionName: sonyimx134_otp_get_vcm;
* Description : Get vcm start and stop parameters read from OTP.;
* Input       : NA;
* Output      : vcm_start vcm_end
* ReturnValue : NA;
* Other       : NA;
***************************************************************************/
static void sonyimx134_otp_get_vcm(u16 *vcm_start, u16 *vcm_end)
{
	if (0 == sonyimx134_vcm_start) {
		*vcm_start = sonyimx134_sensor.vcm->infiniteDistance;
	} else {	
        //*vcm_start = sonyimx134_vcm_start;
        if (sonyimx134_vcm_start > sonyimx134_sensor.vcm->startCurrentOffset)
            *vcm_start = sonyimx134_vcm_start - sonyimx134_sensor.vcm->startCurrentOffset;
        else
            *vcm_start = 0;
	}

	if (0 == sonyimx134_vcm_end) {
		*vcm_end = sonyimx134_sensor.vcm->normalDistanceEnd;
	} else {
		*vcm_end = sonyimx134_vcm_end;
	}

	print_debug("%s, start: %#x, end: %#x", __func__, *vcm_start, *vcm_end);
}
//this function time cost is about 0.0475s on sunny module
static void sonyimx134_get_otp_from_sensor(void)
{
	u8 sum;
	if((sonyimx134_otp_flag & SONYIMX134_OTP_CHECKSUM_ERR) ==SONYIMX134_OTP_CHECKSUM_ERR) {
		print_error("OTP data is worng!\n");
		return;
	}
	sonyimx134_otp_read_id();
	sonyimx134_otp_read_awb();
	sonyimx134_otp_read_vcm();
	sonyimx134_otp_read_lsc();//cost 0.042996s on sunny module

	if((sonyimx134_otp_flag & SONYIMX134_OTP_CHECKSUM_READ) ==SONYIMX134_OTP_CHECKSUM_READ)	{
		print_debug("%s OTP checksum data is read allread!!!\n",__func__);
		return ;
	}

	sonyimx134_read_otp(OTP_CHECKSUM,OTP_CHECKSUM_REG,&OTPCHECKSUMVAL,1);
	sonyimx134_read_otp(OTP_CHECKSUM,OTP_CHECKSUM_FLAG_REG,&OTPCHECKSUMFLAG,1);

	sum = (OTPSUMVAL - OTPCHECKSUMVAL-OTPCHECKSUMFLAG)%0xff;

	print_debug("%s, OTPSUMVAL: %d, OTPCHECKSUMVAL: %d ,sum:%d, OTPCHECKSUMFLAG =%x \n", __func__, OTPSUMVAL, OTPCHECKSUMVAL,sum,OTPCHECKSUMFLAG);

	/*
	*the value for OTPCHECKSUMFLAG shoule be one of them
	*1.0xa5 means checksum enabel
	*2.0xff  means checksum disable
	*3. other means write or read OTP have error or otp data is writend by mistake
	*/
	if(((OTPCHECKSUMVAL==sum)&&(OTPCHECKSUMFLAG ==OTPCHECKSUM_FLAG))
		||(OTPCHECKSUMFLAG ==0xff))	{
		sonyimx134_otp_flag |= SONYIMX134_OTP_CHECKSUM_READ;
	} else {
		sonyimx134_otp_flag = SONYIMX134_OTP_CHECKSUM_ERR;
		sonyimx134_vcm_start = 0;
		sonyimx134_vcm_end = 0;
		print_error("OTP checksum is worng!\n");
	}
}
#endif

/*
 **************************************************************************
 * FunctionName: sonyimx134_set_hdr_default;
 * Description : init hdr;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void sonyimx134_set_hdr_default(void)
{
    sonyimx134_sensor.sensor_hdr_movie.ae_hdr_ratio = 8;
    
    sonyimx134_sensor.sensor_hdr_movie.set_ATR_switch = sonyimx134_set_ATR;
    sonyimx134_sensor.sensor_hdr_movie.set_hdr_exposure_gain = sonyimx134_set_hdr_exposure_gain;
    sonyimx134_sensor.sensor_hdr_movie.support_hdr_movie = sonyimx134_support_hdr_movie;
    sonyimx134_sensor.sensor_hdr_movie.get_hdr_movie_switch = sonyimx134_get_hdr_movie_switch;
    sonyimx134_sensor.sensor_hdr_movie.set_hdr_movie_switch = sonyimx134_set_hdr_movie_switch;
    sonyimx134_sensor.sensor_hdr_movie.over_exposure_adjust = sonyimx134_over_exposure_adjust;
    sonyimx134_sensor.sensor_hdr_movie.set_lmt_sat = sonyimx134_set_lmt_sat;
    sonyimx134_sensor.sensor_hdr_movie.get_hdr_movie_ae_param = sonyimx134_get_hdr_movie_ae_param;
    sonyimx134_sensor.sensor_hdr_movie.set_digital_gain = sonyimx134_set_digital_gain;
    sonyimx134_sensor.sensor_hdr_movie.get_digital_gain = sonyimx134_get_digital_gain;
    sonyimx134_sensor.sensor_hdr_movie.set_awb_gain = sonyimx134_set_awb_gain;
    
    sonyimx134_sensor.sensor_hdr_movie.hdrInfo.atr_on = ATR_ON;
    sonyimx134_sensor.sensor_hdr_movie.hdrInfo.hdr_on = HDR_MOVIE_OFF;
    sonyimx134_sensor.sensor_hdr_movie.stats_data.stats_h =1024;
	sonyimx134_sensor.sensor_hdr_movie.stats_data.stats_v =2;
    sonyimx134_sensor.sensor_hdr_movie.sensor_max_gain =32;
    sonyimx134_sensor.sensor_hdr_movie.sensor_max_analog_gain = 8;
    sonyimx134_sensor.sensor_hdr_movie.sensor_min_gain =1;
    sonyimx134_sensor.sensor_hdr_movie.ae_arithmatic_switch_gain = 0;
    sonyimx134_sensor.sensor_hdr_movie.stats_diff_threshold = 0;
    sonyimx134_sensor.sensor_hdr_movie.stats_max_threshold = 0;
    sonyimx134_sensor.sensor_hdr_movie.ae_target_low = 0;
    sonyimx134_sensor.sensor_hdr_movie.ae_target_high = 0;

    sonyimx134_sensor.sensor_hdr_movie.gain_switch = 0;
    sonyimx134_sensor.sensor_hdr_movie.gain_interval = 0;
    sonyimx134_sensor.sensor_hdr_movie.gain_switch2 = 0;
    sonyimx134_sensor.sensor_hdr_movie.gain_interval2 = 0;
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_set_default;
 * Description : init sonyimx134_sensor;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
*/
static void sonyimx134_set_default(void)
{
    sonyimx134_sensor.init = sonyimx134_init;
	sonyimx134_sensor.exit = sonyimx134_exit;
	sonyimx134_sensor.shut_down = sonyimx134_shut_down;
	sonyimx134_sensor.reset = sonyimx134_reset;
	sonyimx134_sensor.check_sensor = sonyimx134_check_sensor;
	sonyimx134_sensor.power = sonyimx134_power;
	sonyimx134_sensor.init_reg = sonyimx134_init_reg;
	sonyimx134_sensor.stream_on = sonyimx134_stream_on;

	sonyimx134_sensor.get_format = sonyimx134_get_format;
	sonyimx134_sensor.set_flash = NULL;
	sonyimx134_sensor.get_flash = NULL;
	sonyimx134_sensor.set_scene = NULL;
	sonyimx134_sensor.get_scene = NULL;

	sonyimx134_sensor.enum_framesizes = sonyimx134_enum_framesizes;
	sonyimx134_sensor.try_framesizes = sonyimx134_try_framesizes;
	sonyimx134_sensor.set_framesizes = sonyimx134_set_framesizes;
	sonyimx134_sensor.get_framesizes = sonyimx134_get_framesizes;

	sonyimx134_sensor.enum_frame_intervals = sonyimx134_enum_frame_intervals;
	sonyimx134_sensor.try_frame_intervals = NULL;
	sonyimx134_sensor.set_frame_intervals = NULL;
	sonyimx134_sensor.get_frame_intervals = NULL;

	sonyimx134_sensor.get_vts_reg_addr = sonyimx134_get_vts_reg_addr;

	sonyimx134_sensor.get_capability = sonyimx134_get_capability;

	sonyimx134_sensor.set_hflip = sonyimx134_set_hflip;
	sonyimx134_sensor.get_hflip = sonyimx134_get_hflip;
	sonyimx134_sensor.set_vflip = sonyimx134_set_vflip;
	sonyimx134_sensor.get_vflip = sonyimx134_get_vflip;
	sonyimx134_sensor.update_flip = sonyimx134_update_flip;

	sonyimx134_sensor.sensor_hw_3a.support_awb_otp = sonyimx134_support_awb_otp;

	strncpy(sonyimx134_sensor.info.name,"sonyimx134",sizeof(sonyimx134_sensor.info.name));
	sonyimx134_sensor.interface_type = MIPI1;
	sonyimx134_sensor.mipi_lane_count = CSI_LINES_4;
	sonyimx134_sensor.mipi_index = CSI_INDEX_0;
	sonyimx134_sensor.sensor_index = CAMERA_SENSOR_PRIMARY;
	sonyimx134_sensor.skip_frames = 2;//sonyimx134_sensor.skip_frames = 1;

	sonyimx134_sensor.power_conf.pd_valid = LOW_VALID;
	sonyimx134_sensor.power_conf.reset_valid = LOW_VALID;
	sonyimx134_sensor.power_conf.vcmpd_valid = LOW_VALID;

	sonyimx134_sensor.i2c_config.index = I2C_PRIMARY;
	sonyimx134_sensor.i2c_config.speed = I2C_SPEED_400;
	sonyimx134_sensor.i2c_config.addr = SONYIMX134_SLAVE_ADDRESS;
	sonyimx134_sensor.i2c_config.addr_bits = I2C_16BIT;
	sonyimx134_sensor.i2c_config.val_bits = I2C_8BIT;

	sonyimx134_sensor.preview_frmsize_index = 0;
	sonyimx134_sensor.capture_frmsize_index = 0;
	sonyimx134_sensor.frmsize_list = sonyimx134_framesizes;
	sonyimx134_sensor.fmt[STATE_PREVIEW] = V4L2_PIX_FMT_RAW10;
	sonyimx134_sensor.fmt[STATE_CAPTURE] = V4L2_PIX_FMT_RAW10;

#ifdef SONYIMX134_AP_WRITEAE_MODE /* just an example and test case for AP write AE mode */
	sonyimx134_sensor.aec_addr[0] = 0;
	sonyimx134_sensor.aec_addr[1] = 0;
	sonyimx134_sensor.aec_addr[2] = 0;
	sonyimx134_sensor.agc_addr[0] = 0;
	sonyimx134_sensor.agc_addr[1] = 0;
	sonyimx134_sensor.ap_writeAE_delay = 6000; /* 5 expo and gain registers, 6000 us is enough */
#else
	sonyimx134_sensor.aec_addr[0] = 0x0000;
	sonyimx134_sensor.aec_addr[1] = 0x0202;
	sonyimx134_sensor.aec_addr[2] = 0x0203;
	sonyimx134_sensor.agc_addr[0] = 0x0000; /*0x0204 high byte not needed*/
	sonyimx134_sensor.agc_addr[1] = 0x0205;
#endif
	sonyimx134_sensor.sensor_type = SENSOR_SONY;
	sonyimx134_sensor.sensor_rgb_type = SENSOR_RGGB;/* changed by y00231328. add bayer order*/

	sonyimx134_sensor.set_gain = sonyimx134_set_gain;
	sonyimx134_sensor.set_exposure = sonyimx134_set_exposure;
	sonyimx134_sensor.set_exposure_gain = sonyimx134_set_exposure_gain;
	sonyimx134_sensor.set_vts = sonyimx134_set_vts;

	sonyimx134_sensor.get_override_param = sonyimx134_get_override_param;

	sonyimx134_sensor.sensor_gain_to_iso = sonyimx134_gain_to_iso;
	sonyimx134_sensor.sensor_iso_to_gain = sonyimx134_iso_to_gain;

	sonyimx134_sensor.get_sensor_aperture = sonyimx134_get_sensor_aperture;
	sonyimx134_sensor.get_equivalent_focus = NULL;

	sonyimx134_sensor.set_effect = NULL;

	sonyimx134_sensor.isp_location = CAMERA_USE_K3ISP;
	sonyimx134_sensor.sensor_tune_ops = NULL;

	sonyimx134_sensor.af_enable = 1;
	sonyimx134_sensor.get_flash_awb = sonyimx134_get_flash_awb;

	sonyimx134_sensor.image_setting.lensc_param = sonyimx134_lensc_param;
	sonyimx134_sensor.image_setting.ccm_param = sonyimx134_ccm_param;
	sonyimx134_sensor.image_setting.awb_param = sonyimx134_awb_param;
	sonyimx134_sensor.fps_max = 30;
	sonyimx134_sensor.fps_min = 15;
	sonyimx134_sensor.fps = 25;
	sonyimx134_sensor.owner = THIS_MODULE;

	sonyimx134_sensor.info.facing = CAMERA_FACING_BACK;
	sonyimx134_sensor.info.orientation = 270;
	sonyimx134_sensor.info.focal_length = 296;	/* 2.96mm */
	sonyimx134_sensor.info.h_view_angle = 75;	/*  66.1 degree */
	sonyimx134_sensor.info.v_view_angle = 75;
	sonyimx134_sensor.lane_clk = CLK_750M;
	sonyimx134_sensor.hflip = 0;
	sonyimx134_sensor.vflip = 0;
	sonyimx134_sensor.old_flip = 0;

	sonyimx134_sensor.get_gain     				= sonyimx134_get_gain;
	sonyimx134_sensor.get_exposure   			= sonyimx134_get_exposure;
	sonyimx134_sensor.init_isp_reg 				= sonyimx134_init_isp_reg;
	sonyimx134_sensor.support_summary = false;
    sonyimx134_sensor.get_ccm_pregain = NULL;
    sonyimx134_sensor.set_awb = NULL;
    sonyimx134_sensor.set_anti_banding = NULL;
    sonyimx134_sensor.update_framerate = NULL;
    sonyimx134_sensor.awb_dynamic_ccm_gain = NULL;
    sonyimx134_sensor.pclk = 0;
    sonyimx134_sensor.max_int_lines = 0;
    sonyimx134_sensor.min_int_lines = 0;
    sonyimx134_sensor.real_int_lines = 0;
    sonyimx134_sensor.min_gain = 0;
    sonyimx134_sensor.max_gain = 0;
    sonyimx134_sensor.real_gain = 0;
    sonyimx134_sensor.rcc_enable = false;
    sonyimx134_set_hdr_default();
    sonyimx134_sensor.isp_idi_skip = false;
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_module_init;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static __init int sonyimx134_module_init(void)
{
	sonyimx134_set_default();
	return register_camera_sensor(sonyimx134_sensor.sensor_index, &sonyimx134_sensor);
}

/*
 **************************************************************************
 * FunctionName: sonyimx134_module_exit;
 * Description : NA;
 * Input       : NA;
 * Output      : NA;
 * ReturnValue : NA;
 * Other       : NA;
 **************************************************************************
 */
static void __exit sonyimx134_module_exit(void)
{
	unregister_camera_sensor(sonyimx134_sensor.sensor_index, &sonyimx134_sensor);
}

MODULE_AUTHOR("Hisilicon");
module_init(sonyimx134_module_init);
module_exit(sonyimx134_module_exit);
MODULE_LICENSE("GPL");

/********************************** END **********************************************/
