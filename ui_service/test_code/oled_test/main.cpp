
#include <iostream>
#include <stdio.h>
#include <sys/ins_types.h>
//#include <hw/oled_handler.h>
#include <hw/oled_module.h>
#include <common/sp.h>


int main(int argc, char* argv[]) 
{

	sp<oled_module> mOLEDModule = nullptr;
	mOLEDModule = sp<oled_module>(new oled_module());

	const u8* ip = (const u8*)"182.233.323.3";
	mOLEDModule->disp_ip(ip);

	
	return 0;
}

