#include "mp3_functions.h"

void mp3_decoder_initialize() {
  /* Variable Declarations */
  uint8_t mode_reg = 0x0;
  uint8_t clockf_reg = 0x3;

  uint16_t sm_sdinew = 0x800;
  uint8_t sm_cancel = 0x4;

  uint16_t freq_3x_multiplier = 0x6000;

  /* Configuring Required Pins */
  gpio_s chip_select = gpio__construct_as_output(0, 26);
  gpio_s data_select = gpio__construct_as_output(1, 31);
  gpio_s dreq = gpio__construct_as_input(1, 20);

  ssp2_pin_initialize();
  ssp2__initialize(1000);

  gpio__set(chip_select);
  gpio__set(data_select);

  delay__ms(100);
  sj2_write_to_decoder(mode_reg, sm_sdinew | sm_cancel);
  delay__ms(200);
  sj2_write_to_decoder(clockf_reg, freq_3x_multiplier);
}

void sj2_write_to_decoder(uint8_t reg, uint16_t data) {
  cs_c();
  ssp2__exchange_byte(0x2);
  ssp2__exchange_byte(reg);
  ssp2__exchange_byte((data >> 8) & 0xFF);
  ssp2__exchange_byte((data >> 0) & 0xFF);
  cs_c();
}

uint16_t sj2_read_from_decoder(uint8_t addr) {
  uint16_t results = 0;

  cs_c();
  ssp2__exchange_byte(0x3);
  ssp2__exchange_byte(addr);
  results |= (ssp2__exchange_byte(0xFF) << 8);
  results |= (ssp2__exchange_byte(0xFF) << 0);
  ds_c();

  return results;
}

void sj2_to_mp3_decoder(char data) {
  cs_d();
  ssp2__exchange_byte(data);
  ds_d();
}

void cs_c() { LPC_GPIO0->CLR = (1 << 26); }

void ds_c() { LPC_GPIO0->SET = (1 << 26); }

void cs_d() { LPC_GPIO1->CLR = (1 << 26); }

void ds_d() { LPC_GPIO1->SET = (1 << 26); }