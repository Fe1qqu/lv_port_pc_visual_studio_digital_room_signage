#ifndef API_H
#define API_H

#include "schedule_data.h"

struct tm;

int fetch_schedule_data(const char* room_id, const struct tm* date, lesson_t** lessons, int* lesson_count);

#endif
