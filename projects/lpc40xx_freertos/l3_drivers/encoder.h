#pragma once
#include "gpio.h"
#include <stdio.h>

void encoder__init(void);
void encoder__pins_init(void);
void encoder__set_max_position(void);

void encoder__set_as_input(void);
void encoder__turn_on_power(void);
void encoder__reset_index(void);

uint8_t encoder__get_position(void);
uint32_t encoder__get_index(void);
