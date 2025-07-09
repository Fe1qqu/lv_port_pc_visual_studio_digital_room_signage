#ifndef SCHEDULE_DATA_H
#define SCHEDULE_DATA_H

typedef struct {
    const char* type;
    const char* subject;
    const char* teacher;
    int start_hour;
    int start_minute;
    int end_hour;
    int end_minute;
} lesson_t;

lesson_t get_lesson(int index);
int get_lesson_count(void);

#endif
