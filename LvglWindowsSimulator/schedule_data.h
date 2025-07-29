﻿#ifndef SCHEDULE_DATA_H
#define SCHEDULE_DATA_H

#include <stdint.h>

struct tm;

/**
 * Structure representing a single lesson in the schedule.
 * Contains information about the lesson type, subject, teacher, and time.
 */
typedef struct {
    char* type;       /* Type of the lesson */
    char* subject;    /* Subject of the lesson */
    char* teacher;    /* Name of the teacher */
    char* groups;     /* Groups of the lesson */
    uint32_t color;         /* Background color for the lesson type */
    int start_hour;         /* Start hour of the lesson */
    int start_minute;       /* Start minute of the lesson */
    int end_hour;           /* End hour of the lesson */
    int end_minute;         /* End minute of the lesson */
} lesson_t;

/**
 * Sets the room ID for fetching schedule data.
 * @param room_id Pointer to a string containing the room ID.
 */
void set_room_id(const char* room_id);

/**
 * Retrieves a lesson by its index for the current date.
 * @param  index Index of the lesson to retrieve.
 * @return The lesson_t structure containing lesson details.
 */
lesson_t get_lesson(int index);

/**
 * Gets the total number of lessons for the current date.
 * @return The number of lessons in the schedule.
 */
int get_lesson_count(void);

/**
 * Gets the total number of lessons for a specified date.
 * @param date  Pointer to a struct tm containing the date to query (year, month, day).
 * @return The number of lessons for the specified date.
 */
int get_lesson_count_for_date(struct tm* date);

/**
 * Retrieves a lesson by its index for a specified date.
 * @param date  Pointer to a struct tm containing the date to query (year, month, day).
 * @param index  Index of the lesson to retrieve.
 * @return The lesson_t structure containing lesson details.
 */
lesson_t get_lesson_for_date(struct tm* date, int index);

#endif
