#include <stdio.h>

#include "board_io.h"
#include "delay.h"
#include "encoder.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "mp3_functions.h"

static function_pointer_t gpio_callbacks[32];
bool interrupt_received = false;

/*===========================================================*/
/*==================| MP3 GPIO Interrupts |==================*/
/*===========================================================*/

void mp3__interrupt_init(void) {
  /* Enabling INT for the 5 Button Switches */
  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, mp3__gpio_interrupt_dispatcher, "GPIO_ISR");
  NVIC_EnableIRQ(GPIO_IRQn);
}

void mp3__gpio_interrupt_dispatcher(void) {
  int pin_that_triggered_interrupt;
  function_pointer_t attached_user_handler;

  mp3__check_pin(&pin_that_triggered_interrupt);

  attached_user_handler = gpio_callbacks[pin_that_triggered_interrupt];
  // Invoke the user registered callback, and then clear the interrupt
  attached_user_handler();

  mp3__clear_interrupt(pin_that_triggered_interrupt);
}

void mp3__attach_interrupt(gpio_s button, function_pointer_t callback) {
  // 1-D Array Bc no pin is reused for all 5 buttons
  gpio_callbacks[button.pin_number] = callback;

  // All Switches are on Port 2
  LPC_GPIOINT->IO2IntEnF |= (1 << button.pin_number);

  return;
}
void mp3__check_pin(int *pin_num) {
  for (int i = 0; i < 32; i++) {
    if (LPC_GPIOINT->IO2IntStatF & (1 << i)) {
      *pin_num = i;
    }
  }
}

void mp3__attach_interrupts_for_menu(void) {
  mp3__attach_interrupt(get_extra_button(), mp3__TREBLE_BASS_BUTTON_MENU_ISR);
  mp3__attach_interrupt(get_center_button(), mp3__CENTER_BUTTON_4_MENU_ISR);
  mp3__attach_interrupt(get_left_button(), mp3__PREV_ISR);
  mp3__attach_interrupt(get_right_button(), mp3__NEXT_ISR);
  mp3__attach_interrupt(get_up_button(), mp3__MOVE_UP_ISR);
  mp3__attach_interrupt(get_down_button(), mp3__MOVE_DOWN_ISR);
}

void mp3__clear_interrupt(int pin_num) { LPC_GPIOINT->IO2IntClr |= (1 << pin_num); }

/*===========================================================*/
/*==================| MP3 Menu Interrupts |==================*/
/*===========================================================*/

void mp3__NEXT_ISR(void) {
  if (interrupt_received == false) {
    interrupt_received = true;
    xSemaphoreGiveFromISR(mp3_next_bin_sem, NULL);
  }
}
void mp3__PREV_ISR(void) {
  if (interrupt_received == false) {
    interrupt_received = true;
    xSemaphoreGiveFromISR(mp3_prev_bin_sem, NULL);
  }
}
void mp3__PLAY_PAUSE_ISR(void) {
  if (interrupt_received == false) {
    interrupt_received = true;
    pause = !pause;
    xSemaphoreGiveFromISR(mp3_pause_bin_sem, NULL);
  }
}
void mp3__MOVE_UP_ISR(void) {
  if (interrupt_received == false) {
    interrupt_received = true;
    xSemaphoreGiveFromISR(mp3_move_up_bin_sem, NULL);
  }
}
void mp3__MOVE_DOWN_ISR(void) {
  if (interrupt_received == false) {
    interrupt_received = true;
    xSemaphoreGiveFromISR(mp3_move_down_bin_sem, NULL);
  }
}
void mp3__CENTER_BUTTON_4_MENU_ISR(void) {
  if (interrupt_received == false) {
    interrupt_received = true;
    xSemaphoreGiveFromISR(mp3_select_song_bin_sem, NULL);
  }
}
void mp3__TREBLE_BASS_BUTTON_MENU_ISR(void) {
  if (interrupt_received == false) {
    interrupt_received = true;
    xSemaphoreGiveFromISR(mp3_treble_bass_bin_sem, NULL);
  }
}

void mp3__NEXT_handler(void) {
  playing_mode = true;
  lcd__clear();
  mp3__increment_song_index();
  mp3__display_now_playing();
  xQueueSend(Q_songname, song_list__get_name_for_item(song_index), portMAX_DELAY);
}

void mp3__PREV_handler(void) {
  playing_mode = true;
  lcd__clear();
  mp3__decrement_song_index();
  mp3__display_now_playing();
  xQueueSend(Q_songname, song_list__get_name_for_item(song_index), portMAX_DELAY);
}
void mp3__MOVE_UP_handler(void) {
  playing_mode = false;
  mp3__attach_interrupt(get_center_button(), mp3__CENTER_BUTTON_4_MENU_ISR);
  lcd__clear();
  mp3__decrement_song_index();
  mp3__print_songs_in_menu();
}
void mp3__MOVE_DOWN_handler(void) {
  playing_mode = false;
  mp3__attach_interrupt(get_center_button(), mp3__CENTER_BUTTON_4_MENU_ISR);
  lcd__clear();
  mp3__increment_song_index();
  mp3__print_songs_in_menu();
}
void mp3__CENTER_BUTTON_4_MENU_handler(void) {
  playing_mode = true;
  mp3__attach_interrupt(get_center_button(), mp3__PLAY_PAUSE_ISR);
  lcd__clear();
  mp3__display_now_playing();
}

void mp3__BASS_BUTTON_MENU_handler(void) {
  mp3__attach_interrupt(get_extra_button(), mp3__TREBLE_BASS_BUTTON_MENU_ISR);
  mp3__display_bass_menu();
}

void mp3__TREBLE_BUTTON_MENU_handler(void) {
  mp3__attach_interrupt(get_extra_button(), mp3__TREBLE_BASS_BUTTON_MENU_ISR);
  lcd__clear();
  mp3__display_treble_menu();
}

void display_menu_handler(void) {
  lcd__clear();
  mp3__print_songs_in_menu();
}