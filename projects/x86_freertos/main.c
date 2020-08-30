#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

void vAssertCalled(unsigned long ulLine, const char *const pcFileName) {}

void task(void *p) {
  while (1) {
    puts("Hello");
    vTaskDelay(100);
  }
}

int main(void) {
  printf("Hello world!\n");

  xTaskCreate(task, "task", 1, NULL, 1, NULL);
  vTaskStartScheduler();

  return 0;
}
