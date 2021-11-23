#pragma once
#include "gpio.h"
#include <stdio.h>

gpio_s center_button, down_button, right_button, up_button, left_button;

void encoder__init(void);
void encoder__pins_init(void);
void encoder__set_as_input(void);
