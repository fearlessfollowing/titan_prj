/*
 * external/libfmrec_1388/libfmrec.h
 * (C) Copyright 2014-2018
 * Fortemedia, Inc. <www.fortemedia.com>
 * Author: LiFu <fuli@fortemedia.com>
 *
 * This program is dynamic library which privode interface to 
 * convert pcm data file to wav file .
 */
#ifndef LIB_FM_REC_H_
#define LIB_FM_REC_H_

#include <stdbool.h>
#include "fm_wav.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */
#define ESUCCESS			0
#define EUNKNOWN			10000
#define EDUPOPEN			10001
#define EFAILOPEN			10002
#define ENOTOPEN			10003
#define EPARAMINVAL			10004
#define EDATAINVAL			10005
#define ESPIREAD			10006
#define ESPIWRITE			10007
#define ESPISETFAIL			10008
#define EDSPNOTWORK			10009
#define ENOMEMORY			10010
#define EMEMFAULT			10011
#define ECANNOTENRECORD		10012
#define ENOSDCARD			10013
#define EFILECANNOTWRITE	10014
#define EINPROCESSING		10015
#define EFAILTOSETMODE		10016
#define ECOMMANDINVAL		10017

#define FM1388_BUFFER_LEN 	4096
#define MAX_FILENAME_LEN	50
#define MAX_COMMENT_LEN		100
#define MAX_MODE_NUM		20
#define CONFIG_LINE_LEN		255
#define DSP_SPI_REC_CH_NUM	10
#define DSP_SPI_REC_EXTRA_CHUNK_SIZE	1024
#define DSP_SPI_REC_EXTRA_CHUNK_HEADER	8
#define DSP_SPI_REC_INITIAL_CHAR		0xFE

#define CUBIE_TRUCK
#ifdef CUBIE_TRUCK
#define DEFAULT_MODE_CFG_LOCATION	"/system/etc/firmware/"
#else
#define DEFAULT_MODE_CFG_LOCATION	"/system/lib/firmware/"
#endif
#define VOICE_DATA_FILE_NAME 	"voice.dat"
#define DEFAULT_SDCARD_PATH 	"/sdcard/" //"/mnt/sdcard/"

#define MAX_PATH_LEN		128
#define MAX_MAPPING_STR_LEN	DSP_SPI_REC_CH_NUM * 3
#define MAX_MAP_CH_NUM		6


#define MODE_CFG_FILE_NAME		"FM1388_mode.cfg"
#define USER_DEFINED_PATH_FILE	"user_defined_path.cfg"
#define USER_VEC_PATH			"VEC_PATH="
#define USER_KERNEL_SDCARD_PATH	"K_SD_PATH="
#define USER_USER_SDCARD_PATH	"U_SD_PATH="
#define USER_OUTPUT_LOG			"OUTPUT_LOG="
#define USER_USER_LOG_PREFIX	"ulog-"
#define USER_KERNEL_LOG_PREFIX	"klog-"
#define USER_LOG_FOLDER			"FMLog"

typedef struct user_defined_path_t {
	char 	sdcard_path[MAX_PATH_LEN];
	char 	mode_cfg_path[CONFIG_LINE_LEN];
	char	output_log[MAX_PATH_LEN];
	bool	b_output_log;
} user_defined_path;

static const char valid_channels[MAX_MAP_CH_NUM][3] = {"M0", "M1", "M2", "M3", "RL", "RR"};
static const char valid_channels_0426[MAX_MAP_CH_NUM][3] = {"M0", "M1", "M2", "RL", "RR", ""};
static const unsigned int 	sample_rate_list[] 		= {8000, 16000, 24000, 48000};
static const unsigned int 	bits_per_sample_list[] 	= {8, 16, 24, 32};
//static const unsigned char 	channelName[DSP_SPI_REC_CH_NUM][16] 	= { "SPI1:MIC0", "SPI2:MIC1", "SPI3:MIC2", "SPI4:MIC3",
//													"SPI5:AECREFL", "SPI6:AECREFR", "SPI7:LIN", "SPI8:LOUT",
//													"SPI9:SPKOUT", "SPI10:DBG_INFO" };
//should use this array instead of above
static const unsigned char 	channelName[DSP_SPI_REC_CH_NUM][16] 	= { "MIC0", "MIC1", "MIC2", "MIC3",
																		"AECREFL", "AECREFR", "LIN", "LOUT",
																		"SPKOUT", "DBG_INFO" };
static const unsigned char 	channelName_0426[DSP_SPI_REC_CH_NUM][16]= { "MIC0", "MIC1", "MIC2", "AECREFL",
																		"AECREFR", "LINL", "LINR", "LOUTL",
																		"LOUTR", "DBG_INFO" };
//

typedef struct cfg_mode_cmd_t {
	unsigned int 	mode;
	char 	path_setting_file_name[MAX_FILENAME_LEN];
	char 	mode_setting_file_name[MAX_FILENAME_LEN];
	char 	comment[MAX_COMMENT_LEN];
} cfg_mode_cmd;

typedef struct dev_cmd_start_rec_t {
	unsigned short cmd_name;
	unsigned short ch_num;
	unsigned char  ch_idx[DSP_SPI_REC_CH_NUM];
	unsigned char  hd_reserved[2];
} dev_cmd_start_rec;

typedef struct dev_cmd_spi_play_t {
	unsigned short 	cmd_name;
	char  			file_path[MAX_PATH_LEN];
	char  			channel_mapping[DSP_SPI_REC_CH_NUM + 1];
	char			need_recording;
	unsigned short 	rec_ch_num;
	unsigned char  	rec_ch_idx[DSP_SPI_REC_CH_NUM];
} dev_cmd_spi_play;

typedef struct support_mode_t {
	unsigned char number;
	char mode[MAX_MODE_NUM];
	char comment[MAX_MODE_NUM][MAX_COMMENT_LEN];
} support_mode;

int load_fm1388_mode_cfg(char* file_src, support_mode* mode_info);
int convert_data(const char* data_file_name, const char* wav_file_name, 
				unsigned int sample_rate, unsigned int bits_per_sample, 
				int channel_number, const unsigned char* channels, int frame_size, bool after_0426);
int get_dsp_sample_rate(int index);
int load_user_setting(void);
char* get_cfg_path(void);
char* get_sdcard_root(void);
bool is_output_log(void);
int output_debug_log(bool output_console, const char *fmt, ...); 

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */ 

#endif /* LIB_FM_REC_H_ */
