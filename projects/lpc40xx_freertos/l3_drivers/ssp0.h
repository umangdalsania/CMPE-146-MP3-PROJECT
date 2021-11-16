#include "gpio.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"

void ssp0__init(uint32_t max_clock_mhz);
void ssp0_pin_init();

void ssp0__set_clock(uint32_t max_clock_mhz);
uint8_t ssp0__exchange_byte(uint8_t data_out);
