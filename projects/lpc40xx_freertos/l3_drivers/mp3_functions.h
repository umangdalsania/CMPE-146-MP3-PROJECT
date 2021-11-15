#include "delay.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "ssp2.h"
#include <stdio.h>

void mp3_decoder_initialize();

void sj2_write_to_decoder(uint8_t reg, uint16_t data);

uint16_t sj2_read_from_decoder(uint8_t addr);

void sj2_to_mp3_decoder(char byte);

void cs_c();
void ds_c();

void cs_d();
void ds_d();
