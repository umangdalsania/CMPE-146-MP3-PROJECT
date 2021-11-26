#include "encoder.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"

static gpio_s center_button, down_button, right_button, up_button, left_button;

static gpio_s encA, encB;

void encoder__init(void) {
  encoder__pins_init();
  encoder__turn_on_power();
  encoder__set_max_position();
}

void encoder__pins_init(void) {

  /* 5 Button Switches */
  center_button = gpio__construct_with_function(GPIO__PORT_1, 29, GPIO__FUNCITON_0_IO_PIN);
  down_button = gpio__construct_with_function(GPIO__PORT_2, 0, GPIO__FUNCITON_0_IO_PIN);
  right_button = gpio__construct_with_function(GPIO__PORT_2, 1, GPIO__FUNCITON_0_IO_PIN);
  up_button = gpio__construct_with_function(GPIO__PORT_2, 2, GPIO__FUNCITON_0_IO_PIN);
  left_button = gpio__construct_with_function(GPIO__PORT_2, 4, GPIO__FUNCITON_0_IO_PIN);

  /* Encoder Functions */
  encA = gpio__construct_with_function(GPIO__PORT_1, 20, GPIO__FUNCTION_3);
  encB = gpio__construct_with_function(GPIO__PORT_1, 23, GPIO__FUNCTION_3);

  encoder__set_as_input();
}

void encoder__set_as_input(void) {
  /*
   * Note: Pins are configured as pull-up resistor enabled
   * on reset, no need to modify that. Rotary Encoder uses
   * 5 switches and will set the pin low upon press.
   */

  gpio__set_as_input(center_button);
  gpio__set_as_input(down_button);
  gpio__set_as_input(right_button);
  gpio__set_as_input(up_button);
  gpio__set_as_input(left_button);
}

void encoder__reset_index(void) {
  LPC_QEI->CON |= (1 << 3);
  LPC_QEI->CON &= ~(1 << 3);
}

void encoder__turn_on_power(void) { lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__QEI); }
uint8_t encoder__get_position(void) { return (LPC_QEI->POS & 0xFF); }
uint32_t encoder__get_index(void) { return (LPC_QEI->INXCNT); }
void encoder__set_max_position(void) { LPC_QEI->MAXPOS = 1; }
