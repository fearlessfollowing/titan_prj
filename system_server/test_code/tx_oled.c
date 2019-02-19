#include <stdio.h>

#include "tx_oled.h"
#include "tx_key.h"
#include "tx_dzlib.h"



int main(int argc,char *argv[])
{
	key_event ev[3];
	
	tx_oled_init();
	
	tx_key_init();
	
	int i = 0 ,j;
	
	while(1)
	{
		tx_olde_draw_icon_ex(i++ % 6);
		tx_key_read(ev);
		for(j = 0; j <3 ; j++)
		{	
			printf("type = %d code = %d, value =%d\n",ev[j].type, ev[j].code, ev[j].value);
		}
	#if 0	
		tx_oled_cls();
		//tx_oled_drawbmp(0,0,BMP3,sizeof(BMP3));
		//sleep(1);
		//tx_oled_cls();
		//tx_oled_drawstr(0,0,"test_1",strlen("test_1"));
		tx_oled_drawicon(22, 00, 12, 16 ,stateWifiOff_2200_12x16 ,sizeof(stateWifiOff_2200_12x16));
		sleep(2);
		printf("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n");
		tx_oled_drawicon(13, 16, 32, 48 ,logoPhoto_1316_32x48 ,sizeof(logoPhoto_1316_32x48));
		sleep(3);
		printf("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\n");
	#endif	
		sleep(1);
	}	
	
	tx_oled_exit();
	tx_key_exit();
	return 0;
}