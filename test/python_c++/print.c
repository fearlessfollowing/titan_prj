#include <stdio.h>

void print(const char* str, int p)
{
	int i = 0;
	printf("len = %d\n", p);
	for (i = 0; i < p*4; i = i + 1)
		printf("i = %d, val = %c\n", i, str[i]);
	printf("\n");
}

