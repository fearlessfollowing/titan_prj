/*
 * external/libfm1388/libfm1388.h
 * (C) Copyright 2014-2018
 * Fortemedia, Inc. <www.fortemedia.com>
 * Author: HenryZhang <henryhzhang@fortemedia.com>;
 * 			LiFu <fuli@fortemedia.com>
 *
 * This program is dynamic library which privode interface to 
 * let fm_fm1388 application communicate with FM1388 driver.
 */
#ifndef LIB_FM_SMVD_H_
#define LIB_FM_SMVD_H_

#include <stdbool.h>
#include "libfmrec.h"
#include "errorcode.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#define DSP_SPI_FRAMESIZE_ADDR	0x5FFDFF90
#define DSP_SAMPLE_RATE_ADDR 	0x5FFDF72A
#define DSP_FIRMWARE_BUILD_DATE	0x5FFDFF5C
#define DSP_PARAMETER_RELEASE_DATE 0x5FFDFDFC
#define DSP_FIRMWARE_VERSION	0x5FFDFD7E

/*
 * Henry Zhang - define structure and enum for device_read/device_write operations
 */
enum DEV_COMMAND {
	//The long commands
	FM_SMVD_REG_READ,		//Command #0
	FM_SMVD_REG_WRITE,		//Command #1
	FM_SMVD_DSP_ADDR_READ,	//Command #2
	FM_SMVD_DSP_ADDR_WRITE,	//Command #3
	FM_SMVD_MODE_SET,		//Command #4
	FM_SMVD_MODE_GET,		//Command #5
	//The long commands
	FM_SMVD_DSP_BWRITE,		//Command #6
	FM_SMVD_VECTOR_GET,		//Command #7
	FM_SMVD_REG_DUMP,		//Command #8
	
	FM_SMVD_DSP_ADDR_READ_SPI,
	FM_SMVD_DSP_ADDR_WRITE_SPI,

	FM_SMVD_DSP_FETCH_VDATA_START,
	FM_SMVD_DSP_FETCH_VDATA_STOP,

	FM_SMVD_DSP_PLAYBACK_START,
	FM_SMVD_DSP_PLAYBACK_STOP,
	FM_SMVD_DSP_IS_PLAYING,
};

/*
 * The structure dev_cmd_short/long defines the command protocol between the library and the device driver.
 * In device driver, the device read and device write functions handle the structure data and parse it.
 * 
 * dev_cmd_short: the structure for device command FM_SMVD_REG_READ, FM_SMVD_REG_WRITE, FM_SMVD_DSP_READ,
 * FM_SMVD_DSP_WRITE, FM_SMVD_MODE_SET and FM_SMVD_MODE_GET, for which no extra data buffer is needed.
 */
 typedef struct dev_cmd_short_t {
	unsigned short 	cmd_name;	//The commands from #0~#5
	unsigned int 	addr;			//The address of the register or dsp memory for the commands #0~#3, or the dsp mode for #4~#5. 
	unsigned int 	val;			//The operation or returned value for the commands #0~#3, or zero for the commands #4~#5.
	unsigned char  	reserved[6];
} dev_cmd_short;
/* 
 * dev_cmd_long: the structure for device command FM_SMVD_DSP_BWRITE, FM_SMVD_VECTOR_GET and FM_SMVD_REG_DUMP,
 * for which the extra data buffer is necessary for input or output data.
 */
/* 
typedef struct dev_cmd_long_t {
	unsigned short 	cmd_name;	//The command from #6~#8
	unsigned int 	addr;			//The address of dsp memory for the command #6, or zero for #7~#8. 
	unsigned int 	val;			//The the valid data length.
	unsigned char  	reserved[6];
	unsigned char  	buf[CMD_BUF_LEN];	//The data buffer in fixed size, for input and output.
} dev_cmd_long;
*/

typedef struct dev_cmd_mode_gs_t {
	unsigned short 	cmd_name;
	unsigned int 	dsp_mode;
	unsigned char  	hd_reserved[10];
} dev_cmd_mode_gs;

typedef struct dev_cmd_reg_rw_t {
	unsigned short 	cmd_name;
	unsigned int 	reg_addr;
	unsigned int 	reg_val;
	unsigned char  	hd_reserved[6];
} dev_cmd_reg_rw;

typedef struct dev_cmd_record_result_t {
	unsigned int total_frame_number;
	unsigned int error_frame_number;
	unsigned int first_error_frame_counter;
	unsigned int  last_error_frame_counter;
} dev_cmd_record_result;

