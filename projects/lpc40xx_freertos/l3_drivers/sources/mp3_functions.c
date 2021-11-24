#include "mp3_functions.h"
#include "delay.h"
#include "encoder.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "ssp0.h"

#define SCI_MODE 0x0
#define SCI_CLOCKF 0x3
#define SCI_VOLUME 0xB

#define DEBUG 0

/* MP3 Decoder Pins */
static gpio_s mp3_dreq;
static gpio_s mp3_xcs;
static gpio_s mp3_xdcs;
static gpio_s mp3_reset;

void mp3__pins_init(void) {
  mp3_xcs = gpio__construct_with_function(GPIO__PORT_2, 7, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(mp3_xcs);
  mp3__ds();

  mp3_xdcs = gpio__construct_with_function(GPIO__PORT_2, 9, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(mp3_xdcs);
  mp3__data_ds();

  mp3_dreq = gpio__construct_with_function(GPIO__PORT_2, 8, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_input(mp3_dreq);

  mp3_reset = gpio__construct_with_function(GPIO__PORT_0, 16, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(mp3_reset);
  gpio__set(mp3_reset);
}

void mp3__init(void) {
  /* Variable Declarations */
  uint16_t default_settings = 0x4800;
  uint16_t freq_3x_multiplier = 0x6000;

  /* Initializing Decoder + SPI Port 0 */
  mp3__pins_init();
  mp3__reset();
  ssp0__init(1);
  delay__ms(100);

  sj2_write_to_decoder(SCI_MODE, default_settings);
  delay__ms(100);

  sj2_write_to_decoder(SCI_CLOCKF, freq_3x_multiplier);
  delay__ms(100);

#if DEBUG
  printf("Status Read: 0x%04x\n", sj2_read_from_decoder(0x01));
  printf("Mode Read: 0x%04x\n", sj2_read_from_decoder(0x00));
#endif

  sj2_write_to_decoder(SCI_VOLUME, 0x0000);
}

void sj2_write_to_decoder(uint8_t reg, uint16_t data) {
  mp3__cs();
  ssp0__exchange_byte(0x2);
  ssp0__exchange_byte(reg);
  ssp0__exchange_byte((data >> 8) & 0xFF);
  ssp0__exchange_byte((data >> 0) & 0xFF);
  mp3__ds();
}

uint16_t sj2_read_from_decoder(uint8_t reg) {
  uint16_t results = 0;

  mp3__cs();
  ssp0__exchange_byte(0x3);
  ssp0__exchange_byte(reg);
  results |= (ssp0__exchange_byte(0xFF) << 8);
  results |= (ssp0__exchange_byte(0xFF) << 0);
  mp3__ds();

  return results;
}

void sj2_to_mp3_decoder(char data) {
  mp3__data_cs();
  ssp0__exchange_byte(data);
  mp3__data_ds();
}

void mp3__reset(void) {
  gpio__reset(mp3_reset); // Reset Pin Is Active Low
  delay__ms(200);
  gpio__set(mp3_reset);
}

void mp3__volume_adjuster(void) {
  double volume_value = mp3__get_volume_value();

  uint8_t temp = 254 * (1 - volume_value);

  uint16_t volume_to_decoder = (temp << 8) | (temp << 0);

  // printf("Volume to Decoder = %x.\n", volume_to_decoder);

  sj2_write_to_decoder(SCI_VOLUME, volume_to_decoder);
}

double mp3__get_volume_value(void) {
  uint32_t get_index = encoder__get_index();

  // printf("Get Index Value: %li.\n", get_index);

  if (get_index < 0) {
    // Reset if Index is neg ; no such thing as neg volume
    encoder__reset_index();
    return 0;
  }

  if (get_index > 100)
    return 1;

  return (get_index / 100.0);
}

void mp3__cs(void) { gpio__reset(mp3_xcs); }
void mp3__ds(void) { gpio__set(mp3_xcs); }

void mp3__data_cs(void) { gpio__reset(mp3_xdcs); }
void mp3__data_ds(void) { gpio__set(mp3_xdcs); }
