#include <sys/stat.h>     
#include <fcntl.h>     
#include <sys/mman.h> 
#include <unistd.h>     
#include <pthread.h>     
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>

#include "libfmrec.h"
#include "libFM1388Parameter.h"



//output text buffer from parameter list
int generate_result(RequestPara* para_list, int para_size, char* para_string) 
{
	int i;

	if (NULL == para_string) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Fail, para_string is NULL!\n", __func__);
		return -1;
	}

	if (NULL == para_list) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Fail, para_list is NULL!\n", __func__);
		return -1;
	}

	if (0 == para_size) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Fail, para_size is 0!\n", __func__);
		return -1;
	}

	char temp_buf[SMALL_BUFFER_SIZE + 1] = { 0 };
	for (i = 0; i < para_size; i++) {
		memset(temp_buf, 0, (SMALL_BUFFER_SIZE + 1) * sizeof(char));
		if(para_list[i].addr > 0xFFFF)
			snprintf(temp_buf, SMALL_BUFFER_SIZE, "0x%08X\t0x%04X\t\t%s\n", para_list[i].addr & 0xFFFFFFFF, para_list[i].value & 0xFFFF, para_list[i].comment);
		else 
			snprintf(temp_buf, SMALL_BUFFER_SIZE, "0x%X\t0x%04X\t\t%s\n", para_list[i].addr & 0xFFFF, para_list[i].value & 0xFFFF, para_list[i].comment);

		strncat(para_string, temp_buf, SMALL_BUFFER_SIZE);
	}

	return 0;
}

//output text file from parameter list
int generate_result_file(const char* file_path, RequestPara* para_list, int para_size) 
{
	FILE* 	fp_out 		= NULL;
	char* 	para_buffer = NULL;
	int 	i;
	int 	ret 		= 0;

	if (NULL == file_path) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Fail, file_path is NULL!\n", __func__);
		return -1;
	}

	if (NULL == para_list) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Fail, para_list is NULL!\n", __func__);
		return -1;
	}

	if (0 == para_size) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Fail, para_size is 0!\n", __func__);
		return -1;
	}

	//allocate memory for parameter list
	int buffer_length = SMALL_BUFFER_SIZE;//10 + 1 + 6 + 2 + COMMENT_LENGTH + 2; //address + tab + value + tab + tab + comment + CRLF
	para_buffer = (char*)malloc(sizeof(char) * (para_size * (buffer_length + 1)));
	if (NULL == para_buffer) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Fail, paraBuffer is NULL!\n", __func__);
		return -1;
	}

	memset(para_buffer, 0, sizeof(char) * (para_size * (buffer_length + 1)));
	if ((fp_out = fopen(file_path, "wb")) == NULL) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Error open output file %s to write!\n", __func__, file_path);
		if (NULL != para_buffer)
			free(para_buffer);
		
		return -1;
	}

	ret = generate_result(para_list, para_size, para_buffer);
	if (0 == ret) {
		fwrite(para_buffer, sizeof(char), strlen(para_buffer), fp_out);
	}
	fclose(fp_out);
 
	if (NULL != para_buffer)
		free(para_buffer);

	return 0;
}

