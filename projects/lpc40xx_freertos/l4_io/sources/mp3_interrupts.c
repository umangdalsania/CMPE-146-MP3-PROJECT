#include <stdio.h>

#include "encoder.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "mp3_functions.h"

static function_pointer_t gpio_callbacks[32];

void mp3__interrupt_init(void) {
  /* Enabling INT for the 5 Button Switches */
  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, mp3__interrupt_dispatcher, "ISR");
  NVIC_EnableIRQ(GPIO_IRQn);

  /* Enabling INT for the Rotary Encoder */
  // lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__QEI, mp3__interrupt_dispatcher, "ISR");
  // NVIC_EnableIRQ(LPC_PERIPHERAL__QEI);
}

void mp3__interrupt_dispatcher(void) {
  int pin_that_triggered_interrupt;
  function_pointer_t attached_user_handler;

  mp3__check_pin(&pin_that_triggered_interrupt);

  attached_user_handler = gpio_callbacks[pin_that_triggered_interrupt];
  // Invoke the user registered callback, and then clear the interrupt
  attached_user_handler();

  mp3__clear_interrupt(pin_that_triggered_interrupt);
}

void mp3__attach_interrupt(gpio_s button, function_pointer_t callback) {
  // 1-D Array Bc no pin is reused for all 5 buttons
  gpio_callbacks[button.pin_number] = callback;

  // All Switches are on Port 2
  LPC_GPIOINT->IO2IntEnF |= (1 << button.pin_number);

  return;
}

void mp3__check_pin(int *pin_num) {
  for (int i = 0; i < 32; i++) {
    if (LPC_GPIOINT->IO2IntStatF & (1 << i)) {
      *pin_num = i;
    }
  }
}

void mp3__clear_interrupt(int pin_num) { LPC_GPIOINT->IO2IntClr |= (1 << pin_num); }
