#include "ssp0.h"

void ssp0_pin_init() {
  gpio_s sck = gpio__construct_with_function(GPIO__PORT_0, 15, GPIO__FUNCTION_2);
  gpio_s miso = gpio__construct_with_function(GPIO__PORT_0, 17, GPIO__FUNCTION_2);
  gpio_s mosi = gpio__construct_with_function(GPIO__PORT_0, 18, GPIO__FUNCTION_2);

  gpio__set_as_output(sck);
  gpio__set_as_output(mosi);
  gpio__set_as_input(miso);
}

void ssp0__init(uint32_t max_clock_mhz) {
  // Power on Peripheral
  lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__SSP0);

  LPC_SSP0->CR0 = 7;
  LPC_SSP0->CR1 = (1 << 1);

  ssp0__set_clock(max_clock_mhz);
  ssp0_pin_init();
}

void ssp0__set_clock(uint32_t max_clock_mhz) {
  const uint32_t cpu_clock_khz = clock__get_core_clock_hz() / 1000UL;
  const uint8_t reg_max_value = 254;
  const uint32_t max_clock_khz = max_clock_mhz * 1000;
  uint8_t divider = 2;

  while (max_clock_khz < (cpu_clock_khz / divider) && divider <= reg_max_value) {
    divider += 2;
  }

  LPC_SSP0->CPSR = divider;
}

uint8_t ssp0__exchange_byte(uint8_t data_out) {
  // DR (data register) is used for write and read
  LPC_SSP0->DR = data_out;

  while (LPC_SSP0->SR & (1 << 4)) {
    // wait while busy
  }

  return (uint8_t)(LPC_SSP0->DR & 0xFF);
}