#ifndef PROJECT_OLED_LIGHT_H
#define PROJECT_OLED_LIGHT_H

#include <sys/ins_types.h>
#include <common/sp.h>

class ins_i2c;

class ins_led {
public:
    explicit    ins_led();
                ~ins_led();

    void        set_light_val(u8 val);
	int         factory_test(int icnt = 3);

    void        setAllLight(int iOnOff);

    void        close_all();
    
    void        power_off_all();

private:
    void        init();
    void        deinit();

    u8          mRestoreLedVal = 0;
    u8          light_restore_val = 0x7;

    sp<ins_i2c> mI2CLight;
}; 

#endif
