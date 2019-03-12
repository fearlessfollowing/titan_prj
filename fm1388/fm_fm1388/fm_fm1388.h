/*
 * external/fm_fm1388/fm_fm1388.c
 * (C) Copyright 2014-2018
 * Fortemedia, Inc. <www.fortemedia.com>
 * Author: HenryZhang <henryhzhang@fortemedia.com>;
 * 			LiFu <fuli@fortemedia.com>
 *
 * This program is a console application which can communicate with FM1388 driver.
 */
#ifndef __FM_SMVD_H__
#define __FM_SMVD_H__

#include "../libfm1388.h"


//*****************************************************************************
// Definition of the help messages
char cstr_fm_fm1388_help[][128] = {"---------- FM_FM1388 Ver. 1.1.0 ---------",
									"------------ Copyright @2018 ------------",
									"press 'g' to get the current DSP Mode.",
									"press 's' to set DSP Mode.",
									"press 'c' to start SPI playback.",
									"press 'd' to stop SPI playback.",
									"press 'e' to start recording.",
									"press 'f' to stop recording.",
									"press 'r' to get reg value.",
									"press 't' to set reg value.",
									"press 'm' to get memory value.",
									"press 'p' to set memory value.",
									"press 'q' to quit from the program.",
									" "
									};
typedef struct rec_param_t {
	char 			rec_file[MAX_PATH_LEN + 1];
	int 			ch_num;
	unsigned char	ch_idx[DSP_SPI_REC_CH_NUM];
	int 			frame_size;
	unsigned int 	sample_rate;
	unsigned int 	bits_per_sample;
} rec_param;

typedef struct playback_param_t {
	char 			wav_file[MAX_PATH_LEN + 1];
	char 			channel_mapping[MAX_MAPPING_STR_LEN + 1];
	char			need_recording;
	char 			rec_file[MAX_PATH_LEN + 1];
	int 			rec_ch_num;
	unsigned char	rec_ch_idx[DSP_SPI_REC_CH_NUM];
} play_param;

int fm_keyin2action(void);
int fm_smvd_help_display(void);

extern int lib_open_device(void);
extern bool lib_close_device(void);
extern int get_mode(void);
extern int set_mode(int dsp_mode);
extern int get_reg_value(int reg_addr);
extern int set_reg_value(int reg_addr, int value);
extern int get_dsp_mem_value(unsigned int mem_addr);
extern int set_dsp_mem_value(unsigned int mem_addr, int value);
extern int get_dsp_mem_value_spi(unsigned int mem_addr);
extern int start_spi_playback(char* channel_mapping, char* filepath);
extern int stop_spi_playback();

extern int get_channel_number(char* file_path);
extern bool check_channel_mapping(int channel_num, char* channel_mapping);
extern int start_debug_record(unsigned int ch_num, unsigned char* ch_idx, char* filepath);
extern int close_cdev(int fm_cdev_hdl);

#endif // __FM_SMVD_H__
