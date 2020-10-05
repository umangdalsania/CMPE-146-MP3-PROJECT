#include "board_io.h"

#include "gpio.h"
#include "lpc40xx.h"

/**
 * Board information:
 *
 * P1.8 - SD CS
 * P1.9 - SD card detect
 * P1.10 - external flash CS
 *
 * P1.0 - SSP2 - SCK
 * P1.1 - SSP2 - MOSI
 * P1.4 - SSP2 - MISO
 */

static const uint32_t board_io__sd_card_cs_pin = (1 << 8);
static const uint32_t board_io__sd_card_detect_pin = (1 << 9);

void board_io__initialize(void) {
  // Note: return type of gpio__construct_with_function() because we do not need GPIO instance after its configuration

  gpio__construct_with_function(GPIO__PORT_0, 2, GPIO__FUNCTION_1); // P0.2 - Uart-0 Tx
  gpio__construct_with_function(GPIO__PORT_0, 3, GPIO__FUNCTION_1); // P0.3 - Uart-0 Rx

  // SPI bus 2 (SSP2)
  gpio__construct_with_function(GPIO__PORT_1, 0, GPIO__FUNCTION_4); // P1.0 - SCK2
  gpio__construct_with_function(GPIO__PORT_1, 1, GPIO__FUNCTION_4); // P1.1 - MOSI2
  gpio__construct_with_function(GPIO__PORT_1, 4, GPIO__FUNCTION_4); // P1.4 - MISO2

  // SD card
  gpio__construct_as_output(GPIO__PORT_1, 8); // SD card CS
  board_io__sd_card_ds();
  gpio__construct_as_input(GPIO__PORT_1, 9); // SD card detect
}

void board_io__uninit(void) {
  // UART
  gpio__construct_with_function(GPIO__PORT_0, 2, GPIO__FUNCITON_0_IO_PIN);
  gpio__construct_with_function(GPIO__PORT_0, 3, GPIO__FUNCITON_0_IO_PIN);

  // SPI
  gpio__construct_with_function(GPIO__PORT_1, 0, GPIO__FUNCITON_0_IO_PIN);
  gpio__construct_with_function(GPIO__PORT_1, 1, GPIO__FUNCITON_0_IO_PIN);
  gpio__construct_with_function(GPIO__PORT_1, 4, GPIO__FUNCITON_0_IO_PIN);
}

// Note: Not using gpio.h API here to optimize the speed of SSP CS selection
void board_io__sd_card_cs(void) { LPC_GPIO1->CLR = board_io__sd_card_cs_pin; }
void board_io__sd_card_ds(void) { LPC_GPIO1->SET = board_io__sd_card_cs_pin; }

bool board_io__sd_card_is_present(void) {
  const uint32_t card_present_bitmask = (LPC_GPIO1->PIN & board_io__sd_card_detect_pin);
  return (0 == card_present_bitmask); // Signal is active low
}