//parse parameter list from string buffer
int parse_para(char* para_string, RequestPara* para_list, char delimiter) 
{
	int ret = 0;

	if (NULL == para_string) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Fail, parameter buffer is NULL!\n", __func__);
		return -1;
	}

	if (NULL == para_list) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Fail, para_list is NULL!\n", __func__);
		return -1;
	}
		
	char	str_temp_line[SMALL_BUFFER_SIZE + 1];
	char*	temp_line_ptr = NULL;
	char*	temp_line_ptr1 = NULL;
	unsigned int line_index = 0;
	char*	temp_ptr = NULL;
	int		field_index = 0;
	int		string_length = strlen(para_string);
	long	index = 0L;
	unsigned int listIndex = 0;
	char	temp_buf[SMALL_BUFFER_SIZE] = { 0 };
	char*	line = para_string;

	while (index < string_length) {
		//get one line
		temp_line_ptr = strchr(line + index, '\n');
		temp_line_ptr1 = strchr(line + index, '\r');
		if (temp_line_ptr1 != NULL && (temp_line_ptr1 < temp_line_ptr)) temp_line_ptr = temp_line_ptr1;

		if(temp_line_ptr == NULL) { //deal with the case that the last line has not CRLF
			temp_line_ptr = line + strlen(line);
		}
		
		memset(str_temp_line, 0, SMALL_BUFFER_SIZE + 1);
		strncpy(str_temp_line, line + index, temp_line_ptr - line - index);

		if ((str_temp_line[0] == '#') || (str_temp_line[0] == '/')) { //skip comment line
			index = temp_line_ptr - line;
			while ((line[index] == '\n') || (line[index] == '\r')) index++;
			continue;
		}

		memset(&para_list[listIndex], 0, sizeof(RequestPara));

		line_index = 0;

		//output_debug_log(true, "[FM1388 Parameter Lib--%s] str_temp_line=%s\n", __func__, str_temp_line);
		//check operation field
		if (((str_temp_line[0] == OPERATION_READ) || (str_temp_line[0] == OPERATION_WRITE)) && (str_temp_line[1] == delimiter)) {
			para_list[listIndex].op = str_temp_line[0];
			line_index++;
		}
 
		//output_debug_log(true, "[FM1388 Parameter Lib--%s] line_index=%d\n", __func__, line_index);
		field_index = 0;
		while (str_temp_line[line_index] == delimiter) line_index++; //skip continuous seperator
		//output_debug_log(true, "[FM1388 Parameter Lib--%s] line_index111111=%d\n", __func__, line_index);
		temp_ptr = strchr(str_temp_line + line_index, delimiter);
		if (temp_ptr == NULL && (line_index < strlen(str_temp_line))) {
			temp_ptr = str_temp_line + strlen(str_temp_line);
		}
		while (temp_ptr != NULL) {
			memset(temp_buf, 0, SMALL_BUFFER_SIZE);
			strncpy(temp_buf, str_temp_line + line_index, temp_ptr - (str_temp_line + line_index));
			//output_debug_log(true, "[FM1388 Parameter Lib--%s] field_index=%d, temp_buf=%s\n", __func__, field_index, temp_buf);
			if (field_index == 0) { //address
				para_list[listIndex].addr = strtol(temp_buf, NULL, 16);
			}
			else if (field_index == 1) { //value
				para_list[listIndex].value = strtol(temp_buf, NULL, 16);
			}
			else if (field_index == 2) { //comment
				strncpy(para_list[listIndex].comment, temp_buf, COMMENT_LENGTH);
			}
			else { //wrong
				output_debug_log(true, "[FM1388 Parameter Lib--%s] listIndex=%x, line_index=%d,  lineLength=%d, field_index=%d, temp_buf=%s\n", __func__, listIndex, line_index, strlen(str_temp_line), field_index, temp_buf);
				output_debug_log(true, "[FM1388 Parameter Lib--%s] wrong format\n", __func__);
			}

			field_index++;
			line_index = temp_ptr - str_temp_line;
			while (str_temp_line[line_index] == delimiter) line_index++;//skip continuous seperator
			temp_ptr = strchr(str_temp_line + line_index, delimiter);

			if (temp_ptr == NULL && (line_index < strlen(str_temp_line))) {
				temp_ptr = str_temp_line + strlen(str_temp_line);
			}
		}


		index = temp_line_ptr - line;
		while ((line[index] == '\n') || (line[index] == '\r')) index++;
		listIndex++;
	}
	
	ret = listIndex;

	return ret;
}

