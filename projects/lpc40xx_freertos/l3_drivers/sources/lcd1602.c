#include "lcd1602.h"
#include "delay.h"

static uint8_t x_position = 0;
static uint8_t y_position = 0;

void lcd__clock() {
  gpio__set(lcd__enable);
  delay__ms(1);
  gpio__reset(lcd__enable);
  delay__ms(1);
}

void lcd__command(uint8_t command) {

  RS_bit(0);
  RW_bit(0);
  DB7_bit(((1 << 7) & command));
  DB6_bit(((1 << 6) & command));
  DB5_bit(((1 << 5) & command));
  DB4_bit(((1 << 4) & command));
  DB3_bit(((1 << 3) & command));
  DB2_bit(((1 << 2) & command));
  DB1_bit(((1 << 1) & command));
  DB0_bit(((1 << 0) & command));

  lcd__clock();
}

/* Printing onto LCD Screen */

void lcd__print(uint8_t character) {

  if (x_position > 15) {
    if (y_position > 0) {
      lcd__set_position(0, 0);
      y_position = 0;
    } else {
      y_position++;
      lcd__set_position(0, 1);
    }

    x_position = 0;
  }

  RS_bit(1);
  RW_bit(0);

  DB7_bit(((1 << 7) & character));
  DB6_bit(((1 << 6) & character));
  DB5_bit(((1 << 5) & character));
  DB4_bit(((1 << 4) & character));
  DB3_bit(((1 << 3) & character));
  DB2_bit(((1 << 2) & character));
  DB1_bit(((1 << 1) & character));
  DB0_bit(((1 << 0) & character));

  lcd__clock();

  x_position++;
}

void lcd__print_string(const char *song_name) {
  for (int i = 0; i < 32; i++) {
    if (song_name[i] == '\0')
      break;

    lcd__print(song_name[i]);
  }
}

void lcd__set_position(uint8_t x, uint8_t y) {
  if (y == 1) {
    x += 0x40;
  }

  lcd__command(0x80 | x);
}

/* Pin Configurations */

void RS_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__reg_select);
  else
    gpio__reset(lcd__reg_select);
}

void RW_bit(bool read__1h) {
  if (read__1h)
    gpio__set(lcd__read_write_select);
  else
    gpio__reset(lcd__read_write_select);
}

void DB7_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db7);
  else
    gpio__reset(lcd__db7);
}

void DB6_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db6);
  else
    gpio__reset(lcd__db6);
}

void DB5_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db5);
  else
    gpio__reset(lcd__db5);
}

void DB4_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db4);
  else
    gpio__reset(lcd__db4);
}

void DB3_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db3);
  else
    gpio__reset(lcd__db3);
}

void DB2_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db2);
  else
    gpio__reset(lcd__db2);
}

void DB1_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db1);
  else
    gpio__reset(lcd__db1);
}

void DB0_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db0);
  else
    gpio__reset(lcd__db0);
}

/* Init Functions */

void lcd__pins_init() {
  lcd__reg_select = gpio__construct_with_function(GPIO__PORT_1, 31, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__reg_select);
  gpio__reset(lcd__reg_select);

  lcd__read_write_select = gpio__construct_with_function(GPIO__PORT_1, 20, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__read_write_select);
  gpio__reset(lcd__read_write_select);

  lcd__enable = gpio__construct_with_function(GPIO__PORT_1, 28, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__enable);
  gpio__reset(lcd__enable);

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
  uint8_t eight_bit_mode = 0x30;
  uint8_t set_two_line = 0x08;
  uint8_t set_font_5_8 = 0x04;
  uint8_t display_on = 0x0F;
  uint8_t display_off = 0x08;
  uint8_t display_clear = 0x01;
  uint8_t entry_mode_increment_on_shift_off = 0x6;

  lcd__pins_init();
  delay__ms(50);

  lcd__command(eight_bit_mode);
  lcd__command(eight_bit_mode);
  lcd__command(eight_bit_mode);

  lcd__command(eight_bit_mode | set_two_line | set_font_5_8); // 0x3c

  lcd__command(display_off);
  lcd__command(display_clear);
  lcd__command(entry_mode_increment_on_shift_off);
  lcd__command(display_on);
}
