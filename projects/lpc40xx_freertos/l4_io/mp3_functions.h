#pragma once

#include "gpio.h"
#include <stdio.h>

typedef void (*function_pointer_t)(void);

void mp3__init(void);
void mp3__pins_init(void);
void mp3__reset(void);

void sj2_write_to_decoder(uint8_t reg, uint16_t data);
uint16_t sj2_read_from_decoder(uint8_t addr);

void sj2_to_mp3_decoder(char byte);

void mp3__volume_adjuster(void);
double mp3__get_volume_value(void);

void mp3__cs(void);
void mp3__ds(void);
void mp3__data_cs(void);
void mp3__data_ds(void);

void mp3__interrupt_init(void);
void mp3__interrupt_dispatcher(void);
void mp3__attach_interrupt(gpio_s button, function_pointer_t callback);
void mp3__check_pin(int *pin_num);
void mp3__clear_interrupt(int pin_num);

// WIP -> Volume
void mp3__update_qei_interrupts(void);
void mp3__volume_isr(void);