int parse_para_file(const char* file_path, RequestPara* para_list, char delimiter) {
	FILE* 	fp_para		= NULL;
	char* 	para_buffer = NULL;
	int  	ret			= 0;
	long 	file_length = 0;

	if (NULL == file_path) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Fail, file_path is NULL!\n", __func__);
		return -1;
	}

	if (NULL == para_list) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Fail, para_list is NULL!\n", __func__);
		return -1;
	}
	

	//get file length
	if ((fp_para = fopen(file_path, "rb")) == NULL) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Error open parameter file %s to read!\n", __func__, file_path);
		return -1;
	}

	fseek(fp_para, 0L, SEEK_END);
	file_length = ftell(fp_para);
	fseek(fp_para, 0L, SEEK_SET);

	if (0 == file_length) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] parameter file is empty!\n", __func__);
		ret = -1;
		goto EXIT;
	}
		
	para_buffer = (char*)malloc(sizeof(char) * (file_length + 1));
	if (NULL == para_buffer) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Fail, para_buffer is NULL!\n", __func__);
		
		ret = -1;
		goto EXIT;
	}

	memset(para_buffer, 0, file_length + 1);
	long read_len = fread(para_buffer, sizeof(char), file_length, fp_para);
	if (read_len != file_length) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Fail, para_buffer is not completed!\n", __func__);
		
		ret = -1;
		goto EXIT;
	}

	//output_debug_log(true, "[FM1388 Parameter Lib--%s] got parameter string: \n%s\n", __func__, para_buffer);
	ret = parse_para(para_buffer, para_list, delimiter);
	
	EXIT:
	if (NULL != fp_para) fclose(fp_para);
	if (NULL != para_buffer) free(para_buffer);

	return ret;
}

