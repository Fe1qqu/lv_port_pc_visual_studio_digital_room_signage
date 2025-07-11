#include "schedule_ui.h"
#include "schedule_data.h"
#include <lvgl/lvgl.h>
#include <stdio.h>
#include <time.h>

#define MAX_NUMBER_OF_LESSONS 16
//const uint8_t MAX_NUMBER_OF_LESSONS = 16;

static lv_obj_t* list_container;
static lv_obj_t* progress_bars[MAX_NUMBER_OF_LESSONS]; // Store lessons progress bars
static struct tm current_display_date;

void update_schedule_display(struct tm* display_date)
{
    if (!list_container || !display_date) return;

    // Clear existing content
    lv_obj_clean(list_container);
    for (int i = 0; i < MAX_NUMBER_OF_LESSONS; i++)
    {
        progress_bars[i] = NULL;
    }

    // Store display date
    memcpy(&current_display_date, display_date, sizeof(struct tm));

    // Get current time
    time_t now = time(NULL);
    struct tm* current_time = localtime(&now);
    int current_minutes = current_time->tm_hour * 60 + current_time->tm_min;

    // Compare dates (ignoring time)
    int is_past_date = 0, is_future_date = 0;
    if (display_date->tm_year < current_time->tm_year ||
        (display_date->tm_year == current_time->tm_year && display_date->tm_mon < current_time->tm_mon) ||
        (display_date->tm_year == current_time->tm_year && display_date->tm_mon == current_time->tm_mon &&
         display_date->tm_mday < current_time->tm_mday))
    {
        is_past_date = 1;
    }
    else if (display_date->tm_year > current_time->tm_year ||
        (display_date->tm_year == current_time->tm_year && display_date->tm_mon > current_time->tm_mon) ||
        (display_date->tm_year == current_time->tm_year && display_date->tm_mon == current_time->tm_mon &&
         display_date->tm_mday > current_time->tm_mday))
    {
        is_future_date = 1;
    }

    // Get total number of lessons
    int lesson_count = get_lesson_count(); // Update to get_lesson_count_for_date(&current_display_date)

    // Create a block for each lesson
    for (int i = 0; i < lesson_count; i++)
    {
        lesson_t lesson = get_lesson(i); // Update to get_lesson_for_date(&current_display_date, i)

        // Create block container
        lv_obj_t* block = lv_obj_create(list_container);
        lv_obj_set_size(block, 650, 150); // Block size
        lv_obj_set_pos(block, 0, i * 160); // Stack vertically with spacing
        lv_obj_set_style_bg_color(block, lv_color_hex(0xF0F0F0), 0); // Light background
        lv_obj_set_style_border_width(block, 1, 0);
        lv_obj_set_style_pad_all(block, 10, 0);

        // Progress bar
        lv_obj_t* progress_bar = lv_bar_create(block);
        lv_obj_set_size(progress_bar, 600, 20);
        lv_bar_set_range(progress_bar, 0, 100);
        lv_obj_align(progress_bar, LV_ALIGN_TOP_MID, 0, 0);
        //lv_obj_set_style_bg_color(progress_bar, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
        progress_bars[i] = progress_bar;

        // Calculate progress
        int progress = 0;
        if (is_past_date)
        {
            progress = 100;
        }
        else if (is_future_date)
        {
            progress = 0;
        }
        else
        {
            int start_minutes = lesson.start_hour * 60 + lesson.start_minute;
            int end_minutes = lesson.end_hour * 60 + lesson.end_minute;
            if (current_minutes > end_minutes)
            {
                progress = 100;
            }
            else if (current_minutes >= start_minutes && current_minutes <= end_minutes)
            {
                progress = ((current_minutes - start_minutes) * 100) / (end_minutes - start_minutes);
            }
        }
        lv_bar_set_value(progress_bar, progress, LV_ANIM_ON);

        char buffer[6];

        // Start time label
        lv_obj_t* start_time_label = lv_label_create(block);
        snprintf(buffer, sizeof(buffer), "%02d:%02d", lesson.start_hour, lesson.start_minute);
        lv_label_set_text(start_time_label, buffer);
        lv_obj_align_to(start_time_label, progress_bar, LV_ALIGN_TOP_LEFT, 5, -2);
        lv_obj_set_style_text_font(start_time_label, &lv_font_my_montserrat_20, 0);

        // End time label
        lv_obj_t* end_time_label = lv_label_create(block);
        snprintf(buffer, sizeof(buffer), "%02d:%02d", lesson.end_hour, lesson.end_minute);
        lv_label_set_text(end_time_label, buffer);
        lv_obj_align_to(end_time_label, progress_bar, LV_ALIGN_TOP_RIGHT, -18, -2);
        lv_obj_set_style_text_font(end_time_label, &lv_font_my_montserrat_20, 0);

        // Type label
        lv_obj_t* type_label = lv_label_create(block);
        //snprintf(buffer, sizeof(buffer), "%s", lesson.type);
        lv_label_set_text(type_label, lesson.type);
        lv_obj_align(type_label, LV_ALIGN_TOP_MID, 0, 30);
        lv_obj_set_style_text_font(type_label, &lv_font_my_montserrat_20, 0);

        // Subject label (SCROLL)
        lv_obj_t* subject_label = lv_label_create(block);
        //snprintf(buffer, sizeof(buffer), "%s", lesson.subject);
        lv_label_set_text(subject_label, lesson.subject);
        lv_label_set_long_mode(subject_label, LV_LABEL_LONG_MODE_SCROLL);
        lv_obj_set_style_anim_duration(subject_label, 3000, 0);
        lv_obj_set_width(subject_label, 600);
        lv_obj_align(subject_label, LV_ALIGN_TOP_MID, 0, 60);
        lv_obj_set_style_text_font(subject_label, &lv_font_my_montserrat_20, 0);

        // Teacher label
        lv_obj_t* teacher_label = lv_label_create(block);
        //snprintf(buffer, sizeof(buffer), "%s", lesson.teacher);
        lv_label_set_text(teacher_label, lesson.teacher);
        lv_obj_set_width(teacher_label, 600);
        lv_obj_align(teacher_label, LV_ALIGN_TOP_MID, 0, 90);
        lv_obj_set_style_text_font(teacher_label, &lv_font_my_montserrat_20, 0);
    }
}

