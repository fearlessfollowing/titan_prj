

#ifdef __cplusplus
extern "C"{
#endif

/* Standard Linux headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#include "debug_log.h"
#include "sysdata.h"

#define MAX_LOG_SIZE					1048576 
#define MAX_FILE_CNT					5 		

const char *LOG_LEV[] =
{
	"",
	"ERR",
	"WAN",
	"",
	"NOT",
	"",
	"",
	"",
	"DBG",
};


typedef struct LOG_FILE_s
{
	char pcFilePath[256];			/* Log��ǰ�ļ�·�� */
	char pcFilePrefix[12];			/* Logǰ׺ */
	int iMaxFileSize;				/* Log��С */
	int iMaxFileCount;				/* Log�ļ���������Ҫ���ƿռ��С */

	FILE *pfFileHandle;			/* �ļ���� */ 
	int iSeq;						/* Log�ļ���� */
	int iSize;						/* ��ǰ�ļ���С */
	int iLevel;					/* ��ӡ���� */
	int iDirect;					/* �Ƿ�д��־�ļ�������ֱ���������̨ */
	pthread_mutex_t sLockMutex;	/* �߳��� */
       
} LOG_FILE_T;

static LOG_FILE_T sg_LogArgs;


static int logGetSeqID(void)
{
	char acPath[256], acTemp[64];
	FILE *pIDFile = NULL;
	
	sprintf(acPath, "%s%sid.dat", sg_LogArgs.pcFilePath, sg_LogArgs.pcFilePrefix);
	pIDFile = fopen(acPath, "r");
	if (pIDFile != NULL) {
		if (fgets(acTemp, 64, pIDFile)) {
			sg_LogArgs.iSeq = atoi(acTemp);
		}
		
		fclose(pIDFile);
		return 0;
	}
	
	return 1;
}


static void logGetFSize(void)
{
	char acPath[256];
	struct stat stBuf;

	memset(&stBuf, 0, sizeof(stBuf));
	sprintf(acPath, "%s%s%02d.log", sg_LogArgs.pcFilePath, sg_LogArgs.pcFilePrefix, sg_LogArgs.iSeq);
	stat(acPath, &stBuf);
	sg_LogArgs.iSize = stBuf.st_size; 
}


static void logSetSeqID(void)
{
	char acPath[256], acTemp[64];
	FILE *pIDFile;
	
	sprintf(acPath, "%s%sid.dat", sg_LogArgs.pcFilePath, sg_LogArgs.pcFilePrefix);
	pIDFile = fopen(acPath, "w");
	if (pIDFile != NULL) {
		sprintf(acTemp, "%d", sg_LogArgs.iSeq);
		fputs(acTemp, pIDFile);
		fclose(pIDFile);
	}
}


static void logChangeFile(void)
{
	char acPath[256];


	if (sg_LogArgs.pfFileHandle != NULL) {
		fflush(sg_LogArgs.pfFileHandle);
		fclose(sg_LogArgs.pfFileHandle);
	}
	
	sg_LogArgs.iSeq++;
	if (sg_LogArgs.iSeq > sg_LogArgs.iMaxFileCount) {
		sg_LogArgs.iSeq = 1;
	}

	sprintf(acPath, "%s%s%02d.log", sg_LogArgs.pcFilePath, sg_LogArgs.pcFilePrefix, sg_LogArgs.iSeq);
	unlink(acPath);

	sg_LogArgs.pfFileHandle = fopen(acPath, "a+");
	sg_LogArgs.iSize = 0;
	logSetSeqID();
}


