/*
 * external/libfm1388/errorcode.h
 * (C) Copyright 2014-2018
 * Fortemedia, Inc. <www.fortemedia.com>
 * Author: HenryZhang <henryhzhang@fortemedia.com>;
 * 			LiFu <fuli@fortemedia.com>
 *
 * This program is dynamic library which privode interface to 
 * let fm_fm1388 application communicate with FM1388 driver.
 */
#ifndef LIB_FM_ERRORCODE_H_
#define LIB_FM_ERRORCODE_H_

#define ESUCCESS			0
#define EUNKNOWN			10000
#define EDUPOPEN			10001
#define EFAILOPEN			10002
#define ENOTOPEN			10003
#define EPARAMINVAL			10004
#define EDATAINVAL			10005
#define ESPIREAD			10006
#define ESPIWRITE			10007
#define ESPISETFAIL			10008
#define EDSPNOTWORK			10009
#define ENOMEMORY			10010
#define EMEMFAULT			10011
#define ECANNOTENRECORD		10012
#define ENOSDCARD			10013
#define EFILECANNOTWRITE	10014
#define EINPROCESSING		10015
#define EFAILTOSETMODE		10016
#define ECOMMANDINVAL		10017
#define EFILECANNOTREAD		10018
#define ECANNOTENPLAY		10019
#define ENOENOUGHDATA		10020

#endif /*LIB_FM_ERRORCODE_H_ */
