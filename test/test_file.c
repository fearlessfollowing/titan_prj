#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>


int main(int argc, char* argv[])
{
	char const*path = argv[1];

	struct stat s_buf;

 
	/*获取文件信息，把信息放到s_buf中*/

	stat(path,&s_buf);

	if (S_ISREG(s_buf.st_mode)) {
		printf("[%s] is a regular file\n", path);
	} else {
		printf("[%s] is not a regular file\n", path);
	}

	return 0;
}


