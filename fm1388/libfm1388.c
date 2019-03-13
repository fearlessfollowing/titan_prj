/*
 * external/libfm1388/libfm1388.c
 * (C) Copyright 2014-2018
 * Fortemedia, Inc. <www.fortemedia.com>
 * Author: HenryZhang <henryhzhang@fortemedia.com>;
 * 			LiFu <fuli@fortemedia.com>
 *
 * This program is dynamic library which privode interface to 
 * let fm_fm1388 application communicate with FM1388 driver.
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <linux/input.h>
#include <pthread.h>
#include <sys/stat.h>
#include <string.h>
//#include <cutils/log.h>
//#include <cutils/atomic.h>
#include <unistd.h>


#include "libfm1388.h"

#define DEVICE_NAME 		 "/dev/fm1388"
#define CHAR_DEVICE_NAME 	 "/dev/fm1388_smp1"


static unsigned int frame_size = 0;
static unsigned int channel_num = 0;
static unsigned int rec_channel_num = 0;
static int dev_fd = -1;
static int dev_fd_cnt = 0;

char* get_cfg_location(void) 
{
	return get_cfg_path();
}

char* get_sdcard_path(void) 
{
	return get_sdcard_root();
}

bool get_output_log(void) 
{
	return is_output_log();
}

//open device and keep its handle for further calling
//application which call lib_open_device should call lib_close_device() to close the handle
int lib_open_device(void)
{
	load_user_setting();
	
	if (dev_fd != -1) {
		dev_fd_cnt++;
		return dev_fd;
	}
	
	dev_fd = open(DEVICE_NAME, O_RDWR);  
	if (dev_fd == -1) {  
		printf("Failed to open device %s.\n", DEVICE_NAME);
		return -EFAILOPEN;
	}
	dev_fd_cnt++;

	return ESUCCESS;
}

//close the device handle
bool lib_close_device(void)
{
	if (dev_fd != -1) {
		dev_fd_cnt--;
		if(dev_fd_cnt == 0) {
			close(dev_fd);
			dev_fd = -1;
		}
	} else {
		return false;
	}
	
	return true;
}

//get dsp mode
int get_mode(void)
{
	dev_cmd_mode_gs local_dev_cmd;
	
	if (dev_fd == -1) {
		output_debug_log(false, "[libfm1388-%s] device handle is -1, please make sure you have opened device correcly.\n", __func__);
		return -ENOTOPEN;
	}
	
	local_dev_cmd.cmd_name = FM_SMVD_MODE_GET;
	local_dev_cmd.dsp_mode = 0;
	read(dev_fd, &local_dev_cmd, sizeof(dev_cmd_mode_gs));
	output_debug_log(false, "[libfm1388-%s] returned mode=%x.\n", __func__, local_dev_cmd.dsp_mode);
	
	return local_dev_cmd.dsp_mode;
}

//set dsp mode
int set_mode(int dsp_mode)
{
	dev_cmd_mode_gs local_dev_cmd;

	if (dev_fd == -1) {
		output_debug_log(false, "[libfm1388-%s] device handle is -1, please make sure you have opened device correcly.\n", __func__);
		return -ENOTOPEN;
	}
	
	local_dev_cmd.cmd_name = FM_SMVD_MODE_SET;
	local_dev_cmd.dsp_mode = dsp_mode;
	write(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short));
	output_debug_log(false, "[libfm1388-%s] change mode to: %x.\n", __func__, dsp_mode);

	return ESUCCESS;
}

//get register value
int get_reg_value(int reg_addr)
{
	dev_cmd_reg_rw local_dev_cmd;

	if (dev_fd == -1) {
		output_debug_log(false, "[libfm1388-%s] device handle is -1, please make sure you have opened device correcly.\n", __func__);
		return -ENOTOPEN;
	}
	
	local_dev_cmd.cmd_name = FM_SMVD_REG_READ;
	local_dev_cmd.reg_addr = reg_addr;
	read(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short));
	output_debug_log(false, "[libfm1388-%s] get reg(%02x) return value=%x.\n", __func__, reg_addr, local_dev_cmd.reg_val & 0xFFFF);

	return (local_dev_cmd.reg_val & 0xFFFF);
}

//set value to register 
int set_reg_value(int reg_addr, int value)
{
	dev_cmd_reg_rw local_dev_cmd;

	if (dev_fd == -1) {
		output_debug_log(false, "[libfm1388-%s] device handle is -1, please make sure you have opened device correcly.\n", __func__);
		return -ENOTOPEN;
	}
	
	local_dev_cmd.cmd_name = FM_SMVD_REG_WRITE;
	local_dev_cmd.reg_addr = reg_addr;
	local_dev_cmd.reg_val  = value;
	output_debug_log(false, "[libfm1388-%s] change reg(%02x) value to: %x.\n", __func__, reg_addr, value);
	
	write(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short));

	return ESUCCESS;
}

//get value from dsp memory
int get_dsp_mem_value(unsigned int mem_addr)
{
	dev_cmd_short local_dev_cmd;
	
	if (dev_fd == -1) {
		output_debug_log(false, "[libfm1388-%s] device handle is -1, please make sure you have opened device correcly.\n", __func__);
		return -ENOTOPEN;
	}
	
	local_dev_cmd.cmd_name = FM_SMVD_DSP_ADDR_READ;
	local_dev_cmd.addr = mem_addr;
	
	read(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short));
	output_debug_log(false, "[libfm1388-%s] get addr(%08x) return value=%x.\n", __func__, mem_addr, local_dev_cmd.val);

	return (local_dev_cmd.val);
}

//set value to dsp memory
int set_dsp_mem_value(unsigned int mem_addr, int value)
{
	dev_cmd_short local_dev_cmd;
	
	if (dev_fd == -1) {
		output_debug_log(false, "[libfm1388-%s] device handle is -1, please make sure you have opened device correcly.\n", __func__);
		return -ENOTOPEN;
	}
	
	local_dev_cmd.cmd_name 	= FM_SMVD_DSP_ADDR_WRITE;
	local_dev_cmd.addr 		= mem_addr;
	local_dev_cmd.val 		= value;
	
	write(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short)); 
	output_debug_log(false, "[libfm1388-%s] change memory(%08x) value to: %x.\n", __func__, mem_addr, value);

	return ESUCCESS;    
}

//get value of memory address via spi
int get_dsp_mem_value_spi(unsigned int mem_addr)
{
	dev_cmd_short local_dev_cmd;
	
	if (dev_fd == -1) {
		output_debug_log(false, "[libfm1388-%s] device handle is -1, please make sure you have opened device correcly.\n", __func__);
		return -ENOTOPEN;
	}
	
	local_dev_cmd.cmd_name 	= FM_SMVD_DSP_ADDR_READ_SPI;
	local_dev_cmd.addr 		= mem_addr;
	
	read(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short));
	output_debug_log(false, "[libfm1388-%s] get addr(%08x) return value=%x.\n", __func__, mem_addr, local_dev_cmd.val);

	return local_dev_cmd.val;
}

//set value to memory address via spi
int set_dsp_mem_value_spi(unsigned int mem_addr, int value) 
{
	dev_cmd_short local_dev_cmd;

	if (dev_fd == -1) {
		output_debug_log(false, "[libfm1388-%s] device handle is -1, please make sure you have opened device correcly.\n", __func__);
		return -ENOTOPEN;
	}
	
	local_dev_cmd.cmd_name 	= FM_SMVD_DSP_ADDR_WRITE_SPI;
	local_dev_cmd.addr 		= mem_addr;
	local_dev_cmd.val 		= value;
	
	write(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short));
	output_debug_log(false, "[libfm1388-%s] change memory(%08x) value to: %x.\n", __func__, mem_addr, value);

	return ESUCCESS;    
}

int convert_channel_index(unsigned int ch_num, unsigned char* ch_idx, unsigned short* final_ch_num, unsigned char* final_ch_idx) 
{
	int i;
	
	if (final_ch_idx == NULL) return -EPARAMINVAL;
	if (final_ch_num == NULL) return -EPARAMINVAL;
	
	if ((ch_num > 0) && (ch_num <= DSP_SPI_REC_CH_NUM)) {
		*final_ch_num	= ch_num;
		
		if (ch_idx) {
			memcpy(final_ch_idx, ch_idx, sizeof(char) * DSP_SPI_REC_CH_NUM);
			for(i = 0; i < DSP_SPI_REC_CH_NUM; i++) {
				if(final_ch_idx[i] == '0') final_ch_idx[i] = 0;
			}
		} else {
			*final_ch_num	= DSP_SPI_REC_CH_NUM;
			memset(final_ch_idx, '1', sizeof(char) * DSP_SPI_REC_CH_NUM);
		}
	} else {
		*final_ch_num	= DSP_SPI_REC_CH_NUM;
		memset(final_ch_idx, '1', sizeof(char) * DSP_SPI_REC_CH_NUM);
	}

	return ESUCCESS;
}

//start to record
//ch_idx is channel array, '1' means record the related channel, ' ' means omit this channel
int start_debug_record(unsigned int ch_num, unsigned char* ch_idx, char* filepath)
{
	int ret_val = ESUCCESS;
	dev_cmd_start_rec local_dev_cmd;
	char data_file_path[MAX_PATH_LEN] = { 0 };
	int i;
	
	if (dev_fd == -1) {
		output_debug_log(false, "[libfm1388-%s] device handle is -1, please make sure you have opened device correcly.\n", __func__);
		return -ENOTOPEN;
	}
	
	if (filepath == NULL) {
		output_debug_log(false, "[libfm1388-%s] got empty file path.\n", __func__);
		return -EPARAMINVAL;
	}
	
	snprintf(data_file_path, MAX_PATH_LEN, "%s%s", get_sdcard_path(), VOICE_DATA_FILE_NAME);
	unlink(data_file_path);

	local_dev_cmd.cmd_name 		= FM_SMVD_DSP_FETCH_VDATA_START;
	
	ret_val = convert_channel_index(ch_num, ch_idx, &(local_dev_cmd.ch_num), local_dev_cmd.ch_idx);
	if(ret_val != ESUCCESS) {
		return ret_val;
	}
/*	
	if((ch_num > 0) && (ch_num <= DSP_SPI_REC_CH_NUM)) {
		local_dev_cmd.ch_num	= ch_num;
		
		if(ch_idx) {
			memcpy(local_dev_cmd.ch_idx, ch_idx, sizeof(char) * DSP_SPI_REC_CH_NUM);
		}
		else {
			local_dev_cmd.ch_num	= DSP_SPI_REC_CH_NUM;
			memset(local_dev_cmd.ch_idx, '1', sizeof(char) * DSP_SPI_REC_CH_NUM);
		}
	}
	else {
		local_dev_cmd.ch_num	= DSP_SPI_REC_CH_NUM;
		memset(local_dev_cmd.ch_idx, '1', sizeof(char) * DSP_SPI_REC_CH_NUM);
	}
*/	

	frame_size = get_dsp_mem_value_spi(DSP_SPI_FRAMESIZE_ADDR) & 0x0000FFFF;
	if ((frame_size <= 0) || (frame_size > 0x1000)) { //suppose frame size should not be too large
		printf("framesize is not correct. frame_size=%d\n", frame_size);
		output_debug_log(false, "[libfm1388-%s] framesize is not correct. frame_size=%d\n", __func__, frame_size);
		return -EDATAINVAL;
	}
	
	channel_num = ch_num;
	
	ret_val = write(dev_fd, &local_dev_cmd, sizeof(dev_cmd_start_rec));
	if (ret_val < 0) {
		printf("error occurs when start recording. error code=%d\n", ret_val);
		output_debug_log(false, "[libfm1388-%s] error occurs when start recording. error code=%d\n", __func__, ret_val);
		return ret_val;
	}
	
	output_debug_log(false, "[libfm1388-%s] Start SPI Recording with: \n", __func__);
	output_debug_log(false, "[libfm1388-%s] \tchannel_num: %d\n", __func__, local_dev_cmd.ch_num);
	output_debug_log(false, "[libfm1388-%s] \tchannel index: %s\n", __func__, local_dev_cmd.ch_idx);
	return ESUCCESS;
}

