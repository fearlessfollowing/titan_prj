//
// Created by vans on 17-2-14.
//
#include "include_common.h"
#include "ins_i2c.h"
#include "font_ascii.h"
#include "icon_ascii.h"




void test_disp_str()
{
    ssd1306_disp_16_str(test_str,3,3,1,COL_MAX - 3);
    ssd1306_disp_16_str(test_str1,9,19,1,COL_MAX - 9 -1);
    ssd1306_disp_16_str(test_str2,12,35,1,COL_MAX - 12 - 10);
}

//void oled_test()
//{
//    const u8 *test_str = (const u8 *)"higklmnopqrstuvw";
//    const u8 *test_str1 = (const u8 *)".*!%,/";
//    const u8 *test_str2 = (const u8 *)"123.456.789.012";
//    const u8 *test_str_t = (const u8 *)"b";
//    printf("oled_test start\n");
//    printf("sizeof(u64) %zd and sizeof(int64) %zd\n", sizeof(u64),sizeof(int64));
//    ssd1306_init();
//    printf("oled_test start2\n");
//    ssd1306_disp_16_str(test_str,0,0);
////    test_hotrizon_scroll(0, 0, COL_MAX,2);
////    test_disp_str();
////    SSD_Set_Address_Mode(0);
////    SSD_Set_Column_Address(8,100);
////    SSD_Set_Horizontal_Scroll(1, 0, FRAME_2,2);
////    SSD_Set_Activate_Scroll();
////    ssd1306_disp_icon(wifi_open_0000_16x16,0,3,16,16);
////    ssd1306_disp_icon(wifi_open_0000_16x16,0,19,16,16);
////    ssd1306_disp_icon(video_waiting_2016_76x32,0,35,76,32);
//    printf("oled_test start3\n");
//}



