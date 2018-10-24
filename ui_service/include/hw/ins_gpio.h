//
// Created by vans on 17-2-27.
//

#ifndef PROJECT_INS_GPIO_H
#define PROJECT_INS_GPIO_H

int gpio_is_requested(unsigned int gpio);
int gpio_request(unsigned int gpio);
int gpio_free(unsigned int gpio);
int gpio_direction_input(unsigned int gpio);
int gpio_direction_output(unsigned int gpio, int value);
int gpio_get_value(unsigned int gpio);
int gpio_set_value(unsigned int gpio, int value);


#endif //PROJECT_INS_GPIO_H
