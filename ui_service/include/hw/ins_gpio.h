#ifndef _HW_GPIO_H_
#define _HW_GPIO_H_

enum {
    GPIO_OUTPUT_LOW = 0,
    GPIO_OUTPUT_HIGH = 1,
    GPIO_OUTPUT_MAX
};


enum {
	RESET_LOW_LEVEL = 0,
	RESET_HIGH_LEVEL = 1,
	RESET_MAX_LEVEL,
};


int     gpio_is_requested(unsigned int gpio);
int     gpio_request(unsigned int gpio);
int     gpio_free(unsigned int gpio);
int     gpio_direction_input(unsigned int gpio);
int     gpio_direction_output(unsigned int gpio, int value);
int     gpio_get_value(unsigned int gpio);
int     gpio_set_value(unsigned int gpio, int value);


bool    setGpioOutputState(int iGpioNum, int iOutputState);
bool    resetGpio(int iGPioNum, int iResetLevel, int iResetDuration);

#endif /* _INS_GPIO_H_ */
