#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "ff.h"
#include "gpio.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#include "encoder.h"
#include "lcd1602.h"
#include "mp3_functions.h"
#include "sj2_cli.h"
#include "song_list.h"
#include "songname_t.h"

enum State { next, previous, paused, idle, movedown, moveup, playing };

typedef struct {
  char song_data[512];
} songdata_t;

/* Global Variable Declarations */
QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;

SemaphoreHandle_t mp3_state_signal;

extern gpio_s center_button, down_button, right_button, up_button, left_button;

int song_index = 0;
int current_state = idle;
bool pause = false;

TaskHandle_t PlayPause; // Used to Resume or Suspend MP3 Player

/* MP3 Related Functions */
static void mp3_reader_task(void *p);
static void mp3_player_task(void *p);
static void mp3_control(void *p);

/* Menu-Rleated Functions */

static void increment_song_index(void);
static void decrement_song_index(void);
static void print_songs_in_menu(void);
static void attach_interrupts_for_menu(void);
static void display_now_playing(void);

static void play_next_ISR(void);
static void play_prev_ISR(void);
static void play_pause_ISR(void);
static void move_up_list_ISR(void);
static void move_down_list_ISR(void);
static void center_button_for_menu_ISR(void);

/* File Related Functions */
static bool open_file(FIL *file_handler, char *song_name);
static void close_file(FIL *file_handler);
static void read_from_file(FIL *file_handler, char *data, UINT *Bytes_Read);

int main(void) {

  sj2_cli__init();
  encoder__init();
  lcd__init();
  mp3__init();
  attach_interrupts_for_menu();

  mp3_state_signal = xSemaphoreCreateBinary();

  lcd__print_string("=== Select A Song", 0);
  song_list__populate();
  print_songs_in_menu();

  Q_songname = xQueueCreate(1, sizeof(songname_t));
  Q_songdata = xQueueCreate(1, sizeof(songdata_t));

  xTaskCreate(mp3_reader_task, "Mp3_Reader", 4096 / sizeof(void), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(mp3_player_task, "Mp3_Player", 4096 / sizeof(void), NULL, PRIORITY_HIGH, &PlayPause);
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

static bool open_file(FIL *file_handler, char *song_name) {
  if (f_open(file_handler, song_name, FA_READ) == FR_OK)
    return true;

  return false;
}

static void close_file(FIL *file_handler) {
  if (f_close(file_handler) == FR_OK)
    lcd__print_string("Song Over", 3);
}

static void read_from_file(FIL *file_handler, char *buffer, UINT *Bytes_Read) {
  int counter = 0;

  while (1) {
    f_read(file_handler, buffer, sizeof(songdata_t), Bytes_Read);
    counter++;

    if (*Bytes_Read == 0 || (uxQueueMessagesWaiting(Q_songname) == 1))
      break;

    xQueueSend(Q_songdata, buffer, portMAX_DELAY);
  }
}

static void mp3_player_task(void *p) {

  char bytes_512[512];
  gpio_s mp3_dreq = {2, 8};

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

static void decrement_song_index(void) {
  if (song_index == 0) {
    song_index = song_list__get_item_count();
  }
  song_index--;
}

static void increment_song_index(void) {
  song_index++;
  if (song_index >= song_list__get_item_count()) {
    song_index = 0;
  }
}

static void print_songs_in_menu(void) {
  decrement_song_index();
  lcd__print_string(song_list__get_name_for_item(song_index), 2);
  increment_song_index();
  lcd__print_string(song_list__get_name_for_item(song_index), 3);
  increment_song_index();
  lcd__print_string(song_list__get_name_for_item(song_index), 4);
  decrement_song_index();

  lcd__print_arrow(3);
}

static void display_now_playing(void) {
  lcd__clear();
  lcd__print_string("Now Playing: ", 1);
  lcd__print_string(song_list__get_name_for_item(song_index), 2);
  xQueueSend(Q_songname, song_list__get_name_for_item(song_index), portMAX_DELAY);
}

// Interrupt Functions
static void play_next_ISR(void) {
  current_state = next;
  xSemaphoreGiveFromISR(mp3_state_signal, NULL);
}
static void play_prev_ISR(void) {
  current_state = previous;
  xSemaphoreGiveFromISR(mp3_state_signal, NULL);
}
static void play_pause_ISR(void) {
  current_state = paused;
  pause = !pause;
  xSemaphoreGiveFromISR(mp3_state_signal, NULL);
}
static void move_up_list_ISR(void) {
  current_state = moveup;
  xSemaphoreGiveFromISR(mp3_state_signal, NULL);
}
static void move_down_list_ISR(void) {
  current_state = movedown;
  xSemaphoreGiveFromISR(mp3_state_signal, NULL);
}
static void center_button_for_menu_ISR(void) {
  current_state = playing;
  xSemaphoreGiveFromISR(mp3_state_signal, NULL);
}

static void attach_interrupts_for_menu(void) {
  mp3__attach_interrupt(center_button, center_button_for_menu_ISR);
  mp3__attach_interrupt(left_button, play_prev_ISR);
  mp3__attach_interrupt(right_button, play_next_ISR);
  mp3__attach_interrupt(up_button, move_up_list_ISR);
  mp3__attach_interrupt(down_button, move_down_list_ISR);
}

static void mp3_control(void *p) {
  while (1) {
    if (xSemaphoreTake(mp3_state_signal, portMAX_DELAY)) {
      switch (current_state) {
      case paused:
        if (pause)
          vTaskResume(PlayPause);
        else
          vTaskSuspend(PlayPause);
        break;

      case next:
        mp3__attach_interrupt(center_button, play_pause_ISR);
        increment_song_index();
        display_now_playing();
        break;

      case previous:
        mp3__attach_interrupt(center_button, play_pause_ISR);
        decrement_song_index();
        display_now_playing();
        break;

      case moveup:
        mp3__attach_interrupt(center_button, center_button_for_menu_ISR);
        lcd__print_string("=== Select A Song:", 1);
        decrement_song_index();
        print_songs_in_menu();
        break;

      case movedown:
        mp3__attach_interrupt(center_button, center_button_for_menu_ISR);
        lcd__print_string("=== Select A Song:", 1);
        increment_song_index();
        print_songs_in_menu();
        break;

      case playing:
        mp3__attach_interrupt(center_button, play_pause_ISR);
        display_now_playing();
        vTaskResume(PlayPause);

        break;
      }

      current_state = idle;
    }
  }
}