#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../libFM1388Parameter.h"
#include "../libfm1388.h"


int process_spi_operation(char* parameter_string, char* result_file_path, char* strSDCARD) {
	SPIRecord		st_spi_record;
	SPIPlay			st_spi_play;
	SPIPlayRecord	st_spi_play_record;
	RequestPara		result_info[5];
	char			cOperation 	= -1;
	int				ret			= 0;
	int				rec_ch_num	= 0;
	int				i			= 0;
	char 			error_info[COMMENT_LENGTH] = { 0 };
	dev_cmd_record_result rec_result;
	dev_cmd_playback_result playback_result;
	

	if (strSDCARD == NULL) {
		output_debug_log(true, "[%s] wrong parameter, strSDCARD is NULL.\n", __func__);
		strcpy(error_info, "strSDCARD is null");
		ret = PARAMETER_PARSE_ERROR;
		goto GEN_RESULT;
	}

	if(parameter_string == NULL) {
		output_debug_log(true, "[%s] parameter_string is null.\n", __func__);
		strcpy(error_info, "parameter_string is null");
		ret = PARAMETER_PARSE_ERROR;
		goto GEN_RESULT;
	}
	
	if(result_file_path == NULL) {
		output_debug_log(true, "[%s] result_file_path is null.\n", __func__);
		strcpy(error_info, "result_file_path is null");
		ret = PARAMETER_PARSE_ERROR;
		goto GEN_RESULT;
	}
output_debug_log(true, "[%s] parameter_string is %s.\n", __func__, parameter_string);
output_debug_log(true, "[%s] result_file_path is %s.\n", __func__, result_file_path);
	
	memset(&rec_result, 0, sizeof(dev_cmd_record_result));
	memset(&playback_result, 0, sizeof(dev_cmd_playback_result));
	
	cOperation = parameter_string[0];
	switch(cOperation) {
		case SPIPLAY:
			ret = parse_play_command(parameter_string, &st_spi_play);
			if(ret != ESUCCESS) {
				output_debug_log(true, "[%s] Fail to parse SPI playback command parameter.\n", __func__);
				ret = PARAMETER_PARSE_ERROR;
				strcpy(error_info, "Fail to parse SPI playback command parameter");
				goto GEN_RESULT;
			}
			
			if(st_spi_play.cCommand == START) {
				//call libfm1388.so to do the real work
				ret = start_spi_playback_by_ADBTool(st_spi_play.strChannelMapping, 
						st_spi_play.strInputFilePath, st_spi_play.cPlayMode, st_spi_play.cPlayOutput);
				if(ret != ESUCCESS) {
					output_debug_log(true, "[%s] Failed to start SPI playback.\n", __func__);
					ret = OPERATION_FAILED;
					strcpy(error_info, "Failed to start SPI playback");
					goto GEN_RESULT;
				}
			}
			else if(st_spi_play.cCommand == STOP) {
				//call libfm1388.so to do the real work
				ret = stop_spi_playback(&playback_result);
				if(ret != ESUCCESS) {
					output_debug_log(true, "[%s] Failed to stop SPI playback.\n", __func__);
					ret = OPERATION_FAILED;
					strcpy(error_info, "Failed to stop SPI playback");
					goto GEN_RESULT;
				}
				else {
					printf("loss frame number:%d among total frame:%d.\n", playback_result.error_frame_number, playback_result.total_frame_number);
					output_debug_log(false, "[%s] loss frame number:%d among total frame:%d.\n", 
								__func__, playback_result.error_frame_number, playback_result.total_frame_number);
				}
			}
			else if(st_spi_play.cCommand == QUERY) {
				//call libfm1388.so to do the real work
				ret = is_playing();
				if(ret < 0) {
					output_debug_log(true, "[%s] Failed to query SPI playback.\n", __func__);
					ret = OPERATION_FAILED;
					strcpy(error_info, "Failed to query SPI playback");
					goto GEN_RESULT;
				}
				else {
					if(ret == 1) {
						strcpy(error_info, "Playing");
					}
					else {
						strcpy(error_info, "Stopped");
					}
				}
			}
			else {
				output_debug_log(true, "[%s] Got unsupported SPI playback command.\n", __func__);
				ret = COMMAND_UNSUPPORTED;
				strcpy(error_info, "Got unsupported SPI playback command");
				goto GEN_RESULT;
			}
			break;
			
		case SPIRECORD:
			ret = parse_record_command(parameter_string, &st_spi_record, strSDCARD);
			if(ret != ESUCCESS) {
				output_debug_log(true, "[%s] Fail to parse SPI recording command parameter.\n", __func__);
				ret = PARAMETER_PARSE_ERROR;
				strcpy(error_info, "Fail to parse SPI recording command parameter");
				goto GEN_RESULT;
			}
			
			if(st_spi_record.cCommand == START) {
				rec_ch_num = 0;
				for(i = 0; i < DSP_SPI_CH_NUM; i++) {
					if(st_spi_record.strChannelIndex[i] == '1') rec_ch_num ++;
				}
				
				//call libfm1388.so to do the real work
				ret = start_debug_record(rec_ch_num, st_spi_record.strChannelIndex, st_spi_record.strOutputFilePath);
				if(ret != ESUCCESS) {
					output_debug_log(true, "[%s] Failed to start SPI recording.\n", __func__);
					ret = OPERATION_FAILED;
					strcpy(error_info, "Failed to start SPI recording");
					goto GEN_RESULT;
				}
			}
			else if(st_spi_record.cCommand == STOP) {
				//call libfm1388.so to do the real work
				rec_ch_num = 0;
				for(i = 0; i < DSP_SPI_CH_NUM; i++) {
					if(st_spi_record.strChannelIndex[i] == '1') rec_ch_num ++;
				}

				ret = stop_debug_record_by_ADBTool(st_spi_record.strOutputFilePath, st_spi_record.strChannelIndex, rec_ch_num, &rec_result);
				if(ret != ESUCCESS) {
					output_debug_log(true, "[%s] Failed to stop SPI recording.\n", __func__);
					ret = OPERATION_FAILED;
					strcpy(error_info, "Failed to stop SPI recording");
					goto GEN_RESULT;
				}
				else {
					printf("loss frame number:%d among total frame:%d.\n", rec_result.error_frame_number, rec_result.total_frame_number);
					output_debug_log(false, "[%s] loss frame number:%d among total frame:%d.\n", 
								__func__, rec_result.error_frame_number, rec_result.total_frame_number);
				}
			}
			else {
				output_debug_log(true, "[%s] Got unsupported SPI recording command.\n", __func__);
				ret = COMMAND_UNSUPPORTED;
				strcpy(error_info, "Got unsupported SPI recording command");
				goto GEN_RESULT;
			}
			break;
			
		case SPIPLAYRECORD:
			ret = parse_play_record_command(parameter_string, &st_spi_play_record, strSDCARD);
			if(ret != ESUCCESS) {
				output_debug_log(true, "[%s] Fail to parse SPI playback & recording command parameter.\n", __func__);
				ret = PARAMETER_PARSE_ERROR;
				strcpy(error_info, "Fail to parse SPI playback & recording command parameter");
				goto GEN_RESULT;
			}

			if(st_spi_play_record.cCommand == START) {
				rec_ch_num = 0;
				for(i = 0; i < DSP_SPI_CH_NUM; i++) {
					if(st_spi_play_record.strChannelIndex[i] == '1') rec_ch_num ++;
				}
				
				//call libfm1388.so to do the real work
				ret = start_spi_playback_rec_by_ADBTool(st_spi_play_record.strChannelMapping, 
								st_spi_play_record.strInputFilePath, 1, rec_ch_num, 
								st_spi_play_record.strChannelIndex, 
								st_spi_play_record.strOutputFilePath,
								st_spi_play.cPlayMode, st_spi_play.cPlayOutput);
				if(ret != ESUCCESS) {
					output_debug_log(true, "[%s] Failed to start SPI playback & recording.\n", __func__);
					ret = OPERATION_FAILED;
					strcpy(error_info, "Failed to start SPI playback & recording");
					goto GEN_RESULT;
				}
			}
			else if(st_spi_play_record.cCommand == STOP) {
				//call libfm1388.so to do the real work
				ret = stop_spi_playback_rec(st_spi_play_record.strOutputFilePath, st_spi_play_record.strChannelIndex, &playback_result);
				if(ret != ESUCCESS) {
					output_debug_log(true, "[%s] Failed to stop SPI playback & recording.\n", __func__);
					ret = OPERATION_FAILED;
					strcpy(error_info, "Failed to stop SPI playback & recording");
					goto GEN_RESULT;
				}
				else {
					printf("loss frame number:%d among total frame:%d.\n", playback_result.error_frame_number, playback_result.total_frame_number);
					output_debug_log(false, "[%s] loss frame number:%d among total frame:%d.\n", 
								__func__, playback_result.error_frame_number, playback_result.total_frame_number);
				}
			}
			else if(st_spi_play_record.cCommand == QUERY) {
				//call libfm1388.so to do the real work
				ret = is_playing();
				if(ret < 0) {
					output_debug_log(true, "[%s] Failed to query SPI playback & recording.\n", __func__);
					ret = OPERATION_FAILED;
					strcpy(error_info, "Failed to query SPI playback & recording");
					goto GEN_RESULT;
				}
			}
			else {
				output_debug_log(true, "[%s] Got unsupported SPI playback & recording command.\n", __func__);
				ret = COMMAND_UNSUPPORTED;
				strcpy(error_info, "Got unsupported SPI playback & recording command");
				goto GEN_RESULT;
			}
			break;
			
		default:
			output_debug_log(true, "[%s] Unsupported operation.\n", __func__);
			ret = OPERATION_UNSUPPORTED;
			strcpy(error_info, "Unsupported operation");
			goto GEN_RESULT;
	}

GEN_RESULT:
	//write result to result file according to ret value
	result_info[0].op 		= 1;
	result_info[0].addr 	= 0xA5A5A5A5;
	result_info[0].value = ret;
	if(ret != ESUCCESS) {
		strncpy(result_info[0].comment, error_info, COMMENT_LENGTH);
	}
	else {
		if(strlen(error_info) != 0)
			strncpy(result_info[0].comment, error_info, COMMENT_LENGTH);
		else 
			strncpy(result_info[0].comment, "Success", COMMENT_LENGTH);
	}
	
	if(cOperation == SPIRECORD) {
		if((rec_result.error_frame_number != 0) || (rec_result.total_frame_number == 0)) {
			result_info[1].op 		= 1;
			result_info[1].addr 	= rec_result.error_frame_number;
			result_info[1].value 	= 0;
			strncpy(result_info[1].comment, "Loss frame number", COMMENT_LENGTH);

			result_info[2].op 		= 1;
			result_info[2].addr 	= rec_result.total_frame_number;
			result_info[2].value 	= 0;
			strncpy(result_info[2].comment, "Total frame number", COMMENT_LENGTH);

			result_info[3].op 		= 1;
			result_info[3].addr 	= rec_result.first_error_frame_counter;
			result_info[3].value 	= 0;
			strncpy(result_info[3].comment, "First loss happen position", COMMENT_LENGTH);

			result_info[4].op 		= 1;
			result_info[4].addr 	= rec_result.last_error_frame_counter;
			result_info[4].value 	= 0;
			strncpy(result_info[4].comment, "Last loss happen position", COMMENT_LENGTH);

			generate_result_file(result_file_path, result_info, 5);
		}
		else {	
			generate_result_file(result_file_path, result_info, 1);
		}
	}
	else if((cOperation == SPIPLAY) || (cOperation == SPIPLAYRECORD)) {
		if((playback_result.error_frame_number != 0) || (playback_result.total_frame_number == 0)) {
			result_info[1].op 		= 1;
			result_info[1].addr 	= playback_result.error_frame_number;
			result_info[1].value 	= 0;
			strncpy(result_info[1].comment, "Loss frame number", COMMENT_LENGTH);

			result_info[2].op 		= 1;
			result_info[2].addr 	= playback_result.total_frame_number;
			result_info[2].value 	= 0;
			strncpy(result_info[2].comment, "Total frame number", COMMENT_LENGTH);

			result_info[3].op 		= 1;
			result_info[3].addr 	= playback_result.first_error_frame_counter;
			result_info[3].value 	= 0;
			strncpy(result_info[3].comment, "First loss happen position", COMMENT_LENGTH);

			result_info[4].op 		= 1;
			result_info[4].addr 	= playback_result.last_error_frame_counter;
			result_info[4].value 	= 0;
			strncpy(result_info[4].comment, "Last loss happen position", COMMENT_LENGTH);

			generate_result_file(result_file_path, result_info, 5);
		}
		else {	
			generate_result_file(result_file_path, result_info, 1);
		}
	}
	else {	
		generate_result_file(result_file_path, result_info, 1);
	}
	
	return ret;
}
