
#include <stdio.h>

#include "board_io.h"
// #include "common_macros.h"
#include "delay.h"
#include "ff.h"
// #include "i2c.h"
#include "clock.h"
#include "ssp2.h"
#include "uart.h"

static const char *peripherals_init__mount_sd_card(void);
static void peripherals_init__uart0_init(void);

void peripherals_init(void) {
  board_io__initialize();

  // UART initialization is required in order to use <stdio.h> puts, printf() etc; @see system_calls.c
  peripherals_init__uart0_init();

  const uint32_t spi_sd_max_speed_khz = 24 * 1000;
  ssp2__initialize(spi_sd_max_speed_khz);
  peripherals_init__mount_sd_card();
}

static const char *peripherals_init__mount_sd_card(void) {
  // This FATFS object should never go out of scope
  static FATFS sd_card_drive;
  const char *mount_info = "";

  const BYTE option_mount_now = 1;
  const TCHAR *default_drive = (const TCHAR *)"";

  if (FR_OK == f_mount(&sd_card_drive, default_drive, option_mount_now)) {
    mount_info = ("SD card mounted successfully\n");
  } else {
    mount_info = ("WARNING: SD card could not be mounted\n");
  }

  return mount_info;
}

static void peripherals_init__uart0_init(void) {
  // Do not do any bufferring for standard input otherwise getchar(), scanf() may not work
  setvbuf(stdin, 0, _IONBF, 0);

  // Note: PIN functions are initialized by board_io__initialize() for P0.2(Tx) and P0.3(Rx)
  uart__init(UART__0);
}
