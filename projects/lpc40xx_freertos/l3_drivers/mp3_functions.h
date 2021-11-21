#include <stdio.h>

void mp3__init(void);
void mp3__pins_init(void);

void sj2_write_to_decoder(uint8_t reg, uint16_t data);

uint16_t sj2_read_from_decoder(uint8_t addr);

void sj2_to_mp3_decoder(char byte);

void mp3__cs(void);
void mp3__ds(void);

void mp3__data_cs(void);
void mp3__data_ds(void);

void mp3__reset(void);
