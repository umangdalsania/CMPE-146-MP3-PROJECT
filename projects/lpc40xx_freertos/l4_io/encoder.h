#pragma once
#include "gpio.h"

void encoder__init(void);
void encoder__pins_init(void);
void encoder__set_max_position(void);

void encoder__set_as_input(void);
void encoder__turn_on_power(void);

uint32_t encoder__get_index(void);

gpio_s get_extra_button(void);
gpio_s get_center_button(void);
gpio_s get_down_button(void);
gpio_s get_left_button(void);
gpio_s get_right_button(void);
gpio_s get_up_button(void);