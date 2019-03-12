/*
 * external/fm_fm1388/fm_fm1388.c
 * (C) Copyright 2014-2018
 * Fortemedia, Inc. <www.fortemedia.com>
 * Author: HenryZhang <henryhzhang@fortemedia.com>;
 * 			LiFu <fuli@fortemedia.com>
 *
 * This program is a console application which can communicate with FM1388 driver.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>
#include <pthread.h>
//#include <linux/delay.h>
#include <signal.h>
#include <time.h>
#include <sys/statfs.h>
#include "fm_fm1388.h"

static int vbuf_read_flag = 0;
static int vbuf_write_flag = 0;

bool contain_invalid_char(char* channels) {
	int j 	= 0;
	int len = 0;
	
	if(channels == NULL) return true;
	
	len = strlen(channels);
	if((len == 1) && ((channels[0] == 'a') || (channels[0] == 'A'))) {
		return false;
	}
	
	for(j = 0; j < len; j++) {
		if((channels[j] < '0') || (channels[j] > '9'))
			break;
	}
	if(j != len) { return true; } //contains invalid character
	
	return false;
}

int get_audio_conf(rec_param *p_rec_param)
{
	char 			play_file_full_path[MAX_PATH_LEN + 1] 	= {0};
	char 			rec_file[MAX_PATH_LEN + 1] 	= {0};
	char 			rec_chs[MAX_PATH_LEN + 1] 	= {0};
	int  			ch_num = 0, i, j;
	FILE*			temp_fp						= NULL;
	unsigned int 	build_date = 0;

	do {
		printf(" - Please specify the wav file name(like: record.wav):");
		i = scanf("%127s", rec_file);
		if(!i) {
			printf("   The wav file name is invalid\n");
		}
		else {
			snprintf(play_file_full_path, MAX_PATH_LEN, "%s%s", get_sdcard_path(), rec_file);
			temp_fp = fopen(play_file_full_path, "wb");
			if(temp_fp) {
				fclose(temp_fp);
			}
			else {
				printf("   The wav file can not be accessed\n");
				i = 0;
			}
		}
	} while (!i);
	printf("   The wav file path is: %s\n\n", rec_file);
	output_debug_log(false, "%s: The wav file path is: %s\n", __func__, rec_file);
	strcpy(p_rec_param->rec_file, rec_file);

	printf(" - Please select the channels for recording.\n");
	build_date = get_dsp_mem_value(DSP_FIRMWARE_BUILD_DATE);
	if(build_date >= 0x20180426) {
		printf("   0 - MIC0, 1 - MIC1, 2 - MIC2,  3 - AECREFL, 4 - AECREFR,\n   5 - LINL, 6 - LINR, 7 - LOUTL, 8 - LOUTR,   9 - DBG_INFO, a - ALL\n");
	}
	else {
		printf("   0 - MIC0, 1 - MIC1, 2 - MIC2, 3 - MIC3, 4 - AECREFL, 5 - AECREFR,\n   6 - LIN, 7 - LOUT, 8 - SPKOUT, 9 - DBG_INFO, a - ALL\n");
	}
	printf("   No SEPARATORS between digits\n");
	printf("   ex. 0123 - channels 0, 1, 2 and 3 are selected.\n");
	printf("       a   - all of the channels are selected.\n");
	printf("   The channels you will choose are: ");
	i = scanf(" %127s", rec_chs);
	if(i) {
		if(contain_invalid_char(rec_chs)) { i = 0; } //contains invalid character
	}
	
	while (!i) {
		printf("   The channel index number is invalid.\n");
		printf("   The channels you will choose are: ");
		i = scanf(" %127s", rec_chs);
		if(i) {
			if(contain_invalid_char(rec_chs)) { i = 0; } //contains invalid character
		}
	}
	rec_chs[DSP_SPI_REC_CH_NUM] = 0;
	printf("   The selected channels are: %10s\n", rec_chs);
	output_debug_log(false, "%s: The selected channels are: %10s\n", __func__, rec_chs);

	if (rec_chs[0] == 'a') {
		ch_num = DSP_SPI_REC_CH_NUM;
		for (i = 0; i < DSP_SPI_REC_CH_NUM; i++) {
			rec_chs[i] = 0x30 + i;
		}
	}
	else {
		for (i = 0; i < DSP_SPI_REC_CH_NUM; i++) {
			if (rec_chs[i]) {
				ch_num++;
			}
		}
	}
	p_rec_param->ch_num = ch_num;
	memset(p_rec_param->ch_idx, 0, sizeof(unsigned char) * DSP_SPI_REC_CH_NUM);
	for (i = 0; i < ch_num; i++) {
		p_rec_param->ch_idx[rec_chs[i] - 0x30] = '1';
	}

	return 0;
}

int get_playback_conf(play_param *p_playback_param)
{
	char 			play_file[MAX_PATH_LEN + 1] 			= {0};
	char 			play_file_full_path[MAX_PATH_LEN + 1] 	= {0};
	char 			channel_mapping[MAX_PATH_LEN + 1] 		= {0};
	int  			ch_num = 0, i, j;
	FILE*			temp_fp						= NULL;
	rec_param 		local_rec_param;
	char			need_recording[MAX_PATH_LEN + 1]		= {0};
	unsigned int 	build_date = 0;
	
	if(p_playback_param == NULL) {
		printf(" - Invalid parameter data structure.\n");
		output_debug_log(false, "%s: Invalid parameter data structure.\n", __func__);
	}
	
	do {
		printf(" - Please specify the wav file name(like: debug.wav):");
		i = scanf("%127s", play_file);
		if(!i) {
			printf("   The wav file name is invalid\n");
			output_debug_log(false, "%s: The wav file name is invalid\n", __func__);
		}
		else {
			snprintf(play_file_full_path, MAX_PATH_LEN, "%s%s", get_sdcard_path(), play_file);
			temp_fp = fopen(play_file_full_path, "rb");
			if(temp_fp) {
				fclose(temp_fp);
			}
			else {
				printf("   The wav file can not be accessed.\n");
				output_debug_log(false, "%s: The wav file can not be accessed.\n", __func__);
				i = 0;
			}

			ch_num = 0;
			ch_num = get_channel_number(play_file);
			if(ch_num <= 0) {
				printf("   Fail to parse the wav file you provided.\n");
				output_debug_log(false, "%s: Fail to parse the wav file you provided.\n", __func__);
				i = 0;
			}
		}
	} while (!i);
	printf("   The wav file path is: %s\n\n", play_file_full_path);
	output_debug_log(false, "%s: The wav file path is: %s\n", __func__, play_file_full_path);
	strncpy(p_playback_param->wav_file, play_file, MAX_PATH_LEN);

	
	printf(" - Please provide the channel mapping string.\n");
	build_date = get_dsp_mem_value(DSP_FIRMWARE_BUILD_DATE);
	if(build_date >= 0x20180426) {
		printf("   M0 - MIC0, M1 - MIC1, M2 - MIC2, RL - AECREFL, RR - AECREFR\n");
	}
	else {
		printf("   M0 - MIC0, M1 - MIC1, M2 - MIC2, M3 - MIC3, RL - AECREFL, RR - AECREFR\n");
	}
	printf("   No SEPARATORS between digits\n");
	printf("   ex. 0M02M14RL5RR - use the first channel as Mic0, the 3rd channel as Mic1\n");
	printf("                      use the 5th channel as left echo reference\n");
	printf("                      use the 6th channel as right echo reference.\n");
	printf("   Now, Please provide your channel mapping: ");
	i = scanf(" %127s", channel_mapping);
	if(i) {
		if(!check_channel_mapping(ch_num, channel_mapping)) { i = 0; } //contains invalid character
	}
	
	while (!i) {
		printf("   Your channel mapping string is invalid.\n");
		printf("   Please provide your channel mapping: ");
		i = scanf(" %127s", channel_mapping);
		if(i) {
			if(!check_channel_mapping(ch_num, channel_mapping)) { i = 0; } //contains invalid character
		}
	}
	
	printf("   The selected channel mapping is: %s\n", channel_mapping);
	output_debug_log(false, "%s: The selected channel mapping is: %s\n", __func__, channel_mapping);

	strncpy(p_playback_param->channel_mapping, channel_mapping, MAX_MAPPING_STR_LEN);


	//Recording
	printf(" - Do you want to record during playback?(y/n)\n");
	i = scanf(" %127s", need_recording);
	if(i) {
		if((strlen(need_recording) != 1) || 
			((need_recording[0] != 'n') && (need_recording[0] != 'N') &&
			(need_recording[0] != 'y') && (need_recording[0] != 'Y'))) { //contains invalid character
			i = 0; 
		} 
	}
	
	while (!i) {
		printf(" - Please give clear answer.\n");
		printf(" - Do you want to record during playback?(y/n)\n");
		i = scanf(" %127s", need_recording);
		if(i) {
			if((strlen(need_recording) != 1) || 
				((need_recording[0] != 'n') && (need_recording[0] != 'N') &&
				(need_recording[0] != 'y') && (need_recording[0] != 'Y'))) { //contains invalid character
				i = 0; 
			} 
		}
	}
	
	p_playback_param->need_recording = need_recording[0];
	
	if((need_recording[0] == 'n') || (need_recording[0] == 'N')) {
		p_playback_param->need_recording = 0;
	}
	else if((need_recording[0] == 'y') || (need_recording[0] == 'Y')) {
		p_playback_param->need_recording = 1;
	}
	else {
		p_playback_param->need_recording = 0;
	}
	
	if(p_playback_param->need_recording == 1) {
		//get recording parameter
		get_audio_conf(&local_rec_param);
		
		strncpy(p_playback_param->rec_file, local_rec_param.rec_file, MAX_PATH_LEN);
		p_playback_param->rec_ch_num = local_rec_param.ch_num;
		memset(p_playback_param->rec_ch_idx, 0, sizeof(unsigned char) * DSP_SPI_REC_CH_NUM);
		for (i = 0; i < DSP_SPI_REC_CH_NUM; i++) {
			p_playback_param->rec_ch_idx[i] = local_rec_param.ch_idx[i];
		}
	}
	
	return 0;
}

void stop_recording(rec_param* rec_param_data, dev_cmd_record_result *rec_result) {
	if((vbuf_read_flag == 1)) {
		stop_debug_record(rec_param_data->rec_file, rec_param_data->ch_idx, rec_result);
		vbuf_read_flag = 0;
	}
}

void stop_playback(play_param* playback_param_data, dev_cmd_playback_result *playback_result) {
	if((vbuf_write_flag == 1)) {
		if(playback_param_data->need_recording == 1) {
			stop_spi_playback_rec(playback_param_data->rec_file, playback_param_data->rec_ch_idx, playback_result);
		}
		else {
			stop_spi_playback(playback_result);
		}
		vbuf_write_flag = 0;
	}
}

int get_free_disk_space(char* str_path) {
    struct statfs diskInfo; 
      
    statfs(str_path, &diskInfo); 
    unsigned long long blocksize = diskInfo.f_bsize; //每个block里包含的字节数 
    unsigned long long totalsize = blocksize * diskInfo.f_blocks; //总的字节数，f_blocks为block的数目 
      
    unsigned long long freeDisk = diskInfo.f_bfree * blocksize; //剩余空间的大小 
    unsigned long long availableDisk = diskInfo.f_bavail * blocksize; //可用空间大小 
    
    return availableDisk >> 10;
}

int main(int argc, char** argv)  
{  
	support_mode	mode_info;
	rec_param 		local_rec_param;
	play_param	 	local_playback_param;
	
	int  err_code 	= 0;
	int  i 			= 0;
	int  iAction 	= 0;
	int  dsp_mode 	= 0;
	bool loop 		= true;
	int  mode, playback_status;
	unsigned int    reg, choosed_mode, matched = 0;
	unsigned int  	reg_val, addr;
	dev_cmd_record_result rec_result;
	dev_cmd_playback_result playback_result;
	
	err_code = lib_open_device();
	if(err_code < 0) {
		printf("   Failed to open device..\n");
		return err_code;
	}

	output_debug_log(false, "[fm_fm1388--%s]Enter fm_fm1388\n", __func__);
    //Feature: 1228
    //1. open cfg file and parse the file to get the available mode number
    //2. in appliation's menu prompt user with above available modes
    if(load_fm1388_mode_cfg(get_cfg_location(), &mode_info) != ESUCCESS) {
        printf("   Failed to open and parse cfg. %s\n", get_cfg_location());
        output_debug_log(false, "[fm_fm1388--%s]Failed to open and parse cfg. %s\n", __func__, get_cfg_location());
        return -EFAILOPEN;        
    }

	memset(&local_rec_param, 0, sizeof(rec_param));
	while (loop)
	{
		err_code = 0;
		printf("\n\nSupported modes:\n");
		mode = get_mode();
		
		output_debug_log(true, "current mode=%d\n", mode);
		for(i = 0; i < mode_info.number; i++) {
			if(mode == mode_info.mode[i])
				printf("    %2x...%-s (current used)\n", mode_info.mode[i], mode_info.comment[i]);
			else 
				printf("    %2x...%-s\n", mode_info.mode[i], mode_info.comment[i]);
		}
		printf("\n");
		
		//Output the help messages here
		fm_smvd_help_display();

		iAction = fm_keyin2action();
		switch (iAction)
		{
			case GET_DSP_MODE:
				mode = get_mode();
				if(mode < 0) {
					printf(" - Error occurs when get DSP mode. errorcode=%d\n", mode);
					output_debug_log(false, "[fm_fm1388--%s]Error occurs when get DSP mode. errorcode=%d\n", __func__, mode);
				}
				else {
					printf(" - The current dsp_mode = %x\n", mode);
					output_debug_log(false, "[fm_fm1388--%s]The current dsp_mode = %x\n", __func__, mode);
				}
				break;

			case SET_DSP_MODE:
				mode = get_mode();
				if(mode < 0) {
					printf(" - Error occurs when get DSP mode. errorcode=%d\n", mode);
					output_debug_log(false, "[fm_fm1388--%s]Error occurs when get DSP mode. errorcode=%d\n", __func__, mode);
				}
				else {
					printf(" - The current dsp_mode = %x\n", mode);
					printf(" - Please input new mode index(1 hex-digit): ");
					scanf("%127x", &choosed_mode);
					if(mode_info.number) {
						matched = 0;
						for(i = 0; i < mode_info.number; i++) {
							if(choosed_mode == mode_info.mode[i]) {
								err_code = set_mode(choosed_mode);
								if(err_code != ESUCCESS) {
									printf(" - Error occurs when set DSP mode. errorcode=%d\n", err_code);
									output_debug_log(false, "[fm_fm1388--%s]Error occurs when set DSP mode. errorcode=%d\n", __func__, err_code);
									break;
								}
								else {
									matched = 1;
									dsp_mode = get_mode();
									printf(" - The current dsp_mode = %x.\n", dsp_mode);
									output_debug_log(false, "[fm_fm1388--%s]The current dsp_mode = %x.\n", __func__, dsp_mode);
									break;
								}
							}
							else{
								matched =0;
							}
						}
					}
					else{
						if((choosed_mode!=FM_SMVD_CFG_BARGE_IN) &&
							(choosed_mode!=FM_SMVD_CFG_VR) &&
							(choosed_mode!=FM_SMVD_CFG_CM))
							matched = 0;
					}
					if(!matched) {
						printf(" - The mode %x is not supported yet.\n", choosed_mode);
						output_debug_log(false, "[fm_fm1388--%s]The mode %x is not supported yet.\n", __func__, choosed_mode);
					}
				}
				break;

			case START_VBUF_WRITE:
				if(vbuf_read_flag == 1){
					printf(" - The last debug recording is not stopped, please stop it if you want to start a new playback.\n");
					output_debug_log(false, "[fm_fm1388--%s]The last debug recording is not stopped, please stop it if you want to start a new playback.\n", __func__);
					break;
				}
				
				playback_status = is_playing();
				if(playback_status == 1) {
					printf(" - The last playback is not stopped, please stop it if you want to start a new playback.\n");
					output_debug_log(false, "[fm_fm1388--%s]The last playback is not stopped, please stop it if you want to start a new playback.\n", __func__);
					vbuf_write_flag = 0;
					break;
				}
				
				if(playback_status < 0) break;
				
				printf(" - Start SPI Playback...\n");
				output_debug_log(false, "[fm_fm1388--%s]Start SPI Playback...\n", __func__);
				get_playback_conf(&local_playback_param);
				
				//remove final wav file if it exists
				unlink(local_playback_param.rec_file);
				err_code = start_spi_playback_rec(local_playback_param.channel_mapping, local_playback_param.wav_file, 
											local_playback_param.need_recording, local_playback_param.rec_ch_num, 
											local_playback_param.rec_ch_idx, local_playback_param.rec_file);
				if(err_code != ESUCCESS) {
					printf(" - Failed to play!\n");
					output_debug_log(false, "[fm_fm1388--%s]Failed to play!\n", __func__);
					break;
				}
				
				vbuf_write_flag = 1;
				break;

			case STOP_VBUF_WRITE:
				printf(" - Stop SPI Playback.\n");
				output_debug_log(false, "[fm_fm1388--%s]Stop SPI Playback.\n", __func__);
				stop_playback(&local_playback_param, &playback_result);

				printf("loss frame number:%d among total frame:%d.\n", playback_result.error_frame_number, playback_result.total_frame_number);
				output_debug_log(false, "[fm_fm1388-%s] loss frame number:%d among total frame:%d.\n", 
							__func__, playback_result.error_frame_number, playback_result.total_frame_number);
				if(playback_result.error_frame_number != 0) {
					printf("first loss occurs at the %dth frame.\n", playback_result.first_error_frame_counter);
					output_debug_log(false, "[fm_fm1388-%s] first loss occurs at the %dth frame.\n", __func__, playback_result.first_error_frame_counter);
					printf("last loss occurs at the %dth frame.\n", playback_result.last_error_frame_counter);
					output_debug_log(false, "[fm_fm1388-%s] last loss occurs at the %dth frame.\n", __func__, playback_result.last_error_frame_counter);
				}
	
				vbuf_write_flag = 0;
				break;

			case START_VBUF_READ:
				if((vbuf_write_flag == 1) || (vbuf_read_flag == 1)){
					printf(" - The last debug recording or playback is not stopped, please stop it if you want to start a new recording.\n");
					output_debug_log(false, "[fm_fm1388--%s]The last debug recording or playback is not stopped, please stop it if you want to start a new recording.\n", __func__);
					break;
				}
				printf(" - Start debug recording...\n");
				output_debug_log(false, "[fm_fm1388--%s]Start debug recording...\n", __func__);
				if(!err_code) {
					get_audio_conf(&local_rec_param);
					
					unlink(local_rec_param.rec_file);
					err_code = start_debug_record(local_rec_param.ch_num, local_rec_param.ch_idx, local_rec_param.rec_file);
					if(err_code != ESUCCESS) {
						printf(" - Failed to record!\n");
						output_debug_log(false, "[fm_fm1388--%s]Failed to record!\n", __func__);
						break;
					}
					
					vbuf_read_flag = 1;

					//Calculate the disk space be used per minute
					//get frame size for data block calculate
					int frame_size = get_dsp_mem_value_spi(DSP_SPI_FRAMESIZE_ADDR) & 0x0000FFFF;
					if((frame_size > 0) && (frame_size < 0x1000)) { //suppose frame size should not be too large
						int block_size 		= local_rec_param.ch_num * frame_size;
						int block_time 		= (frame_size == 320) ? 10 : 8;
						int kb_per_second 	= (block_size * (1000 / block_time)) >> 10;
						
						char* str_temp = strchr(local_rec_param.rec_file, '/');
						char* str_last = NULL;
						while(str_temp != NULL) {
							str_last = str_temp;
							str_temp = strchr(str_temp + 1, '/');
						}
						
						if(str_last != NULL) {
							char str_path[MAX_PATH_LEN + 1] = { 0 };
							strncpy(str_path, local_rec_param.rec_file, str_last - local_rec_param.rec_file);
							int free_disk_kb	= get_free_disk_space(str_path);
							int seconds_avail	= free_disk_kb / kb_per_second / 2;

							printf("\n   You can record about %d seconds.\n", seconds_avail);
							output_debug_log(false, "[fm_fm1388--%s]You can record about %d seconds.\n", __func__, seconds_avail);
						}
					}
				}
				else if(err_code == -EFAILOPEN) {
					printf(" - Failed to open character device.\n");
					output_debug_log(false, "[fm_fm1388--%s]Failed to open character device.\n", __func__);
				}
				else if(err_code == -EDATAINVAL) {
					printf(" - Got invalidate data from DSP.\n");
					output_debug_log(false, "[fm_fm1388--%s]Got invalidate data from DSP.\n", __func__);
				}
				else {
					printf(" - Error occurs when get DSP data address. errorcode=%d\n", err_code);
					output_debug_log(false, "[fm_fm1388--%s]Error occurs when get DSP data address. errorcode=%d\n", __func__, err_code);
				}
				break;

			case STOP_VBUF_READ:
				printf(" - Stop debug recording.\n");
				output_debug_log(false, "[fm_fm1388--%s]Stop debug recording.\n", __func__);
				stop_recording(&local_rec_param, &rec_result);
				
				printf("loss frame number:%d among total frame:%d.\n", rec_result.error_frame_number, rec_result.total_frame_number);
				output_debug_log(false, "[fm_fm1388-%s] loss frame number:%d among total frame:%d.\n", 
							__func__, rec_result.error_frame_number, rec_result.total_frame_number);
				if(rec_result.error_frame_number != 0) {
					printf("first loss occurs at the %dth frame.\n", rec_result.first_error_frame_counter);
					output_debug_log(false, "[fm_fm1388-%s] first loss occurs at the %dth frame.\n", __func__, rec_result.first_error_frame_counter);
					printf("last loss occurs at the %dth frame.\n", rec_result.last_error_frame_counter);
					output_debug_log(false, "[fm_fm1388-%s] last loss occurs at the %dth frame.\n", __func__, rec_result.last_error_frame_counter);
				}
	
				vbuf_read_flag = 0;
				break;

			case GET_REG_VALUE:
				printf(" - Please input the reg in hex (2 digits, no need 0x): ");
				scanf("%127x", &reg);
				if(reg <= (unsigned int)0xff){
					reg_val = get_reg_value(reg);
					printf(" - reg = 0x%02x, val=0x%04x\n\n", reg, reg_val);
					output_debug_log(false, "[fm_fm1388--%s]get reg value, reg = 0x%02x, val=0x%04x\n\n", __func__, reg, reg_val);
				}
				else {
					printf(" - The reg is greater than 0xff.\n");
					output_debug_log(false, "[fm_fm1388--%s]get reg value, The reg(%x) is greater than 0xff.\n", __func__, reg);
				}
				break;

			case SET_REG_VALUE:
				printf(" - Please input the reg in hex (2 digits, no need 0x): ");
				scanf("%127x", &reg);
				printf(" - Please input the value in hex (4 digits, no need 0x): ");
				scanf("%127x", &reg_val);
				if(reg <= 0xff && reg_val <= 0xffff){
					printf(" - reg = 0x%02x, val=0x%04x\n", reg, reg_val);
					output_debug_log(false, "[fm_fm1388--%s]set reg value, reg = 0x%02x, val=0x%04x\n", __func__, reg, reg_val);
					set_reg_value(reg, reg_val);
				}
				else {
					printf(" - The reg is greater than 0xff or the value is greater than 0xffff.\n");
					output_debug_log(false, "[fm_fm1388--%s]set reg value, The reg(%x) is greater than 0xff or the value(%x) is greater than 0xffff.\n", __func__, reg, reg_val);
				}
				break;

			case GET_MEM_VALUE:
				printf(" - Please input the addr in hex (8 digits, no need 0x): ");
				scanf("%127x", &addr);
				if(addr <= (unsigned int)0xffffffff){
					reg_val = get_dsp_mem_value(addr);
					printf(" - addr = 0x%08x, val=0x%08x\n", addr, reg_val);
					output_debug_log(false, "[fm_fm1388--%s]get memory data, addr = 0x%08x, val=0x%08x\n", __func__, addr, reg_val);
				}
				else {
					printf(" - The addr is greater than 32 bits.\n");
					output_debug_log(false, "[fm_fm1388--%s]get memory data, The addr(%x) is greater than 32 bits.\n", __func__, addr);
				}
				break;

			case SET_MEM_VALUE:
				printf(" - Please input the addr in hex (8 digits, no need 0x): ");
				scanf("%127x", &addr);
				printf(" - Please input the value in hex (4 digits, no need 0x): ");
				scanf("%127x", &reg_val);
				if(addr <= (unsigned int)0xffffffff){
					printf(" - addr = 0x%08x, val=0x%04x\n", addr, reg_val);
					output_debug_log(false, "[fm_fm1388--%s]set memory data: addr = 0x%08x, val=0x%04x\n", __func__, addr, reg_val);
					set_dsp_mem_value(addr, reg_val);
				}
				else {
					printf(" - The addr is greater than 32 bits.\n");
					output_debug_log(false, "[fm_fm1388--%s]set memory data: The addr(%x) is greater than 32 bits.\n", __func__, addr);
				}
				break;

			case DISPLAY_HELP:
				fm_smvd_help_display();
				break;

			case QUIT_PROGRAM:
				stop_recording(&local_rec_param, &rec_result);
				stop_playback(&local_playback_param, &playback_result);
				vbuf_read_flag = 0;
				vbuf_write_flag = 0;
				loop = false;
				break;

			default:
				break;
		}
	}

	lib_close_device();
	
	output_debug_log(false, "[fm_fm1388--%s]Terminate fm_fm1388 normally.\n", __func__);
	return ESUCCESS;
}

int fm_smvd_help_display(void)
{
	int i;
	for (i = 0; strlen(cstr_fm_fm1388_help[i]); i++)
	{
		printf("%s\n", cstr_fm_fm1388_help[i]);
	}
	
	return ESUCCESS;
}

int fm_keyin2action(void)
{
	int  iAction = -1;
	char cKeyin[MAX_PATH_LEN];
	
//	system("echo off");
	scanf("%127s", cKeyin);
	switch (cKeyin[0])
	{
		case 'g':
			iAction = GET_DSP_MODE;
			break;

		case 's':
			iAction = SET_DSP_MODE;
			break;

		case 'c':
			iAction = START_VBUF_WRITE;
			break;

		case 'd':
			iAction = STOP_VBUF_WRITE;
			break;

		case 'e':
			iAction = START_VBUF_READ;
			break;

		case 'f':
			iAction = STOP_VBUF_READ;
			break;

		case 'r':
			iAction = GET_REG_VALUE;
			break;

		case 't':
			iAction = SET_REG_VALUE;
			break;

		case 'm':
			iAction = GET_MEM_VALUE;
			break;

		case 'p':
			iAction = SET_MEM_VALUE;
			break;

		case 'q':
			iAction = QUIT_PROGRAM;
			break;

		default:
			break;
	}
//	system("echo on");

	return (iAction);
}
