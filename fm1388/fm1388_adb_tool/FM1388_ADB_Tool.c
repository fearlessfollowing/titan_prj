#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../libfm1388.h"
#include "FM1388_CommandLine_Parser.h"

char request_parameter_filepath[MAX_PATH_LENGTH]	= { 0 };
char result_filepath[MAX_PATH_LENGTH]				= { 0 };

int process_spi_operation(char* parameter_string, char* result_file_path, char* strSDCARD);
int process_read_write_data(char* request_parameter_filepath, char* result_filepath);

//old parameter format:
//FM1388_ADB_Tool							//use default sdcard and vec path, do read / write FM1388 tuning parameter according to FM1388_ADB_Parameter.txt, result save in FM1388_ADB_Result.txt
//FM1388_ADB_Tool $sdcard_path $vec_path	//use specified default sdcard and vec path, do read / write FM1388 tuning parameter according to FM1388_ADB_Parameter.txt, result save in FM1388_ADB_Result.txt
//FM1388_ADB_Tool $sdcard_path $vec_path $SPI_play_record_param	//use specified default sdcard and vec path, do SPI playback and/or recording, result save in FM1388_ADB_Result.txt

//new parameter format:
//FM1388_ADB_Tool -get-version		//get ADB Tool version, result save in FM1388_ADB_Result.txt
//FM1388_ADB_Tool -get-FW-buildno		//get Firmware build number, result save in FM1388_ADB_Result.txt
//FM1388_ADB_Tool -get-path			//get sdcard and vec file path, result save in FM1388_ADB_Result.txt
//FM1388_ADB_Tool -read-write-data	//read write memory data, read/write op and address is in FM1388_ADB_Parameter.txt, result save in FM1388_ADB_Result.txt
//FM1388_ADB_Tool -read-write-parameter	//read write parameter, read/write op and address is in FM1388_ADB_Parameter.txt, result save in FM1388_ADB_Result.txt
//FM1388_ADB_Tool -play-record --cmd=xxx --rec-channel=xxx --rec-file=xxx --channel-map=xxx --play-file=xxx --option=xxx 		//SPI playback recording
//FM1388_ADB_Tool -download-FW --mode=xxx	//download firmware and set specified mode
//FM1388_ADB_Tool -switch-mode --mode=xxx	//just switch mode
//FM1388_ADB_Tool -reset			//redownload firmware and set mode as current
//FM1388_ADB_Tool -check-dependency	//check FM1388 device, lib existence
int main(int argc, char** argv)
{
	char			sdcard_path[MAX_PATH_LENGTH] = { 0 };
	char			firmware_path[MAX_PATH_LENGTH] = { 0 };
	int				i				= 0;
	int				ret				= 0;

	//initialize lifm1388 and device
	ret = lib_open_device();
	if (ret != ESUCCESS) {
		LOGD("[FM1388_ADB_Tool] FM1388 does not work normally.\n");
		return -1;
	}


	output_debug_log(true, "Welcome to use FM1388_ADB_Tool(%s)\n", ADB_TOOL_VERSION);
	ret = parse_execute_command(argc, argv);
	if(ret < 0) {
		if(EPARAMINVAL == ret) {
			output_debug_log(true, "[FM1388_ADB_Tool] Got wrong parameter.\n");
			show_command_line_usage();
		}
		else {
			output_debug_log(true, "[FM1388_ADB_Tool] Error occus when execute command.\n");
		}
		goto END;
	}
	else if(ret == MAX_COMMAND_NUM){
		output_debug_log(true, "[FM1388_ADB_Tool] can not find command from parameter, will do as old version.\n");
	}
	else {
		goto END;
	}
	
	//get sdcard path from command line parameter
	if ((argc > 4)) {
		output_debug_log(true, "[FM1388_ADB_Tool] Too many parameters.\n");
		show_command_line_usage();
	}
	else if ((argc == 3)) {
		strncpy(sdcard_path, argv[1], MAX_PATH_LENGTH - 1);
		strncpy(firmware_path, argv[2], MAX_PATH_LENGTH - 1);
	}
	else if ((argc == 4)) {
		strncpy(sdcard_path, argv[1], MAX_PATH_LENGTH - 1);
		strncpy(firmware_path, argv[2], MAX_PATH_LENGTH - 1);
	}
	else {
		strncpy(sdcard_path, get_sdcard_path(), MAX_PATH_LENGTH - 1);
		strncpy(firmware_path, get_cfg_location(), MAX_PATH_LENGTH - 1);
		
		//omit the file name in path
		for(i = strlen(firmware_path) - 1; i > 0; i--) {
			if(firmware_path[i] == '/') break;
		}
		
		if((i <= 0) || (firmware_path[i] != '/')) {
			output_debug_log(true, "[FM1388_ADB_Tool] FM1388 firmware and parameter file path is not correct.\n");
			goto END;
		}
		else {
			firmware_path[i + 1] = 0;
		}	
	}

	//prepare request and result file path
	snprintf(request_parameter_filepath, MAX_PATH_LENGTH - 1, "%s%s", sdcard_path, request_parameter_filename);
	snprintf(result_filepath, MAX_PATH_LENGTH - 1, "%s%s", sdcard_path, result_filename);

	if(argc == 2) {
		ret = process_spi_operation(argv[1], result_filepath, sdcard_path);
		if (ret < 0) {
			output_debug_log(true, "[FM1388_ADB_Tool] Error occurs when processing SPI play and record.\n");
		}
	}
	else if(argc == 4) {
		ret = process_spi_operation(argv[3], result_filepath, sdcard_path);
		if (ret < 0) {
			output_debug_log(true, "[FM1388_ADB_Tool] Error occurs when processing SPI play and record.\n");
		}
	}
	else {
		ret = process_read_write_data(request_parameter_filepath, result_filepath);
		if (ret < 0) {
			output_debug_log(true, "[FM1388_ADB_Tool] Error occurs when do parameter read/write.\n");
		}
	}
	
END:
	lib_close_device();

	output_debug_log(true, "[FM1388_ADB_Tool] Finished!\n");

	return 0;
}