//stop recording and convert voice data into wav format then save data to file. 
/*Fuli 20160926 we will get sample rate from DSP instead of input by user. will hardcode bits_per_sample to 16
int stop_debug_record(char* wav_file_name, unsigned int sample_rate, 
						unsigned int bits_per_sample) {
*/
int stop_debug_record(const char* wav_file_name, const unsigned char* channels, dev_cmd_record_result *rec_result) 
{
	dev_cmd_short local_dev_cmd;
	int ret_val 		= ESUCCESS;
	int cur_sample_rate = -1;
	char data_file_path[MAX_PATH_LEN] = { 0 };
	char wav_file_full_path[MAX_PATH_LEN] = { 0 };
	dev_cmd_record_result* temp_cmd = NULL;
	unsigned int build_date = 0;
	
	if (dev_fd == -1) {
		output_debug_log(true, "[libfm1388-%s] device handle is -1, please make sure you have opened device correcly.\n", __func__);
		return -ENOTOPEN;
	}
	
	if (wav_file_name == NULL) {
		output_debug_log(true, "[libfm1388-%s] wav_file_name is null.\n", __func__);
		return -EPARAMINVAL;
	}
	
	if (channels == NULL) {
		output_debug_log(true, "[libfm1388-%s] channels is null.\n", __func__);
		return -EPARAMINVAL;
	}
	
	if (rec_result == NULL) {
		output_debug_log(true, "[libfm1388-%s] rec_result is null.\n", __func__);
		return -EPARAMINVAL;
	}
	
	snprintf(data_file_path, MAX_PATH_LEN, "%s%s", get_sdcard_path(), VOICE_DATA_FILE_NAME);
	
	local_dev_cmd.cmd_name = FM_SMVD_DSP_FETCH_VDATA_STOP;
	ret_val = write(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short));
	if (ret_val <= 0) {
		printf("Error occurs when stop recording data.\n");
		output_debug_log(false, "[libfm1388-%s] Error occurs when stop recording data.\n", __func__);
		return ret_val;
	}
	
	//get sample rate from DSP and hardcode bits_per_sample as 16
	cur_sample_rate = get_dsp_mem_value(DSP_SAMPLE_RATE_ADDR);
	if (cur_sample_rate < 0) {
		printf("got invalid sample rate data:%x\n", cur_sample_rate);
		output_debug_log(false, "[libfm1388-%s] got invalid sample rate data:%x\n\n", __func__, cur_sample_rate);
		return -EDATAINVAL;
	}
	
	cur_sample_rate = get_dsp_sample_rate(cur_sample_rate);
	if (cur_sample_rate < 0) { 
		printf("sample_rate value is not valid:%d.\n", cur_sample_rate);
		output_debug_log(false, "[libfm1388-%s] sample_rate value is not valid:%d.\n", __func__, cur_sample_rate);
		return -EDATAINVAL;
	}

  
	snprintf(wav_file_full_path, MAX_PATH_LEN, "%s%s", get_sdcard_path(), wav_file_name);
	unlink(wav_file_full_path);
	output_debug_log(false, "[libfm1388-%s] voice data file path is:%s\n", __func__, data_file_path);
	output_debug_log(false, "[libfm1388-%s] wav file path is:%s\n", __func__, wav_file_full_path);
	output_debug_log(false, "[libfm1388-%s] channel_num is:%d\n", __func__, channel_num);
	output_debug_log(false, "[libfm1388-%s] channels is:%s\n", __func__, channels);
	output_debug_log(false, "[libfm1388-%s] frame_size is:%d\n", __func__, frame_size);
	output_debug_log(false, "[libfm1388-%s] cur_sample_rate is:%d\n", __func__, cur_sample_rate);
	
 	usleep(5000); //wait for driver stop the recording
	build_date = get_dsp_mem_value(DSP_FIRMWARE_BUILD_DATE);
	ret_val = convert_data(data_file_path, wav_file_full_path, cur_sample_rate, 16, 
						channel_num, channels, frame_size, build_date >= 0x20180426);

	if (ret_val) {
		printf("Error occurs when save recording data. error code=%d\n", ret_val);
		output_debug_log(false, "[libfm1388-%s] Error occurs when save recording data. error code=%d\n", __func__, ret_val);
		return ret_val;
	}
	
	temp_cmd = (dev_cmd_record_result*)&local_dev_cmd;
	output_debug_log(false, "[libfm1388-%s] loss frame number:%d among total frame:%d.\n", 
				__func__, temp_cmd->error_frame_number, temp_cmd->total_frame_number);
	if (temp_cmd->error_frame_number != 0) {
		output_debug_log(false, "[libfm1388-%s] first loss occurs at the %dth frame.\n", 
						__func__, temp_cmd->first_error_frame_counter);
		output_debug_log(false, "[libfm1388-%s] last loss occurs at the %dth frame.\n", 
						__func__, temp_cmd->last_error_frame_counter);
	}
	
	if (rec_result != NULL) {
		rec_result->first_error_frame_counter = temp_cmd->first_error_frame_counter;
		rec_result->last_error_frame_counter = temp_cmd->last_error_frame_counter;
		rec_result->error_frame_number = temp_cmd->error_frame_number;
		rec_result->total_frame_number = temp_cmd->total_frame_number;
	}
	
	return ESUCCESS;
}

