#include <unistd.h>
#include <stdio.h>
#include <android/log.h>
#include <log/log.h>

#define TAG "test_log"

int main(int argc, char* argv[])
{

	int i;

	for (i = 0; i < 100; i++) {
		__android_log_print(LOG_ID_MAIN, TAG, "test log write: %d times\n", i);
		sleep(1);
	}

	return 0;
}
