#include "schedule_data.h"
#include "api.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

static lesson_t* lessons = NULL;
static int lesson_count = 0;
static struct tm last_fetched_date = { 0 };
static char* current_room_id = NULL;

void set_room_id(const char* room_id)
{
    if (current_room_id)
    {
        free(current_room_id);
    }

    current_room_id = strdup(room_id);
}

int get_lesson_count(void)
{
    return lesson_count;
}

lesson_t get_lesson(int index)
{
    if (index >= 0 && index < lesson_count)
    {
        return lessons[index];
    }

    static lesson_t empty = { 0 };
    return empty;
}

int get_lesson_count_for_date(struct tm* date)
{
    if (!current_room_id || !date) return 0;

    // Checking if the data needs to be updated
    if (date->tm_year != last_fetched_date.tm_year ||
        date->tm_mon != last_fetched_date.tm_mon ||
        date->tm_mday != last_fetched_date.tm_mday)
    {
        // Free previously allocated lessons
        for (int i = 0; i < lesson_count; i++)
        {
            free((char*)lessons[i].subject);
            free((char*)lessons[i].teacher);
            free((char*)lessons[i].type);
            free((char*)lessons[i].groups);
        }
        free(lessons);
        lessons = NULL;
        lesson_count = 0;

        if (fetch_schedule_data(current_room_id, date, &lessons, &lesson_count) != 0)
        {
            return 0;
        }
        last_fetched_date = *date;
    }
    return lesson_count;
}

lesson_t get_lesson_for_date(struct tm* date, int index)
{
    int count = get_lesson_count_for_date(date);
    if (count <= 0 || index < 0 || index >= count)
    {
        static lesson_t empty = { 0 };
        return empty;
    }

    return get_lesson(index);
}
