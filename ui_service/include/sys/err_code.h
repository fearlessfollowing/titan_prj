#ifndef _ERR_CODE_H_
#define _ERR_CODE_H_

#ifndef ERR_MODULE_HIGH_TEMP    
#define ERR_MODULE_HIGH_TEMP        305     /* 模组温度过高 */
#endif

#ifndef ERR_NO_mSD
#define ERR_NO_mSD                  310     /* 未插入mSD卡 */
#endif

#ifndef ERR_mSD_STORAGE_INSUFF
#define ERR_mSD_STORAGE_INSUFF      311     /* mSD卡存储空间不足 */
#endif

#ifndef ERR_mSD_WRITE_PROTECT
#define ERR_mSD_WRITE_PROTECT       312     /* mSD卡写保护 */
#endif

#ifndef ERR_mSD_WRITE_SPEED_INSUFF
#define ERR_mSD_WRITE_SPEED_INSUFF  313     /* mSD卡速不足 */
#endif

#ifndef ERR_NO_ALLOW_OPERATION
#define ERR_NO_ALLOW_OPERATION      413     /* 不允许的操作 */
#endif

#ifndef ERR_MIC
#define ERR_MIC                     414     /* 麦克风错误 */
#endif


#ifndef ERR_OF_STITCH
#define ERR_OF_STITCH               415     /* 光流拼接错误 */
#endif

#ifndef ERR_HIGH_TEMPERATURE
#define ERR_HIGH_TEMPERATURE        417     /* 温度过高 */
#endif

#ifndef ERR_CAMERA_BUSY
#define ERR_CAMERA_BUSY             418     /* 相机忙 */
#endif

#ifndef ERR_FILE_OPEN_FAILED
#define ERR_FILE_OPEN_FAILED        430     /* 文件打开错误 */
#endif


#ifndef ERR_FILE
#define ERR_FILE                    431     /* 文件错误 */
#endif

#ifndef ERR_LOW_STORAGE_SPACE
#define ERR_LOW_STORAGE_SPACE       432     /* 存储空间不足 */
#endif

#ifndef ERR_NO_STORAGE_DEVICE
#define ERR_NO_STORAGE_DEVICE       433     /* 无存储设备 */
#endif 

#ifndef ERR_LOW_WRITE_SPEED
#define ERR_LOW_WRITE_SPEED         434     /* 存储写入速度慢 */
#endif

#ifndef ERR_MUX_WRITE
#define ERR_MUX_WRITE               435
#endif 

#ifndef ERR_MUX_OPEN_FAILED
#define ERR_MUX_OPEN_FAILED         436
#endif 


#ifndef ERR_NET_RECONECTING
#define ERR_NET_RECONECTING         437 
#endif

#ifndef ERR_NET_RECONNECT_FAILED
#define ERR_NET_RECONNECT_FAILED    438
#endif 


#ifndef ERR_NET_DISCONNECTED
#define ERR_NET_DISCONNECTED        439
#endif 

#ifndef ERR_SYSTEM_STUCK
#define ERR_SYSTEM_STUCK            450
#endif 


#ifndef ERR_OPEN_MODULE_FAILED
#define ERR_OPEN_MODULE_FAILED      460
#endif 

#ifndef ERR_MODULE_FAILED_READ_CMD
#define ERR_MODULE_FAILED_READ_CMD  462
#endif 

#ifndef ERR_MODULE_FAILED_READ_DATA
#define ERR_MODULE_FAILED_READ_DATA 464
#endif 



#endif /* _ERR_CODE_H_ */