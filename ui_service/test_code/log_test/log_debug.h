/****************************************************************************************
�ļ����ƣ�debug_log.h
�� �ߣ��޾���
˵ ������־����
�޸ļ�¼��

��ǰ�汾��1.2  �� �ߣ��޾���  ���ڣ�2012.02.17
˵ �����޸�LOG������д��־ʱ���������Ϣ

��һ�汾��1.1  �� �ߣ��޾���  ���ڣ�2011.08.10
˵ �����޸��ļ���ʽ�ʹ���ע��

��һ�汾��1.0  �� �ߣ��޾���  ���ڣ�2011.08.09
˵ ����1���ṩ�� ����Ĭ�ϵ���־�������LOG_DIRECT�ͼ���LOG_LEVEL
	  2������ȡ�����ļ�[system]->[loglevel] [system]->[logdirect] ʧ��ʱȡ����Ĭ�ϵĺ�
	  LOG_DIRECT/LOG_LEVEL
	  3����ʼ��ʱ�ṩ���LOG�ļ������ļ���󳤶ȣ�Ĭ��Ϊ5��1M���ļ�,�ļ���Ϊ��
	  ipcam_log_01.log --ipcam_log_05.log
	  4��ipcam_log_id.dat ������ǵ�ǰ��������д����ļ��������ļ������� 1-max ѭ����
	  5��Ĭ�ϴ�ӡ������DEBUG���������ΪTTY
****************************************************************************************/

#ifndef __DEBUG_LOG_H__
#define __DEBUG_LOG_H__

#include <stdarg.h>

typedef enum 
{
	LOG_ERR = 1,	/* ���ش��� */
	LOG_WAR = 2,	/* ���� */
	LOG_NOT = 4,	/* ֪ͨ */
	LOG_DBG = 8,	/* ֻ�ǵ����õ���־ */
	LOG_LAST = 16,
}LOG_LEV_EN;

#define LOG_TTY		0x00 /* ���� */
#define LOG_FLASH	0x01 /* FLASH */

/* ����Ĭ�ϵ���־�������ͼ��𣬵��������ļ�ʧ��ʱ��ȡ��ֵ */
#define LOG_LEVEL	LOG_DBG

#define LOGPARAM_INIT     0x00  /* ��һ��������־���� */
#define LOGPARAM_UPDATE 0x01  /* ������־���� */

/****************************************************************************************
�������ƣ�LOG
�������ܣ���־��ӡ����
��ڲ�����iLevel--LOG_ERR ���ش���LOG_WAN ����LOG_DBG ֻ�ǵ����õ���־
		iLine--Log��Ϣ������
		pcFile--Log��Ϣ�����ļ�
		pcFunc--Log��Ϣ���ں���
		fmt--Log��Ϣ��ʽ�����
���ڲ�������
����ֵ����
****************************************************************************************/
void LOG(int iLevel, int iLine, char *pcFile, const char *pcFunc, const char *fmt, ...);

/****************************************************************************************
�������ƣ�LogInit
�������ܣ���Ҫ��ʼ��дLog�ļ��е�һЩ����
��ڲ�����pcLogPath--Log�ļ�·������:/irlab/log/
		pcLogPrefix--Log�ļ���ǰ׺
		iMaxFileSize--���Log�ļ���
		iMaxFileCount--��ȡ0ʱ����ʾȡĬ��ֵ 5�����Ϊ1M���ļ�
     	iLevel--��ӡ����
			   0x01 ���ش��� 
			   0x02 ���� 
	             0x04 ֪ͨ
	             0x08 ֻ�ǵ����õ���־	                                      
	     iDirect--�Ƿ�д��־�ļ�������ֱ���������̨
	              0x00 ����
	              0x01 FLASH
	     iParamChangeState--���� or ��һ�γ�ʼ����־����.
	              0x00 ��һ��������־����
	              0x01 ������־����
���ڲ�������
����ֵ���ɹ�����0��ʧ�ܷ���1
****************************************************************************************/
int LogInit(char *pcFilePath, char *pcFilePrefix, int iMaxFileSize,\
                    int iMaxFileCount, int iLevel, int iDirect, int iParamChangeState);

/****************************************************************************************
�������ƣ�LogDestroy
�������ܣ��ͷ�LOG��Դ
��ڲ�������
���ڲ�������
����ֵ���ɹ�����0��ʧ�ܷ���1
****************************************************************************************/
void LogDestroy(void);

#ifdef __DEBUG_VERSION__
/* ��Ϊ���԰汾����־����Ҫ��ӡ��������� */
#define LOGERR(fmt,args...)   LOG(LOG_ERR, __LINE__,__FILE__,__FUNCTION__,fmt,## args)
#define LOGWAR(fmt,args...)  LOG(LOG_WAR,__LINE__,__FILE__,__FUNCTION__,fmt,## args)
#define LOGNOT(fmt,args...)   LOG(LOG_NOT,__LINE__,__FILE__,__FUNCTION__,fmt,## args)
#define LOGDBG(fmt,args...)   LOG(LOG_DBG,__LINE__,__FILE__,__FUNCTION__,fmt,## args)
#else
/* ��Ϊ�����汾��ֻ��ӡ������־,����CPUʹ���ʹ��� */
#define LOGERR(fmt,args...)   LOG(LOG_ERR, __LINE__,__FILE__,__FUNCTION__,fmt,## args)
#define LOGWAR(fmt,args...)  
#define LOGNOT(fmt,args...)   
#define LOGDBG(fmt,args...)  
#endif

//#define     LOGFTPDBG(fmt,args...)   LOG(LOG_DBG,"FTP",__LINE__,__FILE__,__FUNCTION__,fmt,## args)
#endif

