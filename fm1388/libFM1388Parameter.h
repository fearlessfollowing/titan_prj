#ifndef __FM_FM1388_TUNING_LIB_H__
#define __FM_FM1388_TUNING_LIB_H__

#define ESUCCESS				0

#define LOGD printf
#define READ_BUF_SIZE			1024
#define MAX_PARAMETER_NUMBER	1024
#define MAX_CMDLINE_LEN			1024
#define SMALL_BUFFER_SIZE 		256
#define LARGE_BUFFER_SIZE 		512
#define COMMENT_LENGTH 			128
#define	PID_LENGTH				16
#define	MAX_PATH_LENGTH			256
#define	MAX_NAME_LENGTH			64

//for playback & recording
#define MAX_MAP_CH_NUM			6
#define DSP_SPI_CH_NUM			10
#define MAX_FILEPATH_LEN		64
#define MAX_PARAMETER_LEN		64
#define PLACEHOLDER				'*'
#define SPIPLAY					'P'
#define SPIRECORD				'R'
#define SPIPLAYRECORD			'B'
#define START					'S'
#define STOP					'T'
#define QUERY					'P'
#define PARAMETER_PARSE_ERROR   -1
#define OPERATION_UNSUPPORTED	-2
#define COMMAND_UNSUPPORTED		-3
#define OPERATION_FAILED		-4
//

#define TX_BASE_ADDRESS			0x5FFDF720
#define RX_BASE_ADDRESS			0x5FFDFC60
#define DBG_BASE_ADDRESS		0x5FFDFD80
#define MAX_ADDRESS				0x5FFDFEC6
#define OPERATION_READ			('0')
#define OPERATION_WRITE			('1')
#define MAX_PARAMETER_NUMBER	1024

typedef struct{
	unsigned char	op;
	unsigned int	addr;
	unsigned short	value;
	char			comment[COMMENT_LENGTH];
}RequestPara;

typedef struct{
	unsigned char	id;
	char			path_file_name[MAX_NAME_LENGTH];
	char			parameter_file_name[MAX_NAME_LENGTH];
	char			mode_name[MAX_NAME_LENGTH];
}ModeInfo;


//for playback & recording
typedef struct {
	unsigned char cOperation;
	unsigned char cCommand;
	unsigned char strChannelIndex[DSP_SPI_CH_NUM];
	char strOutputFilePath[MAX_FILEPATH_LEN + 1];
} SPIRecord;

typedef struct {
	unsigned char cOperation;
	unsigned char cCommand;
	unsigned char cChannelNum;
	char strChannelMapping[MAX_MAP_CH_NUM * 3 + 1];
	char strInputFilePath[MAX_FILEPATH_LEN + 1];
	unsigned char cPlayMode;
	unsigned char cPlayOutput;
} SPIPlay;

typedef struct {
	unsigned char cOperation;
	unsigned char cCommand;
	unsigned char cChannelNum;
	char strChannelMapping[MAX_MAP_CH_NUM * 3 + 1];
	char strInputFilePath[MAX_FILEPATH_LEN + 1];
	unsigned char strChannelIndex[DSP_SPI_CH_NUM];
	char strOutputFilePath[MAX_FILEPATH_LEN + 1];
	unsigned char cPlayMode;
	unsigned char cPlayOutput;
} SPIPlayRecord;
//

//parse and output parameter structure to vec file
int generate_result(RequestPara* para_list, int para_size, char* para_string);
int generate_result_file(const char* file_path, RequestPara* para_list, int para_size);
int parse_para(char* para_string, RequestPara* para_list, char delimiter);
int parse_para_file(const char* file_path, RequestPara* para_list, char delimiter);
int parse_mode(char* mode_string, ModeInfo* para_list, char delimiter);
int parse_mode_file(const char* file_path, ModeInfo* para_list, char delimiter);
int get_parameter_number(const char* file_path);

//for playback & recording
int parse_play_command(char* parameter_string, SPIPlay* p_spi_play);
int parse_record_command(char* parameter_string, SPIRecord* p_spi_record, char* strSDCARD);
int parse_play_record_command(char* parameter_string, SPIPlayRecord* p_spi_play_record, char* strSDCARD);
//

#endif // __FM_FM1388_TUNING_LIB_H__
