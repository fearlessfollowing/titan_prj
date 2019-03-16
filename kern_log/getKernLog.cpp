
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
#include <log/log_wrapper.h>
#include <system_properties.h>
#include <prop_cfg.h>

#undef  TAG
#define TAG "KernLog"


int main(int argc, char **argv)
{
    int iRet;
    int iFd;
    int iReadLen = 0;
    char buf[1024] = {0};

	iRet = __system_properties_init();		/* 属性区域初始化 */
	if (iRet) {
		fprintf(stderr, "kern_log service exit: __system_properties_init() faile, ret = %d", iRet);
		return -1;
	}

    LogWrapper::init(DEFAULT_LOG_FILE_PATH_BASE, "kern_log", true);

    iFd = open("/proc/kmsg", O_RDONLY);
    if (iFd < 0) {
        LOGERR(TAG, "open /proc/kmsg failed.");
    } else {
        while (true) {
            iReadLen = read(iFd, buf, sizeof(buf));
            buf[iReadLen - 1] = '\0';
            LOGDBG(TAG, "%s", buf);
        }
        close(iFd);
    }
    return 0;
}