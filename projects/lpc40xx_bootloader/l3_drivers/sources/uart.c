#include <stdlib.h>

#include "uart.h"

#include "lpc40xx.h"
#include "lpc_peripherals.h"

/// Alias the LPC defined typedef in case we have to define it differently for a different CPU
typedef LPC_UART_TypeDef lpc_uart;

/**
 * Each UART struct is composed of its register pointers, peripheral_id
 * and FreeRTOS queues to support interrupt driven implementation
 */
typedef struct {
  lpc_uart *registers;
  const char *name;
} uart_s;

/*******************************************************************************
 *
 *               P R I V A T E    D A T A    D E F I N I T I O N S
 *
 ******************************************************************************/

/**
 * Some UARTs have a different memory map, but it matches the base registers, so
 * we can use the same memory map to provide a generic driver for all UARTs
 *
 * UART1 for instance has same registers like UART0, but has additional modem control registers
 * but these extra registers are at the end of the memory map that matches with UART0
 */
static uart_s uarts[] = {
    {(lpc_uart *)LPC_UART0, "Uart0"},
    {(lpc_uart *)LPC_UART1, "Uart1"},
    {(lpc_uart *)LPC_UART2, "Uart2"},
    {(lpc_uart *)LPC_UART3, "Uart3"},
};

static const lpc_peripheral_e uart_peripheral_ids[] = {LPC_PERIPHERAL__UART0, LPC_PERIPHERAL__UART1,
                                                       LPC_PERIPHERAL__UART2, LPC_PERIPHERAL__UART3};

/*******************************************************************************
 *
 *                     P R I V A T E    F U N C T I O N S
 *
 ******************************************************************************/

static bool uart__is_receive_queue_enabled(uart_e uart) { return false; }
static bool uart__is_transmit_queue_enabled(uart_e uart) { return false; }

static void uart__wait_for_transmit_to_complete(lpc_uart *uart_regs) {
  const uint32_t transmitter_empty = (1 << 5);
  while (!(uart_regs->LSR & transmitter_empty)) {
  }
}

/*******************************************************************************
 *
 *                      P U B L I C    F U N C T I O N S
 *
 ******************************************************************************/

void uart__init(uart_e uart) {
  lpc_peripheral__turn_on_power_to(uart_peripheral_ids[uart]);

  const uint8_t dlab_bit = (1 << 7);
  const uint8_t eight_bit_datalen = 3;

  lpc_uart *uart_regs = uarts[uart].registers;

  uart_regs->LCR = dlab_bit; // Set DLAB bit to access DLM & DLL

  /* Bootloader uses Uart0 fractional dividers and can wreck havoc in our baud rate code, so re-initialize it
   * Lesson learned: DO NOT RELY ON RESET VALUES
   */
  const uint32_t default_reset_fdr_value = (1 << 4);
  uart_regs->FDR = default_reset_fdr_value;

  // Hardcoded for PCLK = 12MHz, and baud = 115200
  const uint16_t divider = 4;
  uart_regs->DLM = (divider >> 8) & 0xFF;
  uart_regs->DLL = (divider >> 0) & 0xFF;
  uart_regs->FDR = 5 | (8 << 4);

  // Important: Set FCR value before enable UART by writing the LCR register
  // Important: FCR is a write-only register, and we cannot use R/M/W such as |=
  const uint8_t enable_fifo = (1 << 0); // Must be done!
  const uint8_t eight_char_timeout = (2 << 6);
  uart_regs->FCR = enable_fifo;
  uart_regs->FCR = enable_fifo | eight_char_timeout;

  uart_regs->LCR = eight_bit_datalen; // DLAB is reset back to zero also
}

void uart__uninit(uart_e uart) {
  lpc_uart *uart_regs = uarts[uart].registers;

  uart_regs->LCR = (1 << 7);
  uart_regs->FDR = (1 << 4);
  uart_regs->DLL = 0;
  uart_regs->DLM = 0;
  uart_regs->LCR = 0;
  uart_regs->FCR = 0;
}

bool uart__is_initialized(uart_e uart) {
  return lpc_peripheral__is_powered_on(uart_peripheral_ids[uart]) && (0 != uarts[uart].registers->LCR);
}

bool uart__is_transmit_queue_initialized(uart_e uart) { return uart__is_transmit_queue_enabled(uart); }

bool uart__polled_get(uart_e uart, char *input_byte) {
  bool status = false;

  const bool rtos_is_running = false; // taskSCHEDULER_RUNNING == xTaskGetSchedulerState();
  const bool queue_is_enabled = uart__is_receive_queue_enabled(uart);

  if (uart__is_initialized(uart)) {
    /* If the RTOS is running and queues are enabled, then we will be unable to access the
     * RBR register directly since the interrupt would occur and read out the RBR.
     *
     * So when the user calls this function with the RTOS and queues enabled,
     * then we opt to block forever using uart__get() to provide 'polled' behavior.
     */
    if (rtos_is_running && queue_is_enabled) {
      // status = uart__get(uart, input_byte, UINT32_MAX);
    } else {
      lpc_uart *uart_regs = uarts[uart].registers;
      const uint32_t char_available_bitmask = (1 << 0);

      while (!(uart_regs->LSR & char_available_bitmask)) {
      }
      *input_byte = uart_regs->RBR;
    }
  }

  return status;
}

bool uart__polled_put(uart_e uart, char output_byte) {
  bool status = false;
  lpc_uart *uart_regs = uarts[uart].registers;

  if (uart__is_initialized(uart)) {
    status = true;

    // Wait for any prior transmission to complete
    uart__wait_for_transmit_to_complete(uart_regs);
    uart_regs->THR = output_byte;
    uart__wait_for_transmit_to_complete(uart_regs);
  }

  return status;
}
