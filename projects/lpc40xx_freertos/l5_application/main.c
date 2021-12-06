#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "delay.h"
#include "gpio.h"
#include "mp3_functions.h"
#include "task.h"

#include "sj2_cli.h"
#include "song_list.h"

/* Global Variable Declarations */

bool player_mode = false;
extern bool interrupt_received;
TaskHandle_t mp3_player_handle; // Used to Resume or Suspend MP3 Player

/* MP3 Related Functions */
static void mp3_reader_task(void *p);
static void mp3_player_task(void *p);
static void mp3_control(void *p);
static void check_for_interrupt(void);

int main(void) {

  sj2_cli__init();
  mp3__init();

  xTaskCreate(mp3_reader_task, "Mp3_Reader", 4096 / sizeof(void), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(mp3_player_task, "Mp3_Player", 4096 / sizeof(void), NULL, PRIORITY_HIGH, &mp3_player_handle);
  xTaskCreate(mp3_control, "MP3_Controller", 4096 / sizeof(void), NULL, PRIORITY_HIGH, NULL);

  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}

static void mp3_reader_task(void *p) {
  FIL file_handler;
  songname_t name;
  songdata_t buffer;
  UINT Bytes_Read;

  while (1) {
    xQueueReceive(Q_songname, &name.song_name, portMAX_DELAY);
    if (open_file(&file_handler, name.song_name)) {
      read_from_file(&file_handler, buffer.song_data, &Bytes_Read);
      close_file(&file_handler);
    }
  }
}

static void mp3_player_task(void *p) {

  char bytes_512[512];
  gpio_s mp3_dreq = mp3__get_dreq();

  while (1) {
    if (xQueueReceive(Q_songdata, &bytes_512, portMAX_DELAY)) {

      for (int i = 0; i < sizeof(bytes_512); i++) {
        while (!gpio__get(mp3_dreq)) {
          ; // If decoder buffer is full
        }

        sj2_to_mp3_decoder(bytes_512[i]);
      }
    }
  }
}

static void mp3_control(void *p) {
  while (1) {
    if (treble_bass_menu == 0)
      mp3__volume_adjuster();
    else if (treble_bass_menu == 1)
      mp3__treble_adjuster();
    else if (treble_bass_menu == 2)
      mp3__bass_adjuster();

    check_for_interrupt();
    vTaskDelay(50);
  }
}

void check_for_interrupt(void) {

  if (interrupt_received) {
    if (xSemaphoreTakeFromISR(mp3_treble_bass_bin_sem, 0)) {
      treble_bass_menu++;
      if (treble_bass_menu > 1)
        treble_bass_menu = 0;

      if (treble_bass_menu == 1)
        mp3__BASS_BUTTON_MENU_handler();
      else {
        mp3__display_now_playing();
        if (pause) {
          lcd__print_string("=== Paused", 1);
        }
      }
    }

    else {
      if (xSemaphoreTakeFromISR(mp3_prev_bin_sem, 0)) {
        if (pause) {
          pause = false;
          vTaskResume(mp3_player_handle);
        }
        mp3__PREV_handler();
      }

      if (xSemaphoreTakeFromISR(mp3_next_bin_sem, 0)) {
        if (pause) {
          pause = false;
          vTaskResume(mp3_player_handle);
        }
        mp3__NEXT_handler();
      }

      if (xSemaphoreTakeFromISR(mp3_pause_bin_sem, 0)) {
        if (pause) {
          lcd__print_string("=== Paused", 1);
          vTaskSuspend(mp3_player_handle);
        } else {
          lcd__print_string("=== Playing", 1);
          vTaskResume(mp3_player_handle);
        }
      }

      if (xSemaphoreTakeFromISR(mp3_move_up_bin_sem, 0)) {
        mp3__MOVE_UP_handler();
      }

      if (xSemaphoreTakeFromISR(mp3_move_down_bin_sem, 0)) {
        mp3__MOVE_DOWN_handler();
      }

      if (xSemaphoreTakeFromISR(mp3_select_song_bin_sem, 0)) {
        mp3__CENTER_BUTTON_4_MENU_handler();
        vTaskResume(mp3_player_handle);
        pause = false;
      }
    }

    vTaskDelay(300);
    interrupt_received = false;
  }
}