typedef struct dev_cmd_playback_result_t {
	unsigned int total_frame_number;
	unsigned int error_frame_number;
	unsigned int first_error_frame_counter;
	unsigned int  last_error_frame_counter;
} dev_cmd_playback_result;

enum DSP_MODE {
	FM_SMVD_DSP_BYPASS,		//the bypass mode of the dsp. in this mode the DMIC input is bypassed to codec.
	FM_SMVD_DSP_DETECTION,
	FM_SMVD_DSP_MIXTURE,
	FM_SMVD_DSP_FACTORY,
	FM_SMVD_DSP_VR,			//the voice recognition mode of the dsp.
	FM_SMVD_DSP_CM,			//the communication mode of the dsp.
	FM_SMVD_DSP_BARGE_IN,	//the barge-in mode of the dsp. in this mode FM_SMVD detects the keyword and issues interrupt to the host AP.
	FM_SMVD_GET_DSP_MODE,
	FM_SMVD_DUMP_REGISTER,
};

enum FM_SMVD_ACTION {
	SET_BYPASS_MODE,
	SET_DETECTION_MODE,
	SET_MIXTURE_MODE,
	SET_FACTORY_MODE,
	SET_VR_MODE,
	SET_CM_MODE,
	SET_BARGE_IN_MODE,
	GET_DSP_MODE,
	GET_REG_VALUE,
	GET_MEM_VALUE,
	SET_REG_VALUE,
	SET_MEM_VALUE,
	DUMP_REGISTER,
	DISPLAY_HELP,
	QUIT_PROGRAM,
    SET_DSP_MODE,
    START_VBUF_READ,
    STOP_VBUF_READ,
    START_VBUF_FETCHING,
    STOP_VBUF_FETCHING,
    DL_EFT_FW,
    START_VBUF_WRITE,
    STOP_VBUF_WRITE
};

enum DSP_CFG_MODE {
	FM_SMVD_CFG_VR,
	FM_SMVD_CFG_CM,
	FM_SMVD_CFG_BARGE_IN = 3,
};

int lib_open_device(void);
bool lib_close_device(void);
char* get_cfg_location(void);
char* get_sdcard_path(void);
bool get_output_log(void);

int get_mode(void);
int set_mode(int dsp_mode);
int get_reg_value(int reg_addr);
int set_reg_value(int reg_addr, int value);
int get_dsp_mem_value(unsigned int mem_addr);
int set_dsp_mem_value(unsigned int mem_addr, int value);
int get_dsp_mem_value_spi(unsigned int mem_addr);
int set_dsp_mem_value_spi(unsigned int mem_addr, int value);
int start_debug_record(unsigned int ch_num, unsigned char* ch_idx, char* filepath);
/*Fuli 20160926 we will get sample rate from DSP instead of input by user. will hardcode bits_per_sample to 16
int stop_debug_record(char* wav_file_name, 
						unsigned int sample_rate, unsigned int bits_per_sample);
*/
int stop_debug_record(const char* wav_file_name, const unsigned char* channels, dev_cmd_record_result *rec_result);
int stop_debug_record_force(void);
int stop_debug_record_by_ADBTool(const char* wav_file_name, const unsigned char* channels, int channel_number, dev_cmd_record_result *rec_result);

int get_channel_number(char* file_path);
bool check_channel_mapping(int channel_num, char* channel_mapping);
int start_spi_playback(char* channel_mapping, char* filepath);
int start_spi_playback_by_ADBTool(char* channel_mapping, char* filepath,
									unsigned char cPlayMode, unsigned char cPlayOutput);
int stop_spi_playback(dev_cmd_playback_result *playback_result);
int start_spi_playback_rec(char* channel_mapping, char* filepath, char need_recording, 
			unsigned int rec_ch_num, unsigned char* rec_ch_idx, char* rec_filepath);
int start_spi_playback_rec_by_ADBTool(char* channel_mapping, char* filepath, char need_recording, 
			unsigned int rec_ch_num, unsigned char* rec_ch_idx, char* rec_filepath,
			unsigned char cPlayMode, unsigned char cPlayOutput);
int stop_spi_playback_rec(const char* wav_file_name, const unsigned char* channels, dev_cmd_playback_result *playback_result);
int is_playing();
int close_cdev(int fm_cdev_hdl);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */ 

#endif /* LIB_FM_SMVD_H_ */
