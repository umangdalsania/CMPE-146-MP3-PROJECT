#include <stdio.h>
#include <string.h>

#include "board_io.h"
#include "delay.h"
#include "gpio.h"
#include "lpc40xx.h"

#include "clock.h"
#include "hw_timer.h"
#include "uart.h"

#include "ff.h"
#include "iap.h"

typedef enum {
  _s16_32k = 16,
  _s29_32k = 29,
} sector_e;

static const uint32_t application_start_address = 0x00010000;
static const uint32_t application_end_address = 0x0007FFFF;

/// Valid values: 256,512,1024,4096
static uint32_t application_file_buffer[4 * 1024 / sizeof(uint32_t)];

static const char *application_file_name = "lpc40xx_application.bin";
static const char *application_file_name_after_flash = "lpc40xx_application.bin.flashed";

static bool file_present(const char *filename);
static void copy_application_from_sd_card_to_flash(void);
static void flash__erase_application_flash(void);

static const unsigned *application_get_entry_function_address(void) {
  return (unsigned *)(application_start_address + 4);
}
static bool application_is_valid(void);
static void execute_user_app(void);

static inline void turn_on_led(const gpio_s led) { gpio__reset(led); }
static inline void turn_off_led(const gpio_s led) { gpio__set(led); }

int main(void) {
  delay__ms(100);
  puts("----------");
  puts("BOOTLOADER");
  puts("----------");

  const gpio_s bootloader_led = board_io__get_led0();
  const gpio_s file_present_led = board_io__get_led1();

  const gpio_s sw = gpio__construct_as_input(GPIO__PORT_0, 29);
  while (true) {
    if (true == gpio__get(sw)) {
      break;
    }
  };

  turn_on_led(bootloader_led);
  delay__ms(5000);

  if (file_present(application_file_name)) {
    turn_on_led(file_present_led);
    flash__erase_application_flash();
    copy_application_from_sd_card_to_flash();
  } else {
    printf("SD card %s\n", board_io__sd_card_is_present() ? "PRESENT" : "ABSENT");
    printf("%s not detected on SD card, attempting to boot application...", application_file_name);
  }

  puts("-----------------------------");
  puts("Attemping to boot application");

  if (application_is_valid()) {
    clock__uninit();
    hw_timer__disable(LPC_TIMER__0);
    // TODO: uninit SPI

    puts("----------------------");
    puts("Booting to application");
    puts("----------------------");

    // No more printfs after this
    uart__uninit(UART__0);
    execute_user_app();
  } else {
    const unsigned *application_entry_point = application_get_entry_function_address();
    printf("Application entry point: %p: %p\n", application_entry_point, (void *)(*application_entry_point));

    unsigned counter = 0;
    while (1) {
      ++counter;
      if (0 == (counter % 6)) {
        puts("ERROR: Application not valid, hence cannot boot to the application");
        printf("Load '%s' to the SD card to re-flash, and reboot this board\n", application_file_name);
      }

      gpio__toggle(bootloader_led);
      delay__ms(500);
    }
  }

  return 0;
}

static bool file_present(const char *filename) {
  bool present = false;
  FILINFO file_info;

  if (FR_OK == f_stat(filename, &file_info)) {
    // if not a directory, then this is a file
    present = !(file_info.fattrib & AM_DIR);
  }

  return present;
}

static void copy_application_from_sd_card_to_flash(void) {
  printf("Attempting to copy %s\n", application_file_name);

  FIL file;
  if (FR_OK == f_open(&file, application_file_name, FA_READ)) {
    printf("  Opened %s\n", application_file_name);

    uint32_t flash_write_address = application_start_address;
    while (true) {
      memset(application_file_buffer, 0xFF, sizeof(application_file_buffer));

      UINT bytes_read = 0;
      if (FR_OK != f_read(&file, application_file_buffer, sizeof(application_file_buffer), &bytes_read)) {
        break;
      }

      // End of file
      if (!(bytes_read > 0)) {
        puts("  End of file");
        break;
      }

      printf("\n  Read %u bytes\n", (unsigned)bytes_read);

#if 1
      uint8_t status = 0;

      status = Chip_IAP_PreSectorForReadWrite(_s16_32k, _s29_32k);
      printf("  Prepare sectors: %s (%u)\n", (0 == status) ? "OK" : "ERROR", (unsigned)status);

      status = Chip_IAP_CopyRamToFlash(flash_write_address, (uint32_t *)application_file_buffer,
                                       sizeof(application_file_buffer));
      printf("  Write %5u bytes to 0x%08lX: %s (%u)\n", sizeof(application_file_buffer), flash_write_address,
             (0 == status) ? "OK" : "ERROR", (unsigned)status);

      status =
          Chip_IAP_Compare(flash_write_address, (uint32_t)application_file_buffer, sizeof(application_file_buffer));
      printf("  Compare %5u bytes at 0x%08lX: %s (%u)\n", sizeof(application_file_buffer), flash_write_address,
             (0 == status) ? "OK" : "ERROR", (unsigned)status);
#endif

      flash_write_address += bytes_read;
    }
    f_close(&file);

#if 0
    if (FR_OK == f_rename(application_file_name, application_file_name_after_flash)) {
      printf("SUCCESS: Renamed %s to %s\n", application_file_name, application_file_name_after_flash);
    } else {
      printf("ERROR: Failed to rename %s to %s\n", application_file_name, application_file_name_after_flash);
    }
#endif
  } else {
    printf("  INFO: Could not open %s\n", application_file_name);
  }

  puts("");
}

static void flash__erase_application_flash(void) {
  uint8_t status = 0;

  printf("Preparing and erasing sectors...\n");

  status = Chip_IAP_PreSectorForReadWrite(_s16_32k, _s29_32k);
  printf("  Prepare sectors: %s (%u)\n", (0 == status) ? "OK" : "ERROR", (unsigned)status);

  status = Chip_IAP_EraseSector(_s16_32k, _s29_32k);
  printf("  Erased sectors : %s (%u)\n", (0 == status) ? "OK" : "ERROR", (unsigned)status);

  puts("");
}

static bool application_is_valid(void) {
  const unsigned *app_code_start = application_get_entry_function_address();

  return (*app_code_start >= application_start_address) && (*app_code_start <= application_end_address);
}

static void execute_user_app(void) {
  // Re-map Interrupt vectors to the user application
  SCB->VTOR = (application_start_address & 0x1FFFFF80);

  // Application code's RESET handler starts at Application Code + 4
  const unsigned *app_code_start = application_get_entry_function_address();

  // Get the function pointer of user application
  void (*user_code_entry)(void);
  user_code_entry = (void *)*app_code_start;

  // Set user application stack size and run it
  __set_PSP((uint32_t)(app_code_start - 4));
  __set_MSP((uint32_t)(app_code_start - 4));
  user_code_entry();
}
