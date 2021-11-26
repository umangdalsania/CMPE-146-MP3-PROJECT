#pragma once

#include "stdbool.h"
#include <stdio.h>

void lcd__pins_init(void);
void lcd__init(void);

void lcd__clock(void);
void lcd__command(uint8_t command);
void lcd__clear(void);
void lcd__print(uint8_t character);
void lcd__print_string(const char *song_name, int line);
void lcd__print_helper(const char *song_name);
void lcd__set_position(uint8_t x, uint8_t y);
void lcd__print_arrow(int line);

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
