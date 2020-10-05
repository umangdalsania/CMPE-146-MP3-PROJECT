#pragma once

#include <stdbool.h>

#include "gpio.h"

void board_io__initialize(void);
void board_io__uninit(void);

void board_io__sd_card_cs(void);
void board_io__sd_card_ds(void);
bool board_io__sd_card_is_present(void);
