#include "encoder.h"
#include "gpio.h"
#include "lpc40xx.h"

static gpio_s encA, encB;

void encoder__init(void) { encoder__pins_init(); }

void encoder__pins_init(void) {

  /* 5 Button Switches */
  center_button = gpio__construct_with_function(GPIO__PORT_1, 29, GPIO__FUNCITON_0_IO_PIN);
  down_button = gpio__construct_with_function(GPIO__PORT_2, 0, GPIO__FUNCITON_0_IO_PIN);
  right_button = gpio__construct_with_function(GPIO__PORT_2, 1, GPIO__FUNCITON_0_IO_PIN);
  up_button = gpio__construct_with_function(GPIO__PORT_2, 2, GPIO__FUNCITON_0_IO_PIN);
  left_button = gpio__construct_with_function(GPIO__PORT_2, 4, GPIO__FUNCITON_0_IO_PIN);

  /* Encoder Functions */
  // encA = gpio__construct_with_function(GPIO__PORT_1, 20, GPIO__FUNCTION_4);
  // encA = gpio__construct_with_function(GPIO__PORT_1, 20, GPIO__FUNCTION_4);

  encoder__set_as_input();
}

void encoder__set_as_input(void) {
  /* Setting as Input */

  gpio__set_as_input(center_button);
  gpio__set_as_input(down_button);
  gpio__set_as_input(right_button);
  gpio__set_as_input(up_button);
  gpio__set_as_input(left_button);

  /*
   * Note: Pins are configured as pull-up resistor enabled
   * on reset, no need to modify that. Rotary Encoder uses
   * 5 switches and will set the pin low upon press.
   */
}