#ifndef PROJECT_OLED_MODULE_H
#define PROJECT_OLED_MODULE_H

#include <sys/ins_types.h>
#include <common/sp.h>

#define USE_U64

#ifdef USE_U64
#define DAT_TYPE u64
#define DAT_FORMAT "0x%llx"
#else
#define DAT_TYPE u32
#define DAT_FORMAT "0x%x"
#endif

typedef struct _icon_info_ {
    u8 x;
    u8 y;
    u8 w;
    u8 h;

	//not used yet
    u32 size;
    const u8 *dat;
} ICON_INFO;



class ins_i2c;
struct _char_info_;
struct _icon_info_;
struct _page_info_;

class oled_module
{
public:
    oled_module();
    ~oled_module();
    void disp_ip(const u8 *ip);
    void clear_icon(const u32 icon_type);
    void disp_icon(const u32 icon_type);
    void disp_icon(const struct _icon_info_ *pstTmp);
    void fill(const u8 x, const u8 y, const u8 w, const u8 h,const u8 dat);
    //fill width default to (COL_MAX - x)
    void ssd1306_disp_16_str_fill(const u8 *str,const u8 x, const u8 y, bool bHigh);
    void ssd1306_disp_16_str(const u8 *str,const u8 x, const u8 y, bool bHigh = false,u8 width = 0);
    void clear_area(const u8 x,const u8 y,const u8 w,const u8 h);
    void clear_area(const u8 x,const u8 y);
    void clear_area_w(const u8 x,const u8 y,const u8 w);
    void clear_area_h(const u8 x,const u8 y,const u8 h);

	void display_onoff(u8 sw);
	
private:
    void init();
    void deinit();
    void ssd1306_reset();
    void SSD_Set_Start_Column(u8 addr);
    void SSD_Set_Start_Page(u8 addr);
    void SSD_Set_Column_Address(u8 start, u8 end);
    void SSD_Set_Page_Address(u8 start, u8 end);
    void SSD_Set_Display_ON_OFF(u8 attr);
    void SSD_Set_Start_Line(u8 line);
    void SSD_Set_Address_Mode(u8 mode);
    void SSD_Set_Contrast(u8 dat);
    void SSD_Set_Segment_Remap(u8 attr);
    void SSD_Set_Common_Remap(u8 attr);
    void SSD_Set_Multiplex_Ratio(u8 dat);
    void SSD_Set_Entire_Display(u8 attr);
    void SSD_Set_Inverse_Display(u8 attr);
    void SSD_Set_Display_Offset(u8 dat);
    void SSD_Set_Display_Clock(u8 dat);
    void SSD_Set_Precharge_Period(u8 dat);
    void SSD_Set_Segment_Config(u8 left_right, u8 alternative);
    void SSD_Set_VCOM_Level(u8 level);
    void SSD_Set_Charge_Pump(u8 val);
    void SSD_Set_Deactivate_Scroll(void);
    void SSD_Set_Nop(void);
    void SSD_Set_Horizontal_Scroll(u8 LR, u8 start_page, u8 interval, u8 end_page);
    void SSD_Set_Vertical_Scroll(u8 LR, u8 start_page, u8 interval, u8 end_page,u8 offset);
    void SSD_Set_Vertical_Scroll_Area(u8 offset, u8 pages);
    void SSD_Set_Activate_Scroll(void);
    void SSD_Set_RAM_Address(u8 pag, u8 col);
    void ssd1306_fill(const u8 dat);
    void ssd1306_set_off();
    void ssd1306_set_on();
    void ssd1306_init();
    void disp_dat(u8 col, u8 page, u8 col_w,u8 page_num);
    void set_buf(u8 col, u8 page, u8 dat);
    void set_buf(u8 col,u8 page, const u8 *dat, u8 len);
    void set_buf(const u8 dat,u8 col, u8 page_start, u8 col_w,u8 page_num);
    void get_page_info_convert(u8 y,u8 h,sp<struct _page_info_> &mPageInfo);
    void get_char_info(u8 c,sp<struct _char_info_> &mCharInfo, bool high = 0);
    void set_buf_by_page_info(u8 col_start, DAT_TYPE ucMid,sp<struct _page_info_> mPageInfo);
    void ssd1306_disp_icon(const u8 *dat,const u8 x, const u8 y,const u8 w,const u8 h);
    void clear_area_page(const u8 col,const u8 page,const u8 col_w,const u8 page_num);
    void ssd1306_write_cmd(const u8 cmd);
    void ssd1306_write_dat(u8 dat);

    sp<ins_i2c> pstI2C_OLED;

    // byte0 --page0,col0 byte1 -- page0 col1  ...
    u8  *ucBuf;
    u8  *ucBufLast;
    u8  mTmpBuf[32];
    sp<struct _page_info_> mPageInfo;
};


#endif //PROJECT_OLED_MODULE_H
