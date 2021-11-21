#include <stdio.h>

#include "FreeRTOS.h"
#include "gpio.h"
#include "queue.h"
#include "songname_t.h"
#include "task.h"

#include "lcd1602.h"
#include "mp3_functions.h"
#include "song_list.h"

#include "ff.h"
#include "sj2_cli.h"

typedef struct {
  char song_data[512];
} songdata_t;

QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;

// Reader tasks receives song-name over Q_songname to start reading it
static void mp3_reader_task(void *p);

// Player task receives song data over Q_songdata to send it to the MP3 decoder
static void mp3_player_task(void *p);

// Misc Functions
bool open_file(FIL *file_handler, char *song_name);
void close_file(FIL *file_handler);

void read_from_file(FIL *file_handler, char *data, UINT *Bytes_Read);

int main(void) {

  sj2_cli__init();
  lcd__init();
  mp3__init();

  song_list__populate();

  for (size_t song_number = 0; song_number < song_list__get_item_count(); song_number++) {
    printf("Song %2d: %s\n", (1 + song_number), song_list__get_name_for_item(song_number));
  }

  Q_songname = xQueueCreate(1, sizeof(songname_t));
  Q_songdata = xQueueCreate(1, sizeof(songdata_t));

  xTaskCreate(mp3_reader_task, "Mp3_Reader", 4096 / sizeof(void), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(mp3_player_task, "Mp3_Player", 4096 / sizeof(void), NULL, PRIORITY_HIGH, NULL);

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
    printf("Received [%s] to play.\n", name.song_name);
    lcd__clear();
    lcd__print_string(name.song_name);

    if (open_file(&file_handler, name.song_name)) {
      read_from_file(&file_handler, buffer.song_data, &Bytes_Read);
      close_file(&file_handler);
    }
  }
}

bool open_file(FIL *file_handler, char *song_name) {
  if (f_open(file_handler, song_name, FA_READ) == FR_OK) {
    printf("Playing queued song.\n");
    return true;
  }

  else
    printf("File cannot be found on microSD Card!\n");

  return false;
}

void close_file(FIL *file_handler) {
  if (f_close(file_handler) == FR_OK)
    printf("Song is over.\n");
}

void read_from_file(FIL *file_handler, char *buffer, UINT *Bytes_Read) {
  int counter = 0;

  while (1) {
    f_read(file_handler, buffer, sizeof(songdata_t), Bytes_Read);
    counter++;

    if (*Bytes_Read == 0)
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