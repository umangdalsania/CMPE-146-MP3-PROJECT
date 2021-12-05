#pragma once
#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "ff.h"
#include "gpio.h"
#include "queue.h"
#include "semphr.h"

#include "encoder.h"
#include "lcd1602.h"
#include "song_attributes.h"
#include "song_list.h"

typedef void (*function_pointer_t)(void);

extern QueueHandle_t Q_songname;
extern QueueHandle_t Q_songdata;

extern SemaphoreHandle_t mp3_prev_bin_sem;
extern SemaphoreHandle_t mp3_next_bin_sem;
extern SemaphoreHandle_t mp3_pause_bin_sem;
extern SemaphoreHandle_t mp3_move_up_bin_sem;
extern SemaphoreHandle_t mp3_move_down_bin_sem;
extern SemaphoreHandle_t mp3_select_song_bin_sem;
extern SemaphoreHandle_t mp3_treble_bass_bin_sem;

extern volatile bool pause;
extern volatile int treble_bass_menu;
extern volatile bool playing_mode;
extern volatile size_t song_index;

/*===========================================================*/
/*=================| MP3 Hardware Control |==================*/
/*===========================================================*/

void mp3__init(void);
void mp3__decoder_init(void);
void mp3__pins_init(void);
void mp3__reset(void);

void sj2_write_to_decoder(uint8_t reg, uint16_t data);
uint16_t sj2_read_from_decoder(uint8_t addr);

void sj2_to_mp3_decoder(char byte);

void mp3__volume_adjuster(void);
double mp3__get_volume_value(void);

int mp3_get_treble_and_bass_value(void);
void mp3__treble_adjuster(void);
void mp3__update_treble_value(void);
void mp3__bass_adjuster(void);
void mp3__update_bass_value(void);

void mp3__cs(void);
void mp3__ds(void);
void mp3__data_cs(void);
void mp3__data_ds(void);

gpio_s mp3__get_dreq(void);

/*===========================================================*/
/*=================| File Related Controls |=================*/
/*===========================================================*/

bool open_file(FIL *file_handler, char *song_name);
void close_file(FIL *file_handler);
void read_from_file(FIL *file_handler, char *data, UINT *Bytes_Read);

/*===========================================================*/
/*==================| MP3 Menu Functions |===================*/
/*===========================================================*/

void mp3__increment_song_index(void);
void mp3__decrement_song_index(void);
void mp3__print_songs_in_menu(void);
void mp3__display_now_playing(void);
void mp3__display_treble_menu(void);
void mp3__display_bass_menu(void);
void mp3__attach_interrupts_for_menu(void);

/*===========================================================*/
/*=========| MP3 Interrupt for LCD & Volume Rocker |=========*/
/*===========================================================*/

void mp3__interrupt_init(void);
void mp3__display_volume(void);
void mp3__display_treble(void);
void mp3__display_bass(void);
void mp3__clear_volume_positions(void);
void mp3__gpio_interrupt_dispatcher(void);
void mp3__encoder_interrupt_dispatcher(void);
void mp3__attach_interrupt(gpio_s button, function_pointer_t callback);
void mp3__check_pin(int *pin_num);
void mp3__clear_interrupt(int pin_num);

/*===========================================================*/
/*==============| Service Routine + Handlers |===============*/
/*===========================================================*/

void mp3__NEXT_ISR(void);
void mp3__PREV_ISR(void);
void mp3__PLAY_PAUSE_ISR(void);
void mp3__MOVE_UP_ISR(void);
void mp3__MOVE_DOWN_ISR(void);
void mp3__CENTER_BUTTON_4_MENU_ISR(void);
void mp3__TREBLE_BASS_BUTTON_MENU_ISR(void);

void mp3__NEXT_handler(void);
void mp3__PREV_handler(void);
void mp3__MOVE_UP_handler(void);
void mp3__MOVE_DOWN_handler(void);
void mp3__CENTER_BUTTON_4_MENU_handler(void);
void mp3__TREBLE_BUTTON_MENU_handler(void);
void mp3__BASS_BUTTON_MENU_handler(void);
void display_now_playing_handler(void);
void display_menu_handler(void);