#include <stdio.h>

#include "clock.h"
#include "peripherals_init.h"
#include "startup.h"
#include "sys_time.h"

extern void main(void);

void entry_point(void) {
  startup__initialize_ram();
  startup__initialize_fpu();
  startup__initialize_interrupts();

  clock__init();
  sys_time__init(clock__get_peripheral_clock_hz());

  // Peripherals init initializes UART and then we can print the crash report if applicable
  peripherals_init();

  main();

  while (1) {
    ;
  }
}