//parse mode list from string buffer
int parse_mode(char* mode_string, ModeInfo* mode_list, char delimiter) 
{
	int ret = 0;

	if (NULL == mode_string) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Fail, mode buffer is NULL!\n", __func__);
		return -1;
	}

	if (NULL == mode_list) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Fail, mode_list is NULL!\n", __func__);
		return -1;
	}

	char	str_temp_line[LARGE_BUFFER_SIZE + 1];
	char*	temp_line_ptr = NULL;
	char*	temp_line_ptr1 = NULL;
	unsigned int line_index = 0;
	char*	temp_ptr = NULL;
	int		field_index = 0;
	int		string_length = strlen(mode_string);
	long	index = 0L;
	unsigned int list_index = 0;
	char	temp_buf[LARGE_BUFFER_SIZE] = { 0 };
	char*	line = mode_string;

	while (index < string_length) {
		//get one line
		temp_line_ptr = strchr(line + index, '\n');
		temp_line_ptr1 = strchr(line + index, '\r');
		if (temp_line_ptr1 != NULL && (temp_line_ptr1 < temp_line_ptr)) temp_line_ptr = temp_line_ptr1;

		if (temp_line_ptr == NULL) { //deal with the case that the last line has not CRLF
			temp_line_ptr = line + strlen(line);
		}

		memset(str_temp_line, 0, LARGE_BUFFER_SIZE + 1);
		strncpy(str_temp_line, line + index, temp_line_ptr - line - index);

		if ((str_temp_line[0] == '#') || (str_temp_line[0] == '/')) { //skip comment line
			index = temp_line_ptr - line;
			while ((line[index] == '\n') || (line[index] == '\r')) index++;
			continue;
		}

		memset(&mode_list[list_index], 0, sizeof(ModeInfo));

		line_index = 0;

		//output_debug_log(true, "[FM1388 Parameter Lib--%s] str_temp_line=%s\n", __func__, str_temp_line);
		//output_debug_log(true, "[FM1388 Parameter Lib--%s] line_index=%d\n", __func__, line_index);
		//check id field
		field_index = 0;
		while (str_temp_line[line_index] == delimiter) line_index++; //skip continuous seperator
		while (str_temp_line[line_index] == ' ') line_index++; //skip continuous SPACE
		//output_debug_log(true, "[FM1388 Parameter Lib--%s] line_index111111=%d\n", __func__, line_index);
		temp_ptr = strchr(str_temp_line + line_index, delimiter);
		if (temp_ptr == NULL && (line_index < strlen(str_temp_line))) {
			temp_ptr = str_temp_line + strlen(str_temp_line);
		}
		while (temp_ptr != NULL) {
			memset(temp_buf, 0, LARGE_BUFFER_SIZE);
			strncpy(temp_buf, str_temp_line + line_index, temp_ptr - (str_temp_line + line_index));
			//output_debug_log(true, "[FM1388 Parameter Lib--%s] field_index=%d, temp_buf=%s\n", __func__, field_index, temp_buf);
			if (field_index == 0) { //id
				mode_list[list_index].id = (unsigned char)strtol(temp_buf, NULL, 16);
			}
			else if (field_index == 1) { //path file
				strncpy(mode_list[list_index].path_file_name, temp_buf, MAX_NAME_LENGTH);
			}
			else if (field_index == 2) { //parameter file
				strncpy(mode_list[list_index].parameter_file_name, temp_buf, MAX_NAME_LENGTH);
			}
			else if (field_index == 3) { //mode name
				strncpy(mode_list[list_index].mode_name, temp_buf, MAX_NAME_LENGTH);
			}
			else { //wrong
				output_debug_log(true, "[FM1388 Parameter Lib--%s] list_index=%x, line_index=%d,  line_length=%d, field_index=%d, temp_buf=%s\n", __func__, list_index, line_index, strlen(str_temp_line), field_index, temp_buf);
				output_debug_log(true, "[FM1388 Parameter Lib--%s] wrong format\n", __func__);
			}

			field_index++;
			line_index = temp_ptr - str_temp_line;
			while (str_temp_line[line_index] == delimiter) line_index++;//skip continuous seperator
			while (str_temp_line[line_index] == ' ') line_index++; //skip continuous SPACE
			temp_ptr = strchr(str_temp_line + line_index, delimiter);

			if (temp_ptr == NULL && (line_index < strlen(str_temp_line))) {
				temp_ptr = str_temp_line + strlen(str_temp_line);
			}
		}


		index = temp_line_ptr - line;
		while ((line[index] == '\n') || (line[index] == '\r')) index++;
		if(index < string_length)
			list_index++;
	}

	ret = list_index;

	return ret;
}

int parse_mode_file(const char* file_path, ModeInfo* mode_list, char delimiter) 
{
	FILE* 	fp_mode = NULL;
	char* 	mode_buffer = NULL;
	int  	ret = 0;
	long 	file_length = 0;

	if (NULL == file_path) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Fail, file_path is NULL!\n", __func__);
		return -1;
	}

	if (NULL == mode_list) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Fail, mode_list is NULL!\n", __func__);
		return -1;
	}


	//get file length
	if ((fp_mode = fopen(file_path, "rb")) == NULL) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Error open mode config file %s to read!\n", __func__, file_path);
		return -1;
	}

	fseek(fp_mode, 0L, SEEK_END);
	file_length = ftell(fp_mode);
	fseek(fp_mode, 0L, SEEK_SET);

	//output_debug_log(true, "[FM1388 Parameter Lib--%s] file_length=%ld\n", __func__, file_length);
	if (0 == file_length) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] mode config file is empty!\n", __func__);
		ret = -1;
		goto EXIT;
	}

	mode_buffer = (char*)malloc(sizeof(char) * (file_length + 1));
	if (NULL == mode_buffer) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Fail, mode_buffer is NULL!\n", __func__);

		ret = -1;
		goto EXIT;
	}

	memset(mode_buffer, 0, file_length + 1);
	long read_len = fread(mode_buffer, sizeof(char), file_length, fp_mode);
	//output_debug_log(true, "[FM1388 Parameter Lib--%s] read_len=%ld\n", __func__, read_len);
	if (read_len != file_length) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Fail, mode_buffer is not completed!\n", __func__);

		ret = -1;
		goto EXIT;
	}

	//output_debug_log(true, "[FM1388 Parameter Lib--%s] got mode string: \n%s\n", __func__, mode_buffer);
	ret = parse_mode(mode_buffer, mode_list, delimiter);