//just for ADB Tool
int stop_debug_record_by_ADBTool(const char* wav_file_name, const unsigned char* channels, int channel_number, dev_cmd_record_result *rec_result) 
{	
	frame_size = get_dsp_mem_value_spi(DSP_SPI_FRAMESIZE_ADDR) & 0x0000FFFF;
	if ((frame_size <= 0) || (frame_size > 0x1000)) { //suppose frame size should not be too large
		printf("framesize is not correct. frame_size=%d\n", frame_size);
		output_debug_log(true, "[libfm1388-%s] framesize is not correct. frame_size=%d\n", __func__, frame_size);
		return -EDATAINVAL;
	}
	
	if (rec_result == NULL) {
		output_debug_log(true, "[libfm1388-%s] rec_result is null.\n", __func__);
		return -EPARAMINVAL;
	}
	
	channel_num = channel_number;
	
	output_debug_log(false, "[libfm1388-%s] %s is called.\n", __func__, __func__);
	return stop_debug_record(wav_file_name, channels, rec_result);
}

int stop_debug_record_force(void) 
{
	dev_cmd_short local_dev_cmd;
	int ret_val 		= ESUCCESS;
	
	if (dev_fd == -1) {
		output_debug_log(false, "[libfm1388-%s] device handle is -1, please make sure you have opened device correcly.\n", __func__);
		return -ENOTOPEN;
	}
	
	local_dev_cmd.cmd_name = FM_SMVD_DSP_FETCH_VDATA_STOP;
	ret_val = write(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short));
	if(ret_val <= 0) {
		printf("Error occurs when stop recording data.\n");
		output_debug_log(false, "[libfm1388-%s] Error occurs when stop recording data. ret_val=%d\n", __func__, ret_val);
		return ret_val;
	}
	
	return ESUCCESS;
}

