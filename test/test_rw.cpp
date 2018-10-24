#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

static bool is_fs_rw(const char* path)
{
	char tmp_file[512];
	bool ret = false;
	int fd;
	
	memset(tmp_file, 0, sizeof(tmp_file));
	sprintf(tmp_file, "%s/tmpfile", path);

	printf("create tmp file [%s]\n", tmp_file);
//	Log.e(TAG, "create tmp file [%s]\n", tmp_file);
	if ((fd = open(tmp_file, O_CREAT | O_RDWR, 0666)) < 0) {
//		Log.e(TAG, "create tmpfile failed...\n");
 	} else {
 		ret = true;
		close(fd);
//		unlink(tmp_file);
	}
	
	return ret;
}


int main(int argc, char* argv[])
{

	bool ret = is_fs_rw("/var");
	printf("file system is %s\n", (ret == true) ? "rw": "ro");
	return 0;
}