void update_progress_bar(void)
{
    if (!list_container) return;

    // Get current time
    time_t now = time(NULL);
    struct tm* current_time = localtime(&now);
    int current_minutes = current_time->tm_hour * 60 + current_time->tm_min;

    // Check if displayed date is today
    int is_today = (current_display_date.tm_year == current_time->tm_year &&
                    current_display_date.tm_mon == current_time->tm_mon &&
                    current_display_date.tm_mday == current_time->tm_mday);
    if (!is_today) return;

    // Get total number of lessons
    int lesson_count = get_lesson_count(); // Update to get_lesson_count_for_date(&current_display_date)

    // MIGHT BE OPTIMIZED
    // Update progress bar of current lesson
    for (int i = 0; i < lesson_count; i++)
    {
        if (progress_bars[i])
        {
            lesson_t lesson = get_lesson(i); // Update to get_lesson_for_date(&current_display_date, i)
            int start_minutes = lesson.start_hour * 60 + lesson.start_minute;
            int end_minutes = lesson.end_hour * 60 + lesson.end_minute;
            int progress = 0;
            if (current_minutes > end_minutes)
            {
                progress = 100;
            }
            else if (current_minutes >= start_minutes && current_minutes <= end_minutes)
            {
                progress = ((current_minutes - start_minutes) * 100) / (end_minutes - start_minutes);
            }
            lv_bar_set_value(progress_bars[i], progress, LV_ANIM_ON);
        }
    }
}

void init_schedule_ui(void)
{
    // Create main container
    list_container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(list_container, 700, 400);
    lv_obj_center(list_container);
    lv_obj_set_scroll_dir(list_container, LV_DIR_VER); // Vertical scrolling only
    lv_obj_set_scrollbar_mode(list_container, LV_SCROLLBAR_MODE_ON);
    lv_obj_set_style_bg_color(list_container, lv_color_hex(0xFFFFFF), 0); // White background
    lv_obj_add_flag(list_container, LV_OBJ_FLAG_SCROLLABLE);

    // Initial update (current date)
    time_t now = time(NULL);
    struct tm* current_date = localtime(&now);
    update_schedule_display(current_date);
}