/************************ function and interface for SPI Playback *****************************/
int get_channel_number(char* file_path) 
{
	FILE* 		wav_fp = NULL;
	char 		play_file_full_path[MAX_PATH_LEN + 1] 	= {0};
	wav_header	header;
	int 		ret = 0;
	
	if(file_path == NULL) {
		printf(" - Invalid file path.\n");
		output_debug_log(false, "[libfm1388-%s] Invalid file path.\n", __func__);
		return -EPARAMINVAL;
	}
	
	snprintf(play_file_full_path, MAX_PATH_LEN, "%s%s", get_sdcard_path(), file_path);
	wav_fp = fopen(play_file_full_path, "rb");
	if (wav_fp == NULL) {
		printf(" - Fail to open the input wav file, please make sure it exist and can be read.\n");
		output_debug_log(false, "[libfm1388-%s] Fail to open the input wav file(%s), please make sure it exist and can be read.\n", __func__, play_file_full_path);
		return -EFAILOPEN;
	}

	ret = fread(&header, sizeof(wav_header), 1, wav_fp);

	if (ret <= 0) {
		printf(" - Error occurs when read wav file header.\n");
		output_debug_log(false, "[libfm1388-%s] Error occurs when read wav file header. ret=%d\n", __func__, ret);
		return -EFILECANNOTREAD;
	}
	
	return header.num_channels;
}