int LogInit(char *pcFilePath, char *pcFilePrefix, int iMaxFileSize, \
                   int iMaxFileCount, int iLevel, int iDirect, int iParamChangeState)
{
	char acPath[128];
	
	if (pcFilePath != NULL) {
		strncpy(sg_LogArgs.pcFilePath, pcFilePath, 256);
	} else {	
		strcpy(sg_LogArgs.pcFilePath, LOG_WORK_PATH());
	}
	

	if (access(pcFilePath, F_OK) < 0) {
		mkdir(pcFilePath, 0777);
	}

	if (sg_LogArgs.pcFilePath[strlen(sg_LogArgs.pcFilePath) -1] != '/') {
		strcat(sg_LogArgs.pcFilePath, "/");
	}

	if (pcFilePrefix != NULL) {
		strncpy(sg_LogArgs.pcFilePrefix, pcFilePrefix, 12);
	} else {
		strcpy(sg_LogArgs.pcFilePrefix, "ipcam_");
	}

	sg_LogArgs.iLevel = iLevel;
	
	sg_LogArgs.iDirect = iDirect;

	if (iMaxFileSize > MAX_LOG_SIZE || iMaxFileSize == 0) {
		iMaxFileSize = MAX_LOG_SIZE;
	}

	if (iMaxFileCount > MAX_FILE_CNT || iMaxFileCount == 0) {
		iMaxFileCount = MAX_FILE_CNT;
	}

	sg_LogArgs.iMaxFileSize = iMaxFileSize;
	sg_LogArgs.iMaxFileCount = iMaxFileCount;
	sg_LogArgs.iSeq = 1;

	if (LOGPARAM_UPDATE == iParamChangeState) {
		LogDestroy();
	}

	pthread_mutex_init(&sg_LogArgs.sLockMutex, NULL);
	

	if (logGetSeqID()) {
		logSetSeqID(); 
	}

	logGetFSize();

	sprintf(acPath,"%s%s%02d.log", sg_LogArgs.pcFilePath, sg_LogArgs.pcFilePrefix, sg_LogArgs.iSeq);
	if ((sg_LogArgs.pfFileHandle = fopen(acPath, "a+")) == NULL) {
		printf("fopen %s failed\n", acPath);
		return 1;
	} else {
		if (LOG_FLASH == iDirect) {
			printf("Log file is %s\n", acPath);
		}
		return 0;
	}
}


void LogDestroy(void)
{
	pthread_mutex_lock(&sg_LogArgs.sLockMutex);

	if (sg_LogArgs.pfFileHandle != NULL) {
		fclose(sg_LogArgs.pfFileHandle);
	}

	pthread_mutex_unlock(&sg_LogArgs.sLockMutex);
	pthread_mutex_destroy(&sg_LogArgs.sLockMutex);
}


void LOG(int iLevel, int iLine, char *pcFile, const char *pcFunc, const char *fmt, ...)
{
	char acTime[32];
	struct tm stTime;
	struct timeval stDetailTtime;
	FILE *pOut = NULL;
	int iCnt = 0;
	

	if (!(iLevel & sg_LogArgs.iLevel)) {
		return;
	}

	acTime[0] = 0;
	
	pthread_mutex_lock(&sg_LogArgs.sLockMutex);

	if (sg_LogArgs.iSize >= sg_LogArgs.iMaxFileSize) {
		logChangeFile();
	}

	gettimeofday(&stDetailTtime, NULL);
	localtime_r(&stDetailTtime.tv_sec, &stTime);

	sprintf(acTime, "%04d/%02d/%02d %02d:%02d:%02d:%03ld", stTime.tm_year+1900, \
		stTime.tm_mon + 1, stTime.tm_mday, stTime.tm_hour, stTime.tm_min, stTime.tm_sec, 
		stDetailTtime.tv_usec / 1000);

	va_list vap;
	va_start(vap, fmt);

	if (sg_LogArgs.iDirect == LOG_TTY) {
		pOut = stdout;
	} else {
		pOut = sg_LogArgs.pfFileHandle;
	}

	if (pOut != NULL) {
		iCnt = fprintf(pOut, "[%s]--[%s][%s][%s][%d]--", LOG_LEV[iLevel%LOG_LAST], acTime, pcFunc, pcFile, iLine);
		iCnt += vfprintf(pOut, fmt, vap);
		va_end(vap);
		sg_LogArgs.iSize += iCnt;
	}

	pthread_mutex_unlock(&sg_LogArgs.sLockMutex);
}

#ifdef __cplusplus
}
#endif

