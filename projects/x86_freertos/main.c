#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

static void task(void *p) {
  unsigned counter = 0;
  while (1) {
    printf("Counter = %u\n", counter);

    ++counter;
    vTaskDelay(500);
  }
}

int main(void) {
  xTaskCreate(task, "task", 1, NULL, PRIORITY_LOW, NULL);

  puts("Starting FreeRTOS Scheduler\n");
  vTaskStartScheduler();

  return 0;
}