EXIT:
	if (NULL != fp_mode) fclose(fp_mode);
	if (NULL != mode_buffer) free(mode_buffer);

	return ret;
}

int get_parameter_number(const char* file_path) 
{
	FILE* 	fp_para		= NULL;
	int 	count	 	= 0;
	char 	line[255]	= { 0 };

	if (NULL == file_path) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Fail, filePath is NULL!\n", __func__);
		return -1;
	}

	//get file line number
	if ((fp_para = fopen(file_path, "rb")) == NULL) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] Error open parameter file %s to read!\n", __func__, file_path);
		return -1;
	}

	while (fgets(line, 255, fp_para) != NULL) {
		if ((line[0] == '#') || (line[0] == '/')) continue;
		count++;
	}
	
	if (fp_para != NULL) fclose(fp_para);
	return count;
}

//for playback & recording
int parse_play_command(char* parameter_string, SPIPlay* p_spi_play)
{
	int i = 0;
	char str_temp[2];
	
	if (p_spi_play == NULL) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] wrong parameter, p_spi_play is NULL.\n", __func__);
		return -1;
	}

	if (parameter_string == NULL) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] wrong parameter, parameter_string is NULL.\n", __func__);
		return -2;
	}

	memcpy(p_spi_play, parameter_string, sizeof(SPIPlay));
		
	str_temp[0] = p_spi_play->cChannelNum;
	str_temp[1] = 0;
	p_spi_play->cChannelNum = strtol(str_temp, NULL, 16);


	for(i = 0; i < (MAX_MAP_CH_NUM * 3 + 1); i++) {
		if(p_spi_play->strChannelMapping[i] == PLACEHOLDER) {
			p_spi_play->strChannelMapping[i] = 0;
		}
	} 

	for(i = 0; i < (MAX_FILEPATH_LEN + 1); i++) {
		if(p_spi_play->strInputFilePath[i] == PLACEHOLDER) {
			p_spi_play->strInputFilePath[i] = 0;
		}
	} 
/*
output_debug_log(true, "[FM1388 Parameter Lib--%s] p_spi_play->cOperation = %c\n", __func__, p_spi_play->cOperation);
output_debug_log(true, "[FM1388 Parameter Lib--%s] p_spi_play->cCommand = %c\n", __func__, p_spi_play->cCommand);
output_debug_log(true, "[FM1388 Parameter Lib--%s] p_spi_play->cChannelNum = %d\n", __func__, p_spi_play->cChannelNum);
output_debug_log(true, "[FM1388 Parameter Lib--%s] p_spi_play->strChannelMapping = %s\n", __func__, p_spi_play->strChannelMapping);
output_debug_log(true, "[FM1388 Parameter Lib--%s] p_spi_play->strInputFilePath = %s\n", __func__, p_spi_play->strInputFilePath);
*/	
	return ESUCCESS;
}

int parse_record_command(char* parameter_string, SPIRecord* p_spi_record, char* strSDCARD) 
{
	int i = 0;

	if (strSDCARD == NULL) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] wrong parameter, strSDCARD is NULL.\n", __func__);
		return -1;
	}

	if (p_spi_record == NULL) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] wrong parameter, p_spi_record is NULL.\n", __func__);
		return -1;
	}

	if (parameter_string == NULL) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] wrong parameter, parameter_string is NULL.\n", __func__);
		return -2;
	}

	memcpy(p_spi_record, parameter_string, sizeof(SPIRecord));
	for(i = 0; i < (MAX_FILEPATH_LEN + 1); i++) {
		if(p_spi_record->strOutputFilePath[i] == PLACEHOLDER) {
			p_spi_record->strOutputFilePath[i] = 0;
		}
	} 
