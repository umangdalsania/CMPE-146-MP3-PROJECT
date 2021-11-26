#include <stdio.h>

typedef void (*function_pointer_t)(void);

void mp3__init(void);
void mp3__pins_init(void);
void mp3__reset(void);

void sj2_write_to_decoder(uint8_t reg, uint16_t data);
uint16_t sj2_read_from_decoder(uint8_t addr);

void sj2_to_mp3_decoder(char byte);
void mp3__volume_adjuster(void);

void mp3__cs(void);
void mp3__ds(void);
void mp3__data_cs(void);
void mp3__data_ds(void);

double mp3__get_volume_value(void);

void mp3__ISR_init(void);
void mp3__interrupt_dispatcher(void);
void mp3__update_qei_interrupts(void);
void mp3__volume_isr(void);
