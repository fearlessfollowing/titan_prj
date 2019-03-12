#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../libFM1388Parameter.h"
#include "../libfm1388.h"

int get_request_parameter(char* request_parameter_filepath, RequestPara** request_para, int* request_parameter_number) {
	int temp_parameter_number = 0;
	int parameter_number = 0;
	int i = 0;
	
	if ((request_parameter_filepath == NULL) || (request_parameter_number == NULL)) {
		output_debug_log(true, "[%s] Got wrong parameter.\n", __func__);
		return -1;
	}

	//get requested parameter number to allocate memory for structure
	temp_parameter_number = get_parameter_number(request_parameter_filepath);
	if (temp_parameter_number <= 0) {
		output_debug_log(true, "[%s] Got empty request parameter file.\n", __func__);
		return -1;
	}

	*request_para = (RequestPara*)malloc(sizeof(RequestPara) * temp_parameter_number);
	if (*request_para == NULL) {
		output_debug_log(true, "[%s] Can not allocate memory for request parameter.\n", __func__);
		return -1;
	}
	//output_debug_log(true, "[%s] get request parameter number=%d\n", __func__, temp_parameter_number);

	//parse requested parameter file
	parameter_number = parse_para_file(request_parameter_filepath, *request_para, '\t');
	if (parameter_number <= 0) {
		output_debug_log(true, "[%s] Got wrong format request parameter file.\n", __func__);
		return -1;
	}
	
	*request_parameter_number = parameter_number;
	
	return 0;
}


int process_read_write_data(char* request_parameter_filepath, char* result_filepath) {
	int 			ret 						= 0;
	RequestPara* 	request_parameter			= NULL;
	RequestPara* 	result_parameter			= NULL;
	int				request_parameter_number	= 0;
	int 			total_parameter_number		= 0;
	int				result_index				= 0;
	int				need_change					= 0;
	int				i							= 0;
	
	if((request_parameter_filepath == NULL) || (result_filepath == NULL)) {
		output_debug_log(true, "[%s] Got wrong parameters.\n", __func__);
		return -1;
	}
	
	//get requested parameter number to allocate memory for structure
	ret = get_request_parameter(request_parameter_filepath, &request_parameter, &request_parameter_number);
	output_debug_log(true, "[%s] ret=%d\n", __func__, ret);
	output_debug_log(true, "[%s] request_parameter_number=%d\n", __func__, request_parameter_number);
	if (ret < 0) {
		output_debug_log(true, "[%s] Error occurs when calling get_request_parameter(). ret=%d\n", __func__, ret);
		goto EXIT;
	}


	output_debug_log(true, "[%s] deal with write parameter.\n", __func__);
	//process write operation first and save parameter vec file
	for (i = 0; i < request_parameter_number; i++) {
		if (request_parameter[i].op == OPERATION_WRITE) {
			ret = set_dsp_mem_value_spi(request_parameter[i].addr, request_parameter[i].value);
			if(ret != ESUCCESS) {
				output_debug_log(true, "[%s] Failed to write parameter to FM1388. ret=%d\n", ret);
			}
		}
	}

	output_debug_log(true, "[%s] deal with read parameter.\n", __func__);
	//process read operation, then generate result file
	result_parameter = (RequestPara*)malloc(sizeof(RequestPara) * request_parameter_number);
	if (result_parameter == NULL) {
		output_debug_log(true, "[%s] Can not allocate memory for result parameter.\n", __func__);
		ret = -1;
		goto EXIT;
	}

	output_debug_log(true, "[%s] generate result array.\n", __func__);
	result_index = 0;
	for (i = 0; i < request_parameter_number; i++) {
		if (request_parameter[i].op == OPERATION_READ) {
			request_parameter[i].value = get_dsp_mem_value_spi(request_parameter[i].addr) & 0xFFFF;
			
			result_parameter[result_index].addr = request_parameter[i].addr;
			result_parameter[result_index].value = request_parameter[i].value;
			output_debug_log(true, "[%s] address=%08x\n", __func__, result_parameter[result_index].addr);
			output_debug_log(true, "[%s] value=%04x\n", __func__, result_parameter[result_index].value);
			strncpy(result_parameter[result_index].comment, request_parameter[i].comment, COMMENT_LENGTH);

			result_index++;
		}
	}
	
	output_debug_log(true, "[%s] generate result file.\n", __func__);		
	if(result_index > 0)
		generate_result_file(result_filepath, result_parameter, result_index);
		
EXIT:		
	if (request_parameter != NULL) free(request_parameter);
	if (result_parameter != NULL) free(result_parameter);

	return ret;
}
