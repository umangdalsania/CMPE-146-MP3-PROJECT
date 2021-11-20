#pragma once
#include "gpio.h"
#include "lpc40xx.h"
#include "ssp0.h"
#include <stdio.h>

/* LCD Display Pins */
 gpio_s lcd__reg_select; 
 gpio_s lcd__read_write_select;
 gpio_s lcd__enable;
 gpio_s lcd__db7, lcd__db6, lcd__db5, lcd__db4;
