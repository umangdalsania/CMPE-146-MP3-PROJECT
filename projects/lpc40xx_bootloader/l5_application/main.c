#include <stdio.h>
#include <string.h>

#include "board_io.h"
#include "delay.h"
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

static const uint32_t APP_START_ADDRESS = 0x00010000;
static const uint32_t APP_END_ADDRESS = 0x0007FFFF;

static const char *APP_FW_FILENAME = "lpc40xx_application.bin";
static const char *APP_FW_FILENAME_AFTER_FLASHING = "lpc40xx_application.bin.flashed";

static bool flash__fw_file_present(const char *filename);
static void flash__copy_from_sd_card(void);
static void flash__write_data(const void *data, size_t data_size_in_bytes, uint32_t flash_write_address);
static void flash__rename_file(void);
static void flash__erase_application_flash(void);

static void handle_application_boot(void);
static const unsigned *application_get_entry_function_address(void);
static bool application_is_valid(void);
static void application_execute(void);

static const char *get_status_string(uint8_t status) { return (0 == status) ? "OK" : "ERROR"; }

int main(void) {
  const char *line = "------------------------------";

  delay__ms(100);
  puts(line);
  puts("--------- BOOTLOADER ---------");
  printf("-- SD card: %6s\n", board_io__sd_card_is_present() ? "OK" : "ABSENT");
  puts(line);
  delay__ms(100);

  if (flash__fw_file_present(APP_FW_FILENAME)) {
    printf("INFO: Located new FW file: %s\n", APP_FW_FILENAME);

    flash__erase_application_flash();
    flash__copy_from_sd_card();
    flash__rename_file();
  } else {
    printf("INFO: %s not detected on SD card\n", APP_FW_FILENAME);
  }

  puts("");
  puts(line);
  puts("Attemping to boot application");
  handle_application_boot();

  return 0;
}

static bool flash__fw_file_present(const char *filename) {
  bool present = false;
  FILINFO file_info;

  if (FR_OK == f_stat(filename, &file_info)) {
    // if not a directory, then this is a file
    present = !(file_info.fattrib & AM_DIR);
  }

  return present;
}

static void flash__copy_from_sd_card(void) {
  FIL file;
  static uint32_t file_buffer[32 * 1024 / sizeof(uint32_t)] __attribute__((aligned(256)));
  const size_t flash_write_chunk_size = 4096; // Valid values: 256,512,1024,4096

  if (FR_OK == f_open(&file, APP_FW_FILENAME, FA_READ)) {
    printf("  Opened %s\n", APP_FW_FILENAME);

    uint32_t flash_write_address = APP_START_ADDRESS;
    while (true) {
      memset(file_buffer, 0xFF, sizeof(file_buffer));

      UINT bytes_read = 0;
      if (FR_OK != f_read(&file, file_buffer, sizeof(file_buffer), &bytes_read)) {
        break;
      }

      // End of file
      if (!(bytes_read > 0)) {
        break;
      }

      printf("  Read %u bytes\n", (unsigned)bytes_read);
      for (uint32_t address_offset = 0; address_offset < bytes_read; address_offset += flash_write_chunk_size) {
        const void *data = (const void *)file_buffer + address_offset;
        flash__write_data(data, flash_write_chunk_size, flash_write_address);
        flash_write_address += flash_write_chunk_size;
      }
    }
    f_close(&file);
  }

  puts("");
}

static void flash__write_data(const void *data, size_t data_size_in_bytes, uint32_t flash_write_address) {
  uint8_t status = 0;
  status = Chip_IAP_PreSectorForReadWrite(_s16_32k, _s29_32k);
  if (0 != status) {
    printf("    Prepare sectors: %s (%u)\n", get_status_string(status), (unsigned)status);
  }

  status = Chip_IAP_CopyRamToFlash(flash_write_address, (uint32_t *)data, data_size_in_bytes);
  printf("    Write %u bytes to 0x%08lX: %s (%u)\n", data_size_in_bytes, flash_write_address, get_status_string(status),
         (unsigned)status);

  status = Chip_IAP_Compare(flash_write_address, (uint32_t)data, data_size_in_bytes);
  if (0 != status) {
    printf("    Compare %u bytes at 0x%08lX: %s (%u)\n", data_size_in_bytes, flash_write_address,
           get_status_string(status), (unsigned)status);
  }
}

static void flash__rename_file(void) {
  if (FR_OK == f_rename(APP_FW_FILENAME, APP_FW_FILENAME_AFTER_FLASHING)) {
    printf("SUCCESS: Renamed %s to %s\n", APP_FW_FILENAME, APP_FW_FILENAME_AFTER_FLASHING);
  } else {
    printf("ERROR: Failed to rename %s to %s\n", APP_FW_FILENAME, APP_FW_FILENAME_AFTER_FLASHING);
  }
}

static void flash__erase_application_flash(void) {
  uint8_t status = 0;

  printf("Preparing and erasing sectors...\n");

  status = Chip_IAP_PreSectorForReadWrite(_s16_32k, _s29_32k);
  printf("  Prepare sectors: %s (%u)\n", get_status_string(status), (unsigned)status);

  status = Chip_IAP_EraseSector(_s16_32k, _s29_32k);
  printf("  Erased sectors : %s (%u)\n", get_status_string(status), (unsigned)status);

  puts("");
}

static void handle_application_boot(void) {
  if (application_is_valid()) {
    hw_timer__disable(LPC_TIMER__0);
    // TODO: uninit SPI

    puts("  Booting...\n\n");

    // No more printfs after this
    clock__uninit();
    uart__uninit(UART__0);
    board_io__uninit();

    application_execute();
  } else {
    const unsigned *application_entry_point = application_get_entry_function_address();
    printf("Application entry point: %p: %p\n", application_entry_point, (void *)(*application_entry_point));

    while (1) {
      puts("ERROR: Application not valid, hence cannot boot to the application");
      printf("Load '%s' to the SD card to re-flash, and reboot this board\n", APP_FW_FILENAME);

      delay__ms(3000);
    }
  }
}

static const unsigned *application_get_entry_function_address(void) { return (unsigned *)(APP_START_ADDRESS + 4); }

static bool application_is_valid(void) {
  const unsigned *app_code_start = application_get_entry_function_address();

  return (*app_code_start >= APP_START_ADDRESS) && (*app_code_start <= APP_END_ADDRESS);
}

static void application_execute(void) {
  // Re-map Interrupt vectors to the user application
  SCB->VTOR = (APP_START_ADDRESS & 0x1FFFFF80);

  // Application code's RESET handler starts at Application Code + 4
  const unsigned *app_code_start = application_get_entry_function_address();

  // Get the function pointer of user application
  void (*user_code_entry)(void) = (void *)*app_code_start;

  // Application flash should have interrupt vector and first entry should be the stack pointer of that application
  const uint32_t stack_pointer_of_application = *(uint32_t *)(APP_START_ADDRESS);
  __set_PSP(stack_pointer_of_application);
  __set_MSP(stack_pointer_of_application);

  user_code_entry();
}
