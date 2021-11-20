#include "lcd1602.h"
#include "delay.h"

void lcd__clock() {
  gpio__set(lcd__enable);
  delay__ms(5);
  gpio__reset(lcd__enable);
  delay__ms(5);
}

void lcd__command(uint8_t command) {
  gpio__reset(lcd__read_write_select);
  RS_bit(0);
}

/* Initializing Functions */

void lcd__pins_init() {
  lcd__reg_select = gpio__construct_with_function(GPIO__PORT_1, 0, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__reg_select);

  lcd__read_write_select = gpio__construct_with_function(GPIO__PORT_1, 1, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__read_write_select);

  lcd__enable = gpio__construct_with_function(GPIO__PORT_1, 4, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__enable);

  lcd__db7 = gpio__construct_with_function(GPIO__PORT_1, 14, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__db7);

  lcd__db6 = gpio__construct_with_function(GPIO__PORT_4, 28, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__db6);

  lcd__db5 = gpio__construct_with_function(GPIO__PORT_4, 29, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__db5);

  lcd__db4 = gpio__construct_with_function(GPIO__PORT_0, 6, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__db4);

  lcd__db3 = gpio__construct_with_function(GPIO__PORT_0, 7, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__db3);

  lcd__db2 = gpio__construct_with_function(GPIO__PORT_0, 8, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__db2);

  lcd__db1 = gpio__construct_with_function(GPIO__PORT_0, 9, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__db1);

  lcd__db0 = gpio__construct_with_function(GPIO__PORT_0, 26, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__db0);
}

void lcd__init() {
  lcd__pins_init();
  delay__ms(40);

  lcd__set_4_bit_mode();
  lcd__set_2_line();
}

void RS_bit(int active__1h) {
  if (active__1h)
    gpio__set(lcd__reg_select);
  else
    gpio__reset(lcd__reg_select);
}

void DB7_bit(int active__1h) {
  if (active__1h)
    gpio__set(lcd__db7);
  else
    gpio__reset(lcd__db7);
}

void DB6_bit(int active__1h) {
  if (active__1h)
    gpio__set(lcd__db6);
  else
    gpio__reset(lcd__db6);
}

void DB5_bit(int active__1h) {
  if (active__1h)
    gpio__set(lcd__db5);
  else
    gpio__reset(lcd__db5);
}

void DB4_bit(int active__1h) {
  if (active__1h)
    gpio__set(lcd__db4);
  else
    gpio__reset(lcd__db4);
}

void DB3_bit(int active__1h) {
  if (active__1h)
    gpio__set(lcd__db3);
  else
    gpio__reset(lcd__db3);
}

void DB2_bit(int active__1h) {
  if (active__1h)
    gpio__set(lcd__db2);
  else
    gpio__reset(lcd__db2);
}

void DB1_bit(int active__1h) {
  if (active__1h)
    gpio__set(lcd__db1);
  else
    gpio__reset(lcd__db1);
}

void DB0_bit(int active__1h) {
  if (active__1h)
    gpio__set(lcd__db0);
  else
    gpio__reset(lcd__db0);
}
