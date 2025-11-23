#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <time.h>

uint64_t get_current_time_ms();
void sleep_ms(int milliseconds);

#endif // TIMER_H
