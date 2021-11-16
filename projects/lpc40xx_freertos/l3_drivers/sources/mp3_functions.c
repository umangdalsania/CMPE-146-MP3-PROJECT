#include "mp3_functions.h"

static gpio_s mp3_dreq;
static gpio_s mp3_xcs;
static gpio_s mp3_xdcs;
static gpio_s mp3_reset;

void mp3_decoder_initialize() {
  /* Variable Declarations */
  uint8_t mode_reg = 0x0;
  uint8_t clockf_reg = 0x3;

  uint16_t sdinew_mode = 0x0800;

  uint16_t freq_3x_multiplier = 0x3000;

  /* Configuring Required Pins */

  mp3_dreq = gpio__construct_with_function(GPIO__PORT_1, 30, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_input(mp3_dreq);

  mp3_xcs = gpio__construct_with_function(GPIO__PORT_0, 26, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(mp3_xcs);
  mp3_ds();

  mp3_xdcs = gpio__construct_with_function(GPIO__PORT_1, 31, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(mp3_xdcs);
  mp3_data_ds();

  mp3_reset = gpio__construct_with_function(GPIO__PORT_0, 25, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(mp3_reset);
  gpio__set(mp3_reset);

  // Reseting MP3 Decoder
  mp3__reset(true);
  delay__ms(100);
  mp3__reset(false);

  ssp0__init(1);

  delay__ms(100);

  printf("Status Read: 0x%04x\n", sj2_read_from_decoder(0x01));

  sj2_write_to_decoder(mode_reg, sdinew_mode);

  printf("Mode Read: 0x%04x\n", sj2_read_from_decoder(0x00));

  delay__ms(100);
  sj2_write_to_decoder(clockf_reg, freq_3x_multiplier);
  delay__ms(100);
}

void sj2_write_to_decoder(uint8_t reg, uint16_t data) {
  mp3_cs();
  ssp0__exchange_byte(0x2);
  ssp0__exchange_byte(reg);
  ssp0__exchange_byte((data >> 8) & 0xFF);
  ssp0__exchange_byte((data >> 0) & 0xFF);
  mp3_ds();
}

uint16_t sj2_read_from_decoder(uint8_t reg) {
  uint16_t results = 0;

  mp3_cs();
  ssp0__exchange_byte(0x3);
  ssp0__exchange_byte(reg);
  results |= (ssp0__exchange_byte(0xFF) << 8);
  results |= (ssp0__exchange_byte(0xFF) << 0);
  mp3_ds();

  return results;
}

void sj2_to_mp3_decoder(char data) {
  mp3_data_cs();
  ssp0__exchange_byte(data);
  mp3_data_ds();
}

void mp3_cs(void) { gpio__reset(mp3_xcs); }
void mp3_ds(void) { gpio__set(mp3_xcs); }

void mp3_data_cs(void) { gpio__reset(mp3_xdcs); }
void mp3_data_ds(void) { gpio__set(mp3_xdcs); }

void mp3__reset(bool flag) {
  if (flag)
    gpio__reset(mp3_reset);
  else
    gpio__set(mp3_reset);
}