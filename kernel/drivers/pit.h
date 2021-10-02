#pragma once

#include <types.h>

void pit_init(uint32_t frequency);
void pit_tick_increment(void);
uint32_t pit_tick_get(void);
uint32_t pit_read(void);