bool check_channel_mapping(int channel_num, char* channel_mapping) 
{
	int i, j = 0;
	int len = 0;
	unsigned int build_date = 0;
	
	if(channel_mapping == NULL) {
		output_debug_log(false, "[libfm1388-%s] channel_mapping is null.\n", __func__);
		return false;
	}
	
	if(channel_num == 0) {
		output_debug_log(false, "[libfm1388-%s] channel_num is 0.\n", __func__);
		return false;
	}
	
	len = strlen(channel_mapping);
	if((len % 3) != 0) {
		output_debug_log(false, "[libfm1388-%s] channel_mapping(%s) data is not correct, its length should be N time of 3 at least.\n", __func__, channel_mapping);
		return false;
	}
	
	build_date = get_dsp_mem_value(DSP_FIRMWARE_BUILD_DATE);
	while(j < len) {
		if((channel_mapping[j] >= '0') && (channel_mapping[j] <= '9')) {
			if((channel_mapping[j] - '0' + 1) > channel_num)  break;
			j ++;
			
			for(i = 0; i < MAX_MAP_CH_NUM; i++) {
				if(build_date >= 0x20180426) {
					if(strncasecmp(channel_mapping + j, valid_channels_0426[i], 2) == 0)	break;
				}
				else {
					if(strncasecmp(channel_mapping + j, valid_channels[i], 2) == 0)	break;
				}
			}
			if(i == MAX_MAP_CH_NUM) break;
		}
		else break;
		
		j += 2;
	}
	
	if(j < len) {
		output_debug_log(false, "[libfm1388-%s] channel_mapping(%s) data is not correct.\n", __func__, channel_mapping);
		return false;
	}
	
	return true;
}

int trans_mapping_str(char* channel_mapping, char* trans_mapping_str) 
{
	int i, j, len;
	unsigned int build_date = 0;

	if(channel_mapping == NULL) {
		output_debug_log(false, "[libfm1388-%s] channel_mapping is null.\n", __func__);
		return -EPARAMINVAL;
	}
	
	if(trans_mapping_str == NULL) {
		output_debug_log(false, "[libfm1388-%s] trans_mapping_str is null.\n", __func__);
		return -EPARAMINVAL;
	}
	
	build_date = get_dsp_mem_value(DSP_FIRMWARE_BUILD_DATE);
	
	//translate channel mapping string
	len = strlen(channel_mapping);
	j = 0;
	memset(trans_mapping_str, '-', DSP_SPI_REC_CH_NUM);
	trans_mapping_str[DSP_SPI_REC_CH_NUM] = 0;
	while(j < len) {
		if((channel_mapping[j] >= '0') && (channel_mapping[j] <= '9')) {
			for(i = 0; i < MAX_MAP_CH_NUM; i++) {
				if(build_date >= 0x20180426) {
					if(strncasecmp(channel_mapping + j + 1, valid_channels_0426[i], 2) == 0) break;
				}
				else {
					if(strncasecmp(channel_mapping + j + 1, valid_channels[i], 2) == 0)	break;
				}
			}
			if(i == MAX_MAP_CH_NUM) break;
			
			trans_mapping_str[channel_mapping[j] - '0'] = i + '0';
		}
		else break;
		
		j += 3;
	}

	if(j < len) {
		printf("channel mapping string format is wrong.\n");
		output_debug_log(false, "[libfm1388-%s] channel mapping string(%s) format is wrong.\n", __func__, channel_mapping);
		return -EPARAMINVAL;
	}
	
	return ESUCCESS;
}

int copy_playback_result(dev_cmd_short* local_dev_cmd, dev_cmd_playback_result *playback_result) 
{
	dev_cmd_playback_result* temp_result;
	
	temp_result = (dev_cmd_playback_result*)local_dev_cmd;
	output_debug_log(false, "[libfm1388-%s] error frame number:%d among total frame:%d.\n", 
				__func__, temp_result->error_frame_number, temp_result->total_frame_number);
	if(temp_result->error_frame_number != 0) {
		output_debug_log(false, "[libfm1388-%s] first error occurs at the %dth frame.\n", 
						__func__, temp_result->first_error_frame_counter);
		output_debug_log(false, "[libfm1388-%s] last error occurs at the %dth frame.\n", 
						__func__, temp_result->last_error_frame_counter);
	}
	
	if(playback_result != NULL) {
		playback_result->first_error_frame_counter = temp_result->first_error_frame_counter;
		playback_result->last_error_frame_counter = temp_result->last_error_frame_counter;
		playback_result->error_frame_number = temp_result->error_frame_number;
		playback_result->total_frame_number = temp_result->total_frame_number;
	}

	return 0;	
}


