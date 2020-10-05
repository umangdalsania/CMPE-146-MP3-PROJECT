#pragma once

#include <stdbool.h>
#include <stdint.h>

// #include "FreeRTOS.h"
// #include "queue.h"

typedef enum {
  UART__0,
  UART__1,
  UART__2,
  UART__3,
} uart_e;

/**
 * This initializes minimal UART communication
 */
void uart__init(uart_e uart);

void uart__uninit(uart_e uart);

bool uart__is_initialized(uart_e uart);
bool uart__is_transmit_queue_initialized(uart_e uart);

/**
 * If the RTOS is going to be used, then enabling the queues provides event driven implementation such that uart_get()
 * and uart__put() operate off of queues and you do not have to resort to using polled (cpu eating) version
 *
 * The transmit queue will hold data, such that uart__put() can deposit data and return immediately rather than poll.
 * Likewise, the received data is stored to the receive queue, such that uart__get() will sleep for reception.
 *
 * @note Each the the queues should be of item type 'sizeof(char)'
 */
// bool uart__enable_queues(uart_e uart, QueueHandle_t queue_receive, QueueHandle_t queue_transmit);

/**
 * Polled version to get a char; this never returns until a char is obtained
 * @returns false if UART was not initialized
 */
bool uart__polled_get(uart_e uart, char *input_byte);

/**
 * Polled version to output a char; this returns after outputting the byte
 * @returns false if UART was not initialized
 */
bool uart__polled_put(uart_e uart, char output_byte);
