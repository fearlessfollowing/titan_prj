#include <stdio.h>
#include <stdlib.h>

int main()
{
	const char* p = "15";
	long ret = atol(p);

	printf("ret = %ld\n", ret);
	return 0;
}
