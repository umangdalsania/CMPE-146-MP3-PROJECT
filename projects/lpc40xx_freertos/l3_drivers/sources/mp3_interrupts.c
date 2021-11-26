#include "encoder.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "mp3_functions.h"

#include <stdio.h>

void mp3__ISR_init(void) {
  LPC_QEI->INXCMP0 = 0xA;
  // LPC_QEI->INXCMP1 = 2;

  LPC_QEI->IES |= (1 << 9);
  // LPC_QEI->IES |= (1 << 13);

  lpc_peripheral__enable_interrupt(QEI_IRQn, mp3__interrupt_dispatcher, "Dispatcher");
  NVIC_EnableIRQ(QEI_IRQn);
}

void mp3__interrupt_dispatcher(void) {
  function_pointer_t handler;
  uint16_t volume_increase = (1 << 13);
  uint16_t volume_decrease = (1 << 14);

  handler = mp3__volume_adjuster;

  handler();

  LPC_QEI->CLR &= ~volume_increase;
  LPC_QEI->CLR &= ~volume_decrease;
}