//start playback
//channel_mapping likes 1M02M23AL4AR
int start_spi_playback(char* channel_mapping, char* filepath)
{
	int ret_val = ESUCCESS;
	dev_cmd_spi_play local_dev_cmd;
	char trans_ch_mapping[DSP_SPI_REC_CH_NUM + 1];
	
	if(dev_fd == -1) {
		output_debug_log(false, "[libfm1388-%s] device handle is -1, please make sure you have opened device correcly.\n", __func__);
		return -ENOTOPEN;
	}
	
	if(filepath == NULL) {
		output_debug_log(false, "[libfm1388-%s] got empty file path.\n", __func__);
		return -EPARAMINVAL;
	}
	if(channel_mapping == NULL) {
		output_debug_log(false, "[libfm1388-%s] channel_mapping is null.\n", __func__);
		return -EPARAMINVAL;
	}

/* Originally, I want to check the channel number and 
 * channel mapping string validation in here for both app and apk
 * run app in linux console	or adb console, it is ok. 
 * but when library is calling by HAL from APK, the file can not be opened
 * because it is in android space, the path /sdcard/ is not valid, so the file
 * under path /sdcard/ can not be opened.
 * So the channel number and channel mapping string checking work should be done in app and apk seperately.
 * Otherwise, APK and APP should pass its SDCARD path as parameter to this function. such as:
 * in APP, it is /sdcard/ or /mnt/sdcard/
 * in APK, it is /storage/emulated/0/ or /mnt/sdcard/
 */
/*
	ch_num = 0;
	ch_num = get_channel_number(filepath);
	if(ch_num <= 0) {
		printf("   Fail to parse the wav file you provided.\n");
		return ch_num;
	}

	if(!check_channel_mapping(ch_num, channel_mapping)) { //contains invalid character
		printf("   Channel Mapping is invalid.\n");
		return -EPARAMINVAL;
	} 
*/
	local_dev_cmd.cmd_name 		= FM_SMVD_DSP_PLAYBACK_START;
	strncpy(local_dev_cmd.file_path, filepath, MAX_PATH_LEN);
	
	//translate channel mapping string
	ret_val = trans_mapping_str(channel_mapping, trans_ch_mapping);
	if(ret_val != ESUCCESS) {
		output_debug_log(false, "[libfm1388-%s] Failed to translate channel mapping string(%s). ret=%d\n", __func__, channel_mapping, ret_val);
		return ret_val;
	}
	
/*
	len = strlen(channel_mapping);
	j = 0;
	memset(trans_ch_mapping, '-', DSP_SPI_REC_CH_NUM);
	trans_ch_mapping[DSP_SPI_REC_CH_NUM] = 0;
	while(j < len) {
		if((channel_mapping[j] >= '0') && (channel_mapping[j] <= '9')) {
			for(i = 0; i < MAX_MAP_CH_NUM; i++) {
				if(strncasecmp(channel_mapping + j + 1, valid_channels[i], 2) == 0)	break;
			}
			if(i == MAX_MAP_CH_NUM) break;
			
			trans_ch_mapping[channel_mapping[j] - '0'] = i + '0';
		}
		else break;
		
		j += 3;
	}
	
	if(j < len) {
		printf("channel mapping string format is wrong.\n");
		return -EPARAMINVAL;
	}
*/

	
printf("channel mapping string is translated to: %s\n", trans_ch_mapping);
	output_debug_log(false, "[libfm1388-%s] channel mapping string is translated to: %s\n", __func__, trans_ch_mapping);
	memset(local_dev_cmd.channel_mapping, 0, sizeof(char) * (DSP_SPI_REC_CH_NUM + 1));
	strncpy(local_dev_cmd.channel_mapping, trans_ch_mapping, DSP_SPI_REC_CH_NUM);
	
	local_dev_cmd.need_recording = 0;
	
	output_debug_log(false, "[libfm1388-%s] Will start SPI Playback with:\n", __func__);
	output_debug_log(false, "[libfm1388-%s] \tlocal_dev_cmd.file_path=%s\n", __func__, local_dev_cmd.file_path);
	output_debug_log(false, "[libfm1388-%s] \tlocal_dev_cmd.channel_mapping=%s\n", __func__, local_dev_cmd.channel_mapping);
	output_debug_log(false, "[libfm1388-%s] \tlocal_dev_cmd.need_recording=%d\n", __func__, local_dev_cmd.need_recording);
	

	ret_val = write(dev_fd, &local_dev_cmd, sizeof(dev_cmd_spi_play));
	if(ret_val < 0) {
		printf("error occurs when start playback. error code=%d\n", ret_val);
		output_debug_log(false, "[libfm1388-%s] error occurs when start playback. error code=%d\n", __func__, ret_val);
		return ret_val;
	}
	
	return ESUCCESS;
}

int start_spi_playback_by_ADBTool(char* channel_mapping, char* filepath, unsigned char cPlayMode, unsigned char cPlayOutput) 
{
	output_debug_log(false, "[libfm1388-%s] Will start SPI Playback from ADB Tool.\n", __func__);
	return start_spi_playback(channel_mapping, filepath);
}

int stop_spi_playback(dev_cmd_playback_result *playback_result) 
{
	dev_cmd_short local_dev_cmd;
	int ret_val = ESUCCESS;
	
	if(dev_fd == -1) {
		output_debug_log(false, "[libfm1388-%s] device handle is -1, please make sure you have opened device correcly.\n", __func__);
		return -ENOTOPEN;
	}
	
	if(playback_result == NULL) {
		output_debug_log(false, "[libfm1388-%s] got empty playback result pointer.\n", __func__);
		return -EPARAMINVAL;
	}

	output_debug_log(false, "[libfm1388-%s] Will stop SPI Playback.\n", __func__);
	local_dev_cmd.cmd_name = FM_SMVD_DSP_PLAYBACK_STOP;
	ret_val = write(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short));
	if(ret_val < 0) {
		printf("Error occurs when stop playback.error code=%d\n", ret_val);
		output_debug_log(false, "[libfm1388-%s] Error occurs when stop playback.error code=%d\n", __func__, ret_val);
		return ret_val;
	}

	copy_playback_result(&local_dev_cmd, playback_result);
	
	return ESUCCESS;
}

