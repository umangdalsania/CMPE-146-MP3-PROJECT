#pragma once
#include "gpio.h"
#include "lpc40xx.h"
#include "ssp0.h"
#include "stdbool.h"

#include <stdio.h>

/* LCD Display Pins */
gpio_s lcd__reg_select;
gpio_s lcd__read_write_select;
gpio_s lcd__enable;
gpio_s lcd__db7, lcd__db6, lcd__db5, lcd__db4, lcd__db3, lcd__db2, lcd__db1, lcd__db0;

void lcd__pins_init();
void lcd__init();

void lcd__clock();
void lcd__command(uint8_t command);
void lcd__print(uint8_t character);
void lcd__print_string(const char *song_name);
void lcd__set_position(uint8_t x, uint8_t y);

/* Pin Config */

void RS_bit(bool active__1h);
void RW_bit(bool read__1h);
void DB7_bit(bool active__1h);
void DB6_bit(bool active__1h);
void DB5_bit(bool active__1h);
void DB4_bit(bool active__1h);
void DB3_bit(bool active__1h);
void DB2_bit(bool active__1h);
void DB1_bit(bool active__1h);
void DB0_bit(bool active__1h);