/*	
output_debug_log(true, "[FM1388 Parameter Lib--%s] p_spi_record->cOperation = %c\n", __func__, p_spi_record->cOperation);
output_debug_log(true, "[FM1388 Parameter Lib--%s] p_spi_record->cCommand = %c\n", __func__, p_spi_record->cCommand);
output_debug_log(true, "[FM1388 Parameter Lib--%s] p_spi_record->strChannelIndex = %s\n", __func__, p_spi_record->strChannelIndex);
output_debug_log(true, "[FM1388 Parameter Lib--%s] p_spi_record->strOutputFilePath = %s\n", __func__, p_spi_record->strOutputFilePath);
*/	

	return ESUCCESS;
}

int parse_play_record_command(char* parameter_string, SPIPlayRecord* p_spi_play_record, char* strSDCARD) 
{
	int i = 0;
	char str_temp[2];
	
	if (strSDCARD == NULL) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] wrong parameter, strSDCARD is NULL.\n", __func__);
		return -1;
	}

	if (p_spi_play_record == NULL) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] wrong parameter, p_spi_play_record is NULL.\n", __func__);
		return -1;
	}

	if (parameter_string == NULL) {
		output_debug_log(true, "[FM1388 Parameter Lib--%s] wrong parameter, parameter_string is NULL.\n", __func__);
		return -2;
	}

	memcpy(p_spi_play_record, parameter_string, sizeof(SPIPlayRecord));
	str_temp[0] = p_spi_play_record->cChannelNum;
	str_temp[1] = 0;
	p_spi_play_record->cChannelNum = strtol(str_temp , NULL, 16);

	for(i = 0; i < (MAX_MAP_CH_NUM * 3 + 1); i++) {
		if(p_spi_play_record->strChannelMapping[i] == PLACEHOLDER) {
			p_spi_play_record->strChannelMapping[i] = 0;
		}
	} 

	for(i = 0; i < (MAX_FILEPATH_LEN + 1); i++) {
		if(p_spi_play_record->strInputFilePath[i] == PLACEHOLDER) {
			p_spi_play_record->strInputFilePath[i] = 0;
		}
	} 

	for(i = 0; i < (MAX_FILEPATH_LEN + 1); i++) {
		if(p_spi_play_record->strOutputFilePath[i] == PLACEHOLDER) {
			p_spi_play_record->strOutputFilePath[i] = 0;
		}
	} 
	
/*	
output_debug_log(true, "[FM1388 Parameter Lib--%s] p_spi_play_record->cOperation = %c\n", __func__, p_spi_play_record->cOperation);
output_debug_log(true, "[FM1388 Parameter Lib--%s] p_spi_play_record->cCommand = %c\n", __func__, p_spi_play_record->cCommand);
output_debug_log(true, "[FM1388 Parameter Lib--%s] p_spi_play_record->cChannelNum = %d\n", __func__, p_spi_play_record->cChannelNum);
output_debug_log(true, "[FM1388 Parameter Lib--%s] p_spi_play_record->strChannelMapping = %s\n", __func__, p_spi_play_record->strChannelMapping);
output_debug_log(true, "[FM1388 Parameter Lib--%s] p_spi_play_record->strInputFilePath = %s\n", __func__, p_spi_play_record->strInputFilePath);
output_debug_log(true, "[FM1388 Parameter Lib--%s] p_spi_play_record->strChannelIndex = %s\n", __func__, p_spi_play_record->strChannelIndex);
output_debug_log(true, "[FM1388 Parameter Lib--%s] p_spi_play_record->strOutputFilePath = %s\n", __func__, p_spi_play_record->strOutputFilePath);
*/
	return ESUCCESS;
}
//

