#include "mp3_functions.h"

#include "delay.h"
#include "lpc40xx.h"
#include "ssp0.h"
#include "stdlib.h"

#define SCI_MODE 0x0
#define SCI_CLOCKF 0x3
#define SCI_VOLUME 0xB
#define SCI_BASS 0x2

#define DEBUG 0

/*===========================================================*/
/*=================| Global Variable Decl |==================*/
/*===========================================================*/

QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;

SemaphoreHandle_t mp3_prev_bin_sem;
SemaphoreHandle_t mp3_next_bin_sem;
SemaphoreHandle_t mp3_pause_bin_sem;
SemaphoreHandle_t mp3_move_up_bin_sem;
SemaphoreHandle_t mp3_move_down_bin_sem;
SemaphoreHandle_t mp3_select_song_bin_sem;
SemaphoreHandle_t mp3_treble_bass_bin_sem;

volatile bool pause;
volatile int treble_bass_menu;
volatile bool playing_mode;
volatile size_t song_index;

/*===========================================================*/
/*=================| MP3 Hardware Control |==================*/
/*===========================================================*/

/* MP3 Decoder Pins */
static gpio_s mp3_dreq;
static gpio_s mp3_xcs;
static gpio_s mp3_xdcs;
static gpio_s mp3_reset;

/* MP3 Vol Vars */
static double volume_value = 0.0;
static int previous_index_value = 0;
static int current_vol_step = 50;

/* MP3 Treble and Bass Vars */
// Combined current_ST_AMPLITUDE and current_ST_FREQLIMIT
static uint8_t ST_AMPLITUDE_conversion_lookup_table[16] = {0xff, 0xef, 0xdf, 0xcf, 0xbf, 0xaf, 0x9f, 0x8f,
                                                           0x0f, 0x1f, 0x2f, 0x3f, 0x4f, 0x5f, 0x6f, 0x7f};
static uint8_t current_ST_AMPLITUDE = 8;
// static uint8_t current_ST_FREQLIMIT = 15;
static uint8_t current_SB_AMPLITUDE = 0;
static uint8_t current_SB_FREQLIMIT = 15;

