#include <stdio.h>


int main()
{
	float fr = 0.0f;
	unsigned int totalsize = 1;
	unsigned int leftsec = 0;

	fr = 4096 * 2 / (8*1024* 1.0f);
	leftsec = (unsigned int) (totalsize / fr);
	printf("left sec = %d", leftsec);

}