int start_spi_playback_rec(char* channel_mapping, char* filepath, char need_recording, unsigned int rec_ch_num, unsigned char* rec_ch_idx, char* rec_filepath) {
	int ret_val = ESUCCESS;
	dev_cmd_spi_play local_dev_cmd;
	char trans_ch_mapping[DSP_SPI_REC_CH_NUM + 1];
	char data_file_path[MAX_PATH_LEN] = { 0 };
	
	if(dev_fd == -1) {
		output_debug_log(false, "[libfm1388-%s] device handle is -1, please make sure you have opened device correcly.\n", __func__);
		return -ENOTOPEN;
	}
	
	if(filepath == NULL) {
		output_debug_log(false, "[libfm1388-%s] got empty file path.\n", __func__);
		return -EPARAMINVAL;
	}
	if(channel_mapping == NULL) {
		output_debug_log(false, "[libfm1388-%s] channel_mapping is null.\n", __func__);
		return -EPARAMINVAL;
	}
	
	if((need_recording == 1) && (rec_filepath == NULL)) {
		output_debug_log(false, "[libfm1388-%s] require to recording, but not provide data file path.\n", __func__);
		return -EPARAMINVAL;
	}

	local_dev_cmd.cmd_name 		= FM_SMVD_DSP_PLAYBACK_START;
	strncpy(local_dev_cmd.file_path, filepath, MAX_PATH_LEN);
	
	//translate channel mapping string
	ret_val = trans_mapping_str(channel_mapping, trans_ch_mapping);
	if(ret_val != ESUCCESS) {
		output_debug_log(false, "[libfm1388-%s] Failed to translate channel mapping string(%s).\n", __func__, channel_mapping);
		return ret_val;
	}
	
	//printf("channel mapping string is translated to: %s\n", trans_ch_mapping);
	output_debug_log(false, "[libfm1388-%s] channel mapping string is translated to: %s\n", __func__, trans_ch_mapping);
	memset(local_dev_cmd.channel_mapping, 0, sizeof(char) * (DSP_SPI_REC_CH_NUM + 1));
	strncpy(local_dev_cmd.channel_mapping, trans_ch_mapping, DSP_SPI_REC_CH_NUM);

	local_dev_cmd.need_recording = need_recording;

	if(need_recording == 1) {	
		snprintf(data_file_path, MAX_PATH_LEN, "%s%s", get_sdcard_path(), VOICE_DATA_FILE_NAME);
		unlink(data_file_path);

		ret_val = convert_channel_index(rec_ch_num, rec_ch_idx, &(local_dev_cmd.rec_ch_num), local_dev_cmd.rec_ch_idx);
		if(ret_val != ESUCCESS) {
			output_debug_log(false, "[libfm1388-%s] Failed to convert recording channel string(%s).\n", __func__, rec_ch_idx);
			return ret_val;
		}

		//printf("convert_channel_index string is: %s\n", local_dev_cmd.rec_ch_idx);
		
		frame_size = get_dsp_mem_value_spi(DSP_SPI_FRAMESIZE_ADDR) & 0x0000FFFF;
		if((frame_size <= 0) || (frame_size > 0x1000)) { //suppose frame size should not be too large
			printf("framesize is not correct. frame_size=%d\n", frame_size);
			output_debug_log(false, "[libfm1388-%s] framesize is not correct. frame_size=%d\n", __func__, frame_size);
			return -EDATAINVAL;
		}

		rec_channel_num = rec_ch_num;
	}	

	output_debug_log(false, "[libfm1388-%s] Will start SPI Playback and recording with:\n", __func__);
	output_debug_log(false, "[libfm1388-%s] \tlocal_dev_cmd.file_path=%s\n", __func__, local_dev_cmd.file_path);
	output_debug_log(false, "[libfm1388-%s] \tlocal_dev_cmd.channel_mapping=%s\n", __func__, local_dev_cmd.channel_mapping);
	output_debug_log(false, "[libfm1388-%s] \tlocal_dev_cmd.need_recording=%d\n", __func__, local_dev_cmd.need_recording);
	output_debug_log(false, "[libfm1388-%s] \tlocal_dev_cmd.rec_ch_num=%d\n", __func__, local_dev_cmd.rec_ch_num);
	output_debug_log(false, "[libfm1388-%s] \tlocal_dev_cmd.rec_ch_idx=%s\n", __func__, local_dev_cmd.rec_ch_idx);
	
	ret_val = write(dev_fd, &local_dev_cmd, sizeof(dev_cmd_spi_play));
	if(ret_val < 0) {
		printf("error occurs when start playback. error code=%d\n", ret_val);
		output_debug_log(false, "[libfm1388-%s] error occurs when start playback. error code=%d\n", __func__, ret_val);
		return ret_val;
	}
	
	return ESUCCESS;
}

int start_spi_playback_rec_by_ADBTool(char* channel_mapping, char* filepath, char need_recording, 
					unsigned int rec_ch_num, unsigned char* rec_ch_idx, char* rec_filepath,
					unsigned char cPlayMode, unsigned char cPlayOutput) {
	output_debug_log(false, "[libfm1388-%s] Will start SPI Playback & Recording from ADB Tool\n", __func__);
	return start_spi_playback_rec(channel_mapping, filepath, need_recording, rec_ch_num, rec_ch_idx, rec_filepath);
}

