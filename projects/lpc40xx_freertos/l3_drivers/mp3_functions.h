#include "stdio.h"
#include "gpio.h"
#include "ssp2.h"
#include "lpc40xx.h"

void mp3_decoder_initialize();

void cs(bool flag);

void sj2_write_to_decoder(uint8_t address, uint16_t data);

void sj2_read_from_decoder(uint8_t command);
