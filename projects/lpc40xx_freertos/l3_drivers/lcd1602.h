#pragma once
#include "gpio.h"
#include "lpc40xx.h"
#include "ssp0.h"
#include <stdio.h>

/* LCD Display Pins */
 gpio_s lcd__reg_select; 
 gpio_s lcd__read_write_select;
 gpio_s lcd__enable;
 gpio_s lcd__db7, lcd__db6, lcd__db5, lcd__db4,
            lcd__db3, lcd__db2, lcd__db1, lcd__db0;

void lcd__pins_init();
void lcd__init();

void lcd__clock();
void lcd__command(uint8_t command );
/* Pin Config */

void RS_bit(int active__1h);
void Read_Write_bit(int read__1h);

void DB7_bit(int active__1h);
void DB6_bit(int active__1h);
void DB5_bit(int active__1h);
void DB4_bit(int active__1h);
void DB3_bit(int active__1h);
void DB2_bit(int active__1h);
void DB1_bit(int active__1h);
void DB0_bit(int active__1h);
