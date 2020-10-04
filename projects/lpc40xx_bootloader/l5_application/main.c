#include <stdio.h>

#include "board_io.h"
#include "delay.h"

int main(void) {
  const gpio_s bootloader_led = board_io__get_led0();

  while (1) {
    gpio__toggle(bootloader_led);
    delay__ms(100);
  }

  return 0;
}