void mp3__pins_init(void) {
  mp3_xcs = gpio__construct_with_function(GPIO__PORT_2, 7, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(mp3_xcs);
  mp3__ds();

  mp3_xdcs = gpio__construct_with_function(GPIO__PORT_2, 9, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(mp3_xdcs);
  mp3__data_ds();

  mp3_dreq = gpio__construct_with_function(GPIO__PORT_2, 8, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_input(mp3_dreq);

  mp3_reset = gpio__construct_with_function(GPIO__PORT_0, 16, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(mp3_reset);
  gpio__set(mp3_reset);
}

void mp3__decoder_init(void) {
  /* Variable Declarations */
  uint16_t default_settings = 0x4800;
  uint16_t freq_3x_multiplier = 0x6000;

  /* Initializing Decoder + SPI Port 0 */
  mp3__interrupt_init();
  mp3__pins_init();
  mp3__reset();
  ssp0__init(1);
  delay__ms(100);

  sj2_write_to_decoder(SCI_MODE, default_settings);
  delay__ms(100);

  sj2_write_to_decoder(SCI_CLOCKF, freq_3x_multiplier);
  delay__ms(100);

#if DEBUG
  printf("Status Read: 0x%04x\n", sj2_read_from_decoder(0x01));
  printf("Mode Read: 0x%04x\n", sj2_read_from_decoder(0x00));
#endif
  sj2_write_to_decoder(SCI_VOLUME, 0xfefe);

  sj2_write_to_decoder(SCI_BASS, 0x0f0f);
}

void mp3__init(void) {
  mp3__decoder_init();
  encoder__init();
  lcd__init();
  mp3__attach_interrupts_for_menu();

  mp3_prev_bin_sem = xSemaphoreCreateBinary();
  mp3_next_bin_sem = xSemaphoreCreateBinary();
  mp3_pause_bin_sem = xSemaphoreCreateBinary();
  mp3_move_up_bin_sem = xSemaphoreCreateBinary();
  mp3_move_down_bin_sem = xSemaphoreCreateBinary();
  mp3_select_song_bin_sem = xSemaphoreCreateBinary();
  mp3_treble_bass_bin_sem = xSemaphoreCreateBinary();

  Q_songname = xQueueCreate(1, sizeof(songname_t));
  Q_songdata = xQueueCreate(1, sizeof(songdata_t));

  pause = false;
  treble_bass_menu = 0;
  playing_mode = false;
  song_index = song_list__get_item_count();
  song_list__populate();
  mp3__print_songs_in_menu();
}

/*===========================================================*/
/*==============| MP3 Communication Controls |===============*/
/*===========================================================*/

void sj2_write_to_decoder(uint8_t reg, uint16_t data) {
  mp3__cs();
  ssp0__exchange_byte(0x2);
  ssp0__exchange_byte(reg);
  ssp0__exchange_byte((data >> 8) & 0xFF);
  ssp0__exchange_byte((data >> 0) & 0xFF);
  mp3__ds();
}

uint16_t sj2_read_from_decoder(uint8_t reg) {
  uint16_t results = 0;

  mp3__cs();
  ssp0__exchange_byte(0x3);
  ssp0__exchange_byte(reg);
  results |= (ssp0__exchange_byte(0xFF) << 8);
  results |= (ssp0__exchange_byte(0xFF) << 0);
  mp3__ds();

  return results;
}

void sj2_to_mp3_decoder(char data) {
  mp3__data_cs();
  ssp0__exchange_byte(data);
  mp3__data_ds();
}

void mp3__reset(void) {
  gpio__reset(mp3_reset); // Reset Pin Is Active Low
  delay__ms(200);
  gpio__set(mp3_reset);
}

void mp3__cs(void) { gpio__reset(mp3_xcs); }
void mp3__ds(void) { gpio__set(mp3_xcs); }

void mp3__data_cs(void) { gpio__reset(mp3_xdcs); }
void mp3__data_ds(void) { gpio__set(mp3_xdcs); }

gpio_s mp3__get_dreq(void) { return mp3_dreq; }

/*===========================================================*/
/*=================| File Related Controls |=================*/
/*===========================================================*/

bool open_file(FIL *file_handler, char *song_name) {
  if (f_open(file_handler, song_name, FA_READ) == FR_OK)
    return true;

  return false;
}

void close_file(FIL *file_handler) { f_close(file_handler); }

void read_from_file(FIL *file_handler, char *buffer, UINT *Bytes_Read) {
  int counter = 0;

  while (1) {
    f_read(file_handler, buffer, sizeof(songdata_t), Bytes_Read);
    counter++;

    if (*Bytes_Read == 0 || (uxQueueMessagesWaiting(Q_songname) == 1))
      break;

    xQueueSend(Q_songdata, buffer, portMAX_DELAY);
  }
}

/*===========================================================*/
/*==================| MP3 Volume Control |===================*/
/*===========================================================*/

void mp3__volume_adjuster(void) {

  if (previous_index_value == encoder__get_index()) {
    return;
  }

  volume_value = mp3__get_volume_value();
  uint8_t volume_value_for_one_ear = 254 * (1 - ((volume_value * 0.50) + 0.5));
  uint16_t volume_value_to_both_ears = (volume_value_for_one_ear << 8) | (volume_value_for_one_ear << 0);

  sj2_write_to_decoder(SCI_VOLUME, volume_value_to_both_ears);
  if (playing_mode) {
    mp3__display_volume();
  }
}

double mp3__get_volume_value(void) {
  int get_index = encoder__get_index();

  if (current_vol_step == 50) {
    if (get_index < previous_index_value) {
      previous_index_value = get_index;
      return 0;
    }

    else
      current_vol_step++;
  }

  else if (current_vol_step == 100) {
    if (get_index > previous_index_value) {
      previous_index_value = get_index;
      return 1;
    }

    else
      current_vol_step--;

  }

  else {
    if (get_index > previous_index_value)
      current_vol_step++;
    else
      current_vol_step--;
  }

  previous_index_value = get_index;
  return (current_vol_step / 100.0);
}

void mp3__display_volume(void) {

  int vol_step_for_display = (current_vol_step - 50) / 3;

  mp3__clear_volume_positions();
  lcd__set_position(3, 4);

  if (current_vol_step == 100) {
    for (int i = 0; i < 17; i++) {
      lcd__print('=');
    }
  }

  else {
    for (int i = 0; i < vol_step_for_display; i++) {
      lcd__print('=');
    }
  }
}

void mp3__clear_volume_positions(void) {
  char *clear = "-----------------";

  lcd__set_position(3, 4);

  for (int i = 0; i < 17; i++)
    lcd__print(clear[i]);
}

/*===========================================================*/
/*=================| SCI_BASS |==================*/
// Name         Bits   Description
// ST_AMPLITUDE 15:12  Treble Control in 1.5 dB steps (-8..7, 0 = off)
// ST_FREQLIMIT 11:8   Lower limit frequency in 1000 Hz steps (1..15)
// SB_AMPLITUDE 7:4    Bass Enhancement in 1 dB steps (0..15, 0 = off)
// SB_FREQLIMIT 3:0    Lower limit frequency in 10 Hz steps (2..15)
/*===========================================================*/

uint16_t mp3_get_treble_and_bass_value(void) {

  printf("----------------------------------------\n");
  printf("current_ST_AMPLITUDE Read: %d\n", current_ST_AMPLITUDE);
  printf("current_ST_AMPLITUDE Read:  0x%4x\n", current_ST_AMPLITUDE);

  uint16_t treble_and_bass_value = 0;

  // treble_and_bass_value |= (ST_AMPLITUDE_conversion_lookup_table[current_ST_AMPLITUDE] << 8) |
  // (current_SB_AMPLITUDE);

  treble_and_bass_value |= (ST_AMPLITUDE_conversion_lookup_table[current_ST_AMPLITUDE] << 8);
  printf("treble_and_bass_value Read: 0x%4x\n", treble_and_bass_value);

  // treble_and_bass_value |= ((current_ST_AMPLITUDE - 8) << 12);
  // treble_and_bass_value |= (current_ST_FREQLIMIT << 8);

  treble_and_bass_value |= (current_SB_AMPLITUDE << 4);
  printf("current_SB_AMPLITUDE Read:  0x%4x\n", current_SB_AMPLITUDE);
  printf("treble_and_bass_value Read: 0x%4x\n", treble_and_bass_value);

  treble_and_bass_value |= (current_SB_FREQLIMIT << 0);
  printf("current_SB_FREQLIMIT Read:  0x%4x\n", current_SB_FREQLIMIT);
  printf("treble_and_bass_value Read: 0x%4x\n", treble_and_bass_value);
  printf("----------------------------------------\n");

  return treble_and_bass_value;
}

void mp3__treble_adjuster(void) {

  if (previous_index_value == encoder__get_index()) {
    return;
  }

  uint16_t treble_and_bass_value = 0;
  mp3__update_treble_value();
  treble_and_bass_value = mp3_get_treble_and_bass_value();

  sj2_write_to_decoder(SCI_BASS, treble_and_bass_value);
  mp3__display_treble();
}

void mp3__update_treble_value(void) {
  int get_index = encoder__get_index();

  if (current_ST_AMPLITUDE == 0) {
    if (get_index < previous_index_value) {
      previous_index_value = get_index;
      return;
    }

    else
      current_ST_AMPLITUDE++;
  }

  else if (current_ST_AMPLITUDE == 15) {
    if (get_index > previous_index_value) {
      previous_index_value = get_index;
      return;
    }

    else
      current_ST_AMPLITUDE--;
  }

  else {
    if (get_index > previous_index_value)
      current_ST_AMPLITUDE++;
    else
      current_ST_AMPLITUDE--;
  }

  previous_index_value = get_index;
  return;
}

void mp3__display_treble(void) {

  int8_t treble_step_for_display = current_ST_AMPLITUDE;
  char treble_step_for_display_str[10];

  treble_step_for_display = treble_step_for_display - 8;
  sprintf(treble_step_for_display_str, "Treble:%2d", treble_step_for_display);
  lcd__print_string(treble_step_for_display_str, 3);
}

void mp3__bass_adjuster(void) {

  if (previous_index_value == encoder__get_index()) {
    return;
  }

  uint16_t treble_and_bass_value = 0;
  mp3__update_bass_value();
  treble_and_bass_value = mp3_get_treble_and_bass_value();

  sj2_write_to_decoder(SCI_BASS, treble_and_bass_value);
  mp3__display_bass();
}

void mp3__update_bass_value(void) {
  int get_index = encoder__get_index();

  if (current_SB_AMPLITUDE == 0) {
    if (get_index < previous_index_value) {
      previous_index_value = get_index;
      return;
    }

    else
      current_SB_AMPLITUDE++;
  }

  else if (current_SB_AMPLITUDE == 15) {
    if (get_index > previous_index_value) {
      previous_index_value = get_index;
      return;
    }

    else
      current_SB_AMPLITUDE--;
  }

  else {
    if (get_index > previous_index_value)
      current_SB_AMPLITUDE++;
    else
      current_SB_AMPLITUDE--;
  }

  previous_index_value = get_index;
  return;
}

void mp3__display_bass(void) {

  uint8_t bass_step_for_display = current_SB_AMPLITUDE;
  char bass_step_for_display_str[10];
  sprintf(bass_step_for_display_str, "Bass:%d", bass_step_for_display);
  lcd__print_string(bass_step_for_display_str, 3);
}

/*===========================================================*/
/*==================| MP3 Menu Functions |===================*/
/*===========================================================*/

void mp3__decrement_song_index(void) {
  if (song_index == 0) {
    song_index = song_list__get_item_count();
  }
  song_index--;
}

void mp3__increment_song_index(void) {
  song_index++;
  if (song_index >= song_list__get_item_count()) {
    song_index = 0;
  }
}

void mp3__print_songs_in_menu(void) {
  lcd__print_string("=== Menu", 1);
  mp3__decrement_song_index();
  lcd__print_string(song_list__get_name_for_item(song_index), 2);
  mp3__increment_song_index();
  lcd__print_string(song_list__get_name_for_item(song_index), 3);
  mp3__increment_song_index();
  lcd__print_string(song_list__get_name_for_item(song_index), 4);
  mp3__decrement_song_index();

  lcd__print_selector(3);
}

void mp3__display_now_playing(void) {
  lcd__clear();
  lcd__print_string("=== Playing ", 1);
  lcd__print_string(song_list__get_name_for_item(song_index), 2);
  lcd__print_string("Vol", 4);
  mp3__display_volume();
  xQueueSend(Q_songname, song_list__get_name_for_item(song_index), portMAX_DELAY);
}

void mp3__display_treble_menu(void) {
  lcd__clear();
  lcd__print_string("=== Treble Menu", 1);
  mp3__display_treble();
}

void mp3__display_bass_menu(void) {
  lcd__clear();
  lcd__print_string("=== Bass Menu", 1);
  mp3__display_bass();
}