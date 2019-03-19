#ifndef _FACTORY_TEST_H_
#define _FACTORY_TEST_H_

#include <thread>
#include <vector>
#include <common/sp.h>
#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <hw/ins_led.h>
#include <common/include_common.h>
#include <hw/oled_module.h>
#include <sys/ins_types.h>

enum {
	FACTORY_CMD_EXIT = 1,
	FACTORY_TEST_LED = 2,
	FACTORY_TEST_AWB = 3,
};


class FactoryTest {

public:
	FactoryTest();
	~FactoryTest();

    void set_light_direct(u8 val);

    void set_light(u8 val);

	void oledTest();
	void awbTest();
	void enterBlcbpc();
	void exitBlcbpc();

private:

	int awbCorrectTest();

	sp<ins_led> mOLEDLight;
    sp<oled_module> mOLEDModule;
	

	void init();
	void deinit();

};


#endif /* _FACTORY_TEST_H_ */
