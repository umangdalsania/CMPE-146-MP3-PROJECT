#pragma once

#include <stdint.h>

#include "lpc40xx.h"

static inline void clock__init(void) { LPC_SC->PCLKSEL = 1; }

static inline void clock__uninit(void) { LPC_SC->PCLKSEL = 4; }

static inline uint32_t clock__get_core_clock_hz(void) { return (UINT32_C(12) * 1000 * 1000); }

static inline uint32_t clock__get_peripheral_clock_hz(void) { return clock__get_core_clock_hz(); }