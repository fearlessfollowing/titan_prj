#include <android/log.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define TAG "skyway-test"
int main(int argc, char* argv[])
{
	int cnt = 0;
	int prio = ANDROID_LOG_DEFAULT;
	int ret = 0;
	char buf[256];
	
	while (1) {
		
//		sprintf(buf, "just test, write %d count", cnt++);
//		printf("%s\n", buf);
		ret = __android_log_print(prio, TAG, "just test, write %d count\n", cnt++);
		if (ret < 0) {
			printf("write log failed, reason: %s\n", strerror(errno));
			break;
		}
		prio++;
		
		if (ANDROID_LOG_ERROR == prio)
			prio = ANDROID_LOG_DEFAULT;
		
		sleep(1);
		
	}
	return 0;
}