int stop_spi_playback_rec(const char* wav_file_name, const unsigned char* channels, dev_cmd_playback_result *playback_result) {
	dev_cmd_short local_dev_cmd;
	dev_cmd_playback_result* tmp_result;
	int ret_val = ESUCCESS;
	int cur_sample_rate = -1;
	char data_file_path[MAX_PATH_LEN] = { 0 };
	char wav_file_full_path[MAX_PATH_LEN] = { 0 };
	int rec_ch_num = 0, i;
	unsigned int build_date = 0;
	
	if(dev_fd == -1) {
		output_debug_log(false, "[libfm1388-%s] device handle is -1, please make sure you have opened device correcly.\n", __func__);
		return -ENOTOPEN;
	}
	
	if(playback_result == NULL) {
		output_debug_log(false, "[libfm1388-%s] got empty playback result pointer.\n", __func__);
		return -EPARAMINVAL;
	}
	
	if(wav_file_name == NULL) {
		output_debug_log(false, "[libfm1388-%s] wav_file_name is null.\n", __func__);
		return -EPARAMINVAL;
	}
	
	if(channels == NULL) {
		output_debug_log(false, "[libfm1388-%s] channels is null.\n", __func__);
		return -EPARAMINVAL;
	}
	
	output_debug_log(false, "[libfm1388-%s] Will stop SPI Playback and recording.\n", __func__);

	local_dev_cmd.cmd_name = FM_SMVD_DSP_PLAYBACK_STOP;
	ret_val = write(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short));
	if(ret_val < 0) {
		printf("Error occurs when stop playback.error code=%d\n", ret_val);
		output_debug_log(false, "[libfm1388-%s] Error occurs when stop playback.error code=%d\n", __func__, ret_val);
		return ret_val;
	}

	snprintf(data_file_path, MAX_PATH_LEN, "%s%s", get_sdcard_path(), VOICE_DATA_FILE_NAME);
	
	//get sample rate from DSP and hardcode bits_per_sample as 16
	cur_sample_rate = get_dsp_mem_value(DSP_SAMPLE_RATE_ADDR);
	if(cur_sample_rate < 0) {
		printf("got invalid sample rate data:%x\n", cur_sample_rate);
		output_debug_log(false, "[libfm1388-%s] got invalid sample rate data:%x\n", __func__, cur_sample_rate);
		return -EDATAINVAL;
	}
	
	cur_sample_rate = get_dsp_sample_rate(cur_sample_rate);
	if(cur_sample_rate < 0) { 
		printf("sample_rate value is not valid:%d.\n", cur_sample_rate);
		output_debug_log(false, "[libfm1388-%s] sample_rate value is not valid:%d.\n", __func__, cur_sample_rate);
		return -EDATAINVAL;
	}

	frame_size = get_dsp_mem_value_spi(DSP_SPI_FRAMESIZE_ADDR) & 0x0000FFFF;
	if((frame_size <= 0) || (frame_size > 0x1000)) { //suppose frame size should not be too large
		printf("framesize is not correct. frame_size=%d\n", frame_size);
		output_debug_log(false, "[libfm1388-%s] framesize is not correct. frame_size=%d\n", __func__, frame_size);
		return -EDATAINVAL;
	}

	rec_ch_num = 0;
	for(i = 0; i < DSP_SPI_REC_CH_NUM; i++) {
		if(channels[i] == '1') rec_ch_num ++;
	}

	snprintf(wav_file_full_path, MAX_PATH_LEN, "%s%s", get_sdcard_path(), wav_file_name);
	unlink(wav_file_full_path);
   
	usleep(5000); //wait for driver stop the recording
printf("voice data file path:%s, wav file path:%s\n", data_file_path, wav_file_full_path);

	output_debug_log(false, "[libfm1388-%s] Will convert wav file from SPI Playback and recording:\n", __func__);
	output_debug_log(false, "[libfm1388-%s] \tdata_file_path=%s\n", __func__, data_file_path);
	output_debug_log(false, "[libfm1388-%s] \twav_file_full_path=%s\n", __func__, wav_file_full_path);
	output_debug_log(false, "[libfm1388-%s] \tchannels=%s\n", __func__, channels);
	output_debug_log(false, "[libfm1388-%s] \trec_ch_num=%d\n", __func__, rec_ch_num);
	output_debug_log(false, "[libfm1388-%s] \tcur_sample_rate=%d\n", __func__, cur_sample_rate);
	output_debug_log(false, "[libfm1388-%s] \trame_size=%d\n", __func__, frame_size);

	build_date = get_dsp_mem_value(DSP_FIRMWARE_BUILD_DATE);
	ret_val = convert_data(data_file_path, wav_file_full_path, cur_sample_rate, 16, 
						rec_ch_num, channels, frame_size, build_date >= 0x20180426);

	if(ret_val) {
		printf("Error occurs when save recording data. error code=%d\n", ret_val);
		output_debug_log(false, "[libfm1388-%s] Error occurs when save recording data. error code=%d\n", __func__, ret_val);
		return ret_val;
	}

	copy_playback_result(&local_dev_cmd, playback_result);
	
	return ESUCCESS;
}

int is_playing() {
	dev_cmd_short local_dev_cmd;
	int ret_val = ESUCCESS;
	
	if(dev_fd == -1) {
		output_debug_log(false, "[libfm1388-%s] device handle is -1, please make sure you have opened device correcly.\n", __func__);
		return -ENOTOPEN;
	}
	
	local_dev_cmd.cmd_name = FM_SMVD_DSP_IS_PLAYING;
	ret_val = read(dev_fd, &local_dev_cmd, sizeof(dev_cmd_short));
	if(ret_val < 0) {
		printf("Error occurs when check playing status. error code=%d\n", ret_val);
		output_debug_log(false, "[libfm1388-%s] Error occurs when check playing status. error code=%d\n", __func__, ret_val);
		return ret_val;
	}

	output_debug_log(false, "[libfm1388-%s] Query playback status, ret=%d\n", __func__, local_dev_cmd.val);

	return local_dev_cmd.val;
}


