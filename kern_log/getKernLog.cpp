
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/statfs.h>
#include <sys/stat.h>
#include <sys/ins_types.h>
#include <util/msg_util.h>
#include <common/sp.h>
#include <sys/sig_util.h>
#include <update/update_util.h>
#include <update/dbg_util.h>
#include <log/stlog.h>
#include <log/arlog.h>
#include <system_properties.h>


#undef  TAG
#define TAG "KernLog"

#define KERN_LOG_PATH "/home/nvidia/insta360/log/kern_log"

int main(int argc, char **argv)
{
    int iRet;
    int iFd;
    int iReadLen = 0;
    char buf[1024] = {0};

    registerSig(default_signal_handler);
    signal(SIGPIPE, pipe_signal_handler);

	arlog_configure(true, true, KERN_LOG_PATH, false);	/* 配置日志 */

	iRet = __system_properties_init();		/* 属性区域初始化 */
	if (iRet) {
		Log.e(TAG, "update_check service exit: __system_properties_init() faile, ret = %d", iRet);
		return -1;
	}

    iFd = open("/proc/kmsg", O_RDONLY);
    if (iFd < 0) {
        Log.e(TAG, "[%s: %d] open /proc/kmsg failed.", __FILE__, __LINE__);
        arlog_close();
    } else {
        while (true) {
            iReadLen = read(iFd, buf, sizeof(buf));
            buf[iReadLen - 1] = '\0';
            Log.d(TAG, "%s", buf);
        }
        
        close(iFd);
    }

    return 0;
}