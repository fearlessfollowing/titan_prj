#ifndef _FACTORY_TEST_H_
#define _FACTORY_TEST_H_

#include <thread>
#include <vector>
#include <common/sp.h>
#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <hw/oled_light.h>
#include <common/include_common.h>
#include <hw/oled_module.h>

enum {
	FACTORY_CMD_EXIT = 1,
	FACTORY_TEST_LED = 2,
};


class FactoryTest {

public:
	explicit FactoryTest(const sp<ARMessage> &notify);
	~FactoryTest();

    sp<ARMessage> obtainMessage(uint32_t what);

	void handleMessage(const sp<ARMessage> &msg);

private:


	std::thread th_msg_;
	sp<oled_light> mOLEDLight;
    sp<oled_module> mOLEDModule;
	
	sp<ARLooper> mLooper;
    sp<ARHandler> mHandler;
	//sp<InputManager> mInputManager;
    sp<ARMessage> mNotify;


	void init_handler_thread();

	void init();

	void set_light_direct(u8 val);

	void set_light(u8 val);

	void deinit();

    u8 last_light = 0;
    u8 fli_light = 0;
    u8 front_light;

	void exitAll();


    bool bExitMsg;
    void sendExit();
};


#endif /* _FACTORY_TEST_H_ */
