#ifndef __FM_FM1388_COMMANDLINE_PARSER_H__
#define __FM_FM1388_COMMANDLINE_PARSER_H__

#include "../libFM1388Parameter.h"

#define MAX_COMMAND_STR_LEN			32
#define MAX_COMMAND_NUM				15
#define MAX_PLAY_RECORD_PARAM_NUM 	6
#define MAX_PLAY_RECORD_PARAM_NUM 	6
#define MAX_FILE_NUM				5

static const char* request_parameter_filename	= "FM1388_ADB_Parameter.txt";
static const char* result_filename				= "FM1388_ADB_Result.txt";
static const char* ADB_TOOL_VERSION				= "1.2(20180830)";
static const char* SUCCESS						= "Success";
static const char* FAILED						= "Failed";

enum FM1388_ADB_COMMAND {
	FM1388_ADB_GET_VERSION,		
	FM1388_ADB_GET_FW_BUILD_NO,		
	FM1388_ADB_GET_PATH,	
	FM1388_ADB_READ_WRITE_DATA,	
	FM1388_ADB_READ_WRITE_PARAMETER,		
	FM1388_ADB_PLAY_RECORD,		
	FM1388_ADB_SWITCH_MODE,		
	FM1388_ADB_RESET_DSP,		
	FM1388_ADB_GET_FW_PATH,		
	FM1388_ADB_CHECK_DEPENDENCY,
	FM1388_ADB_CHECK_FW_WRITABLE,
	FM1388_ADB_CHECK_VEC_WRITABLE,
	FM1388_ADB_GET_CURRENT_VEC_PATH,	
	FM1388_ADB_GET_MODE_LIST,
	FM1388_ADB_SHOW_HELP,
};

enum FM1388_ADB_PLAYREC_PARAM {
	FM1388_ADB_PLAYREC_CMD,		
	FM1388_ADB_PLAYREC_REC_CHANNEL,		
	FM1388_ADB_PLAYREC_REC_FILE,	
	FM1388_ADB_PLAYREC_PLAY_CHANNEL,	
	FM1388_ADB_PLAYREC_PLAY_FILE,		
	FM1388_ADB_PLAYREC_OPTION,
};

typedef struct {
	unsigned char cCommandID;
	char strCommand[MAX_COMMAND_STR_LEN];
} TunerCommand;

static TunerCommand supported_command[MAX_COMMAND_NUM] = { 	{FM1388_ADB_GET_VERSION, 			"get-version"}, 
															{FM1388_ADB_GET_FW_BUILD_NO, 		"get-FW-buildno"}, 
															{FM1388_ADB_GET_PATH, 				"get-path"}, 
															{FM1388_ADB_READ_WRITE_DATA, 		"read-write-data"},
															{FM1388_ADB_READ_WRITE_PARAMETER, 	"read-write-parameter"},
															{FM1388_ADB_PLAY_RECORD, 			"play-record"},
															{FM1388_ADB_SWITCH_MODE, 			"switch-mode"},
															{FM1388_ADB_RESET_DSP, 				"reset"},
															{FM1388_ADB_GET_FW_PATH, 			"get-FW-path"},
															{FM1388_ADB_CHECK_DEPENDENCY, 		"check-dependency"},
															{FM1388_ADB_CHECK_FW_WRITABLE,		"check-fw-writable"},
															{FM1388_ADB_CHECK_VEC_WRITABLE,		"check-vec-writable"},
															{FM1388_ADB_GET_CURRENT_VEC_PATH,	"get-vec-path"},
															{FM1388_ADB_GET_MODE_LIST,			"get-mode-list"},
															{FM1388_ADB_SHOW_HELP,			    "help"},
														};

static const char play_record_parameter[MAX_PLAY_RECORD_PARAM_NUM][MAX_PARAMETER_LEN] = { "cmd", "rec-channel", "rec-file", "channel-map", "play-file", "option" };

typedef struct {
	char strFilePath[MAX_FILEPATH_LEN];
	unsigned int mode;
} DependFile;

static DependFile file_check_list[MAX_FILE_NUM] = { 	
													{"/dev/fm1388", 						R_OK | W_OK }, 
													{"/system/bin/fm_fm1388", 				R_OK | W_OK | X_OK }, 
													{"/system/lib/libfm1388.so", 			R_OK | W_OK }, 
													{"/system/lib/libfmrec_1388.so",		R_OK | W_OK }, 
													{"/system/lib/libFM1388Parameter.so",	R_OK | W_OK }, 
												};

int parse_execute_command(int argc, char** argv);


#endif // __FM_FM1388_COMMANDLINE_PARSER_H__